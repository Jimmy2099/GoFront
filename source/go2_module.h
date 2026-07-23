//  Copyright 2026 Herb Sutter
//  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

//===========================================================================
//  Go2 module and package source assembly
//===========================================================================

#ifndef CPP2_GO2_MODULE_H
#define CPP2_GO2_MODULE_H

#include "go2.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace cpp2 {

struct go2_module_load_result {
    bool handled = false;
    bool success = false;
    std::string text;
};

class go2_module_loader {
    struct import_spec {
        std::string path;
        std::string alias;
    };

    struct import_binding {
        std::string alias;
        std::string target;
        bool standard = false;
    };

    struct source_file {
        std::filesystem::path path;
        std::vector<std::string> lines;
        std::vector<bool> skipped;
        std::vector<bool> cpp1_passthrough;
        std::string package_name;
        std::vector<import_spec> imports;
    };

    enum class package_state { loading, loaded };

    struct module {
        std::filesystem::path root;
        std::filesystem::path manifest;
        std::string name;
        std::string go_version;
        std::string toolchain;
        std::map<std::string, std::string> requirements;
        std::map<std::pair<std::string, std::string>, std::filesystem::path> replacements;
        std::map<std::string, std::set<std::string>> exclusions;
    };

    struct resolved_module {
        module* owner = nullptr;
        std::string version;
        std::string source;
    };

    struct package {
        std::filesystem::path directory;
        package_state state = package_state::loading;
        bool entry = false;
        std::string module_key;
        std::string import_path;
        std::string cpp_namespace;
        std::string name;
        std::vector<source_file> files;
        std::vector<import_binding> bindings;
    };

    std::vector<error_entry>& errors;
    std::filesystem::path entry_file;
    std::map<std::string, module> modules;
    std::map<std::string, package> packages;
    std::map<std::string, std::string> selected_versions;
    std::map<std::string, resolved_module> resolved_modules;
    std::vector<std::string> output_order;
    std::set<std::string> hoisted_headers;
    bool needs_fmt = false;

    auto report(std::string message) -> void {
        errors.emplace_back(source_position(-1, -1), "go2 module: " + std::move(message));
    }

    static auto starts_with_keyword(std::string_view text, std::string_view keyword) -> bool {
        return text.starts_with(keyword)
            && (text.size() == keyword.size() || std::isspace(static_cast<unsigned char>(text[keyword.size()])));
    }

    static auto is_identifier(std::string_view text) -> bool {
        if (text.empty() || !(std::isalpha(static_cast<unsigned char>(text.front())) || text.front() == '_')) {
            return false;
        }
        return std::all_of(text.begin() + 1, text.end(), [](unsigned char ch) {
            return std::isalnum(ch) || ch == '_';
        });
    }

    static auto filename_ends_with(std::filesystem::path const& path, std::string_view suffix) -> bool {
        auto const name = path.filename().string();
        return name.size() >= suffix.size() && name.ends_with(suffix);
    }

    auto package_key(std::filesystem::path const& path) -> std::string {
        auto error = std::error_code{};
        auto absolute = std::filesystem::absolute(path, error);
        if (error) {
            return path.lexically_normal().generic_string();
        }
        return absolute.lexically_normal().generic_string();
    }

    static auto tokens(std::string_view text) -> std::vector<std::string> {
        auto result = std::vector<std::string>{};
        while (!text.empty()) {
            text = go2_trim(text);
            if (text.empty()) {
                break;
            }
            auto end = text.find_first_of(" \t");
            result.emplace_back(text.substr(0, end));
            text = end == std::string_view::npos ? std::string_view{} : text.substr(end + 1);
        }
        return result;
    }

    auto manifest_in(std::filesystem::path const& directory) -> std::filesystem::path {
        auto error = std::error_code{};
        auto go2_manifest = directory / "go2.mod";
        if (std::filesystem::is_regular_file(go2_manifest, error) && !error) {
            return go2_manifest;
        }
        error.clear();
        auto go_manifest = directory / "go.mod";
        if (std::filesystem::is_regular_file(go_manifest, error) && !error) {
            return go_manifest;
        }
        return {};
    }

    static auto source_declares_package(std::filesystem::path const& path) -> bool {
        auto input = std::ifstream{path};
        auto line = std::string{};
        while (std::getline(input, line)) {
            auto const code = go2_trim(line);
            if (!code.empty() && !code.starts_with("//") && starts_with_keyword(code, "package")) {
                return true;
            }
        }
        return false;
    }

    static auto is_go2_package_source(std::filesystem::path const& path) -> bool {
        auto const extension = path.extension().string();
        return extension == ".go2"
            || extension == ".go"
            || extension == ".cpp"
            || extension == ".cc"
            || extension == ".cxx"
            || extension == ".c++";
    }

    auto add_requirement(module& current, std::string path, std::string version) -> bool {
        if (path.empty() || version.empty()) {
            report("invalid require directive in " + current.manifest.string());
            return false;
        }
        if (auto existing = current.requirements.find(path); existing != current.requirements.end() && existing->second != version) {
            report("module " + current.name + " requires conflicting versions of " + path);
            return false;
        }
        current.requirements[std::move(path)] = std::move(version);
        return true;
    }

    auto add_replacement(
        module& current,
        std::string path,
        std::string version,
        std::filesystem::path directory
    ) -> bool {
        if (path.empty() || directory.empty()) {
            report("invalid replace directive in " + current.manifest.string());
            return false;
        }
        if (directory.is_relative()) {
            directory = current.root / directory;
        }
        directory = directory.lexically_normal();
        auto const key = std::make_pair(path, version);
        if (auto existing = current.replacements.find(key); existing != current.replacements.end() && existing->second != directory) {
            report("module " + current.name + " replaces " + path + (version.empty() ? "" : " " + version) + " with multiple directories");
            return false;
        }
        current.replacements.emplace(std::move(key), std::move(directory));
        return true;
    }

    auto add_exclusion(module& current, std::string path, std::string version) -> bool {
        if (path.empty() || version.empty()) {
            report("invalid exclude directive in " + current.manifest.string());
            return false;
        }
        current.exclusions[std::move(path)].insert(std::move(version));
        return true;
    }

    static auto relative_path_for_lock(
        std::filesystem::path const& base,
        std::filesystem::path const& path
    ) -> std::string {
        auto error = std::error_code{};
        auto relative = std::filesystem::relative(path, base, error);
        if (!error && !relative.empty()) {
            return relative.generic_string();
        }
        return path.generic_string();
    }

    static auto lock_quote(std::string_view value) -> std::string {
        auto result = std::string{"\""};
        for (auto const ch : value) {
            if (ch == '\\' || ch == '\"') {
                result += '\\';
            }
            result += ch;
        }
        result += '\"';
        return result;
    }

    auto parse_manifest(std::filesystem::path const& manifest, module& current) -> bool {
        auto input = std::ifstream{manifest};
        if (!input.is_open()) {
            report("could not read module manifest " + manifest.string());
            return false;
        }

        enum class directive_block { none, require, replace, exclude, retract };
        auto block = directive_block::none;
        auto line = std::string{};
        while (std::getline(input, line)) {
            auto code = go2_trim(line);
            if (auto comment = code.find("//"); comment != std::string_view::npos) {
                code = go2_trim(code.substr(0, comment));
            }
            if (code.empty()) {
                continue;
            }
            if (code == ")") {
                if (block == directive_block::none) {
                    report("unexpected ')' in " + manifest.string());
                    return false;
                }
                block = directive_block::none;
                continue;
            }

            auto parse_require = [&](std::string_view text) {
                auto values = tokens(text);
                return values.size() == 2 && add_requirement(current, std::move(values[0]), std::move(values[1]));
            };
            auto parse_replace = [&](std::string_view text) {
                auto values = tokens(text);
                auto arrow = std::find(values.begin(), values.end(), "=>");
                if (arrow == values.end() || arrow == values.begin() || arrow + 1 == values.end()
                    || arrow + 2 != values.end() || std::distance(values.begin(), arrow) > 2)
                {
                    report("invalid local replace directive in " + manifest.string());
                    return false;
                }
                auto version = std::string{};
                if (std::distance(values.begin(), arrow) == 2) {
                    version = values[1];
                }
                return add_replacement(
                    current,
                    std::move(values.front()),
                    std::move(version),
                    std::filesystem::path{*(arrow + 1)}
                );
            };
            auto parse_exclude = [&](std::string_view text) {
                auto values = tokens(text);
                return values.size() == 2 && add_exclusion(current, std::move(values[0]), std::move(values[1]));
            };
            auto parse_retract = [&](std::string_view text) {
                return !go2_trim(text).empty();
            };

            if (block != directive_block::none) {
                auto valid = false;
                switch (block) {
                break; case directive_block::require: valid = parse_require(code);
                break; case directive_block::replace: valid = parse_replace(code);
                break; case directive_block::exclude: valid = parse_exclude(code);
                break; case directive_block::retract: valid = parse_retract(code);
                break; case directive_block::none: assert(false);
                }
                if (!valid) {
                    report("invalid directive in " + manifest.string());
                    return false;
                }
                continue;
            }

            if (starts_with_keyword(code, "module")) {
                if (!current.name.empty()) {
                    report("duplicate module declaration in " + manifest.string());
                    return false;
                }
                auto values = tokens(code.substr(6));
                if (values.size() != 1 || values.front().empty()) {
                    report("invalid module declaration in " + manifest.string());
                    return false;
                }
                current.name = std::move(values.front());
                continue;
            }
            if (starts_with_keyword(code, "go")) {
                auto values = tokens(code.substr(2));
                if (values.size() != 1 || !current.go_version.empty()) {
                    report("invalid go directive in " + manifest.string());
                    return false;
                }
                current.go_version = std::move(values.front());
                continue;
            }
            if (starts_with_keyword(code, "toolchain")) {
                auto values = tokens(code.substr(9));
                if (values.size() != 1 || !current.toolchain.empty()) {
                    report("invalid toolchain directive in " + manifest.string());
                    return false;
                }
                current.toolchain = std::move(values.front());
                continue;
            }
            auto parse_block_or_item = [&](std::string_view keyword, directive_block kind, auto&& parse_item) {
                if (!starts_with_keyword(code, keyword)) {
                    return false;
                }
                auto text = go2_trim(code.substr(keyword.size()));
                if (text == "(") {
                    block = kind;
                    return true;
                }
                return parse_item(text);
            };
            if (starts_with_keyword(code, "require")) {
                if (!parse_block_or_item("require", directive_block::require, parse_require)) {
                    report("invalid require directive in " + manifest.string());
                    return false;
                }
                continue;
            }
            if (starts_with_keyword(code, "replace")) {
                if (!parse_block_or_item("replace", directive_block::replace, parse_replace)) {
                    return false;
                }
                continue;
            }
            if (starts_with_keyword(code, "exclude")) {
                if (!parse_block_or_item("exclude", directive_block::exclude, parse_exclude)) {
                    report("invalid exclude directive in " + manifest.string());
                    return false;
                }
                continue;
            }
            if (starts_with_keyword(code, "retract")) {
                if (!parse_block_or_item("retract", directive_block::retract, parse_retract)) {
                    report("invalid retract directive in " + manifest.string());
                    return false;
                }
                continue;
            }

            report("unsupported directive in " + manifest.string() + ": " + std::string{code});
            return false;
        }

        if (block != directive_block::none) {
            report("unterminated directive block in " + manifest.string());
            return false;
        }
        if (current.name.empty()) {
            report("missing module declaration in " + manifest.string());
            return false;
        }
        return true;
    }

    auto load_module(std::filesystem::path const& directory, std::string_view expected_name = {}) -> module* {
        auto manifest = manifest_in(directory);
        if (manifest.empty()) {
            report("could not find go2.mod or go.mod in module directory " + directory.string());
            return nullptr;
        }

        auto error = std::error_code{};
        auto root = std::filesystem::absolute(manifest.parent_path(), error).lexically_normal();
        if (error) {
            root = manifest.parent_path().lexically_normal();
        }
        auto key = package_key(root);
        if (auto found = modules.find(key); found != modules.end()) {
            if (!expected_name.empty() && found->second.name != expected_name) {
                report("module at " + root.string() + " declares '" + found->second.name + "', expected '" + std::string{expected_name} + "'");
                return nullptr;
            }
            return &found->second;
        }

        auto [position, inserted] = modules.emplace(key, module{});
        assert(inserted);
        auto& current = position->second;
        current.root = std::move(root);
        current.manifest = std::move(manifest);
        if (!parse_manifest(current.manifest, current)) {
            return nullptr;
        }
        if (!expected_name.empty() && current.name != expected_name) {
            report("module at " + current.root.string() + " declares '" + current.name + "', expected '" + std::string{expected_name} + "'");
            return nullptr;
        }
        return &current;
    }

    static auto cpp_namespace_for(std::string_view import_path) -> std::string {
        static constexpr auto hex = "0123456789abcdef";
        auto result = std::string{"go2pkg_"};
        for (auto const ch : import_path) {
            auto const value = static_cast<unsigned char>(ch);
            if (std::isalnum(value)) {
                result += ch;
            }
            else {
                result += '_';
                result += hex[value >> 4];
                result += hex[value & 0x0f];
                result += '_';
            }
        }
        return result;
    }

    static auto is_module_prefix(std::string_view import_path, std::string_view module_path) -> bool {
        return import_path == module_path
            || (import_path.starts_with(module_path) && import_path.size() > module_path.size() && import_path[module_path.size()] == '/');
    }

    static auto compare_module_versions(std::string_view left, std::string_view right) -> int {
        auto trim_prefix = [](std::string_view value) {
            if (value.starts_with('v')) {
                value.remove_prefix(1);
            }
            return value;
        };
        left = trim_prefix(left);
        right = trim_prefix(right);

        auto split = [](std::string_view value) {
            auto build = value.find('+');
            if (build != std::string_view::npos) {
                value = value.substr(0, build);
            }
            auto prerelease = value.find('-');
            return std::pair{
                prerelease == std::string_view::npos ? value : value.substr(0, prerelease),
                prerelease == std::string_view::npos ? std::string_view{} : value.substr(prerelease + 1)
            };
        };
        auto const [left_core, left_prerelease] = split(left);
        auto const [right_core, right_prerelease] = split(right);

        auto next_number = [](std::string_view& value, unsigned long long& result) {
            auto dot = value.find('.');
            auto part = value.substr(0, dot);
            value = dot == std::string_view::npos ? std::string_view{} : value.substr(dot + 1);
            if (part.empty() || !std::all_of(part.begin(), part.end(), [](unsigned char ch) { return std::isdigit(ch); })) {
                return false;
            }
            result = 0;
            for (auto const ch : part) {
                result = result * 10 + static_cast<unsigned long long>(ch - '0');
            }
            return true;
        };

        auto left_numbers = left_core;
        auto right_numbers = right_core;
        for (;;) {
            if (left_numbers.empty() && right_numbers.empty()) {
                break;
            }
            auto left_value = 0ULL;
            auto right_value = 0ULL;
            if ((!left_numbers.empty() && !next_number(left_numbers, left_value))
                || (!right_numbers.empty() && !next_number(right_numbers, right_value)))
            {
                return left < right ? -1 : left > right ? 1 : 0;
            }
            if (left_value != right_value) {
                return left_value < right_value ? -1 : 1;
            }
        }

        if (left_prerelease.empty() != right_prerelease.empty()) {
            return left_prerelease.empty() ? 1 : -1;
        }
        return left_prerelease < right_prerelease ? -1 : left_prerelease > right_prerelease ? 1 : 0;
    }

    static auto select_highest_version(
        std::map<std::string, std::string>& versions,
        std::string const& module_path,
        std::string const& version
    ) -> void {
        if (auto existing = versions.find(module_path); existing == versions.end()) {
            versions.emplace(module_path, version);
        }
        else if (compare_module_versions(existing->second, version) < 0) {
            existing->second = version;
        }
    }

    auto add_graph_requirement(
        module const& root,
        std::map<std::string, std::string>& versions,
        std::string const& module_path,
        std::string const& version
    ) -> bool {
        if (module_path == root.name) {
            report("module " + root.name + " cannot require itself");
            return false;
        }
        if (auto excluded = root.exclusions.find(module_path); excluded != root.exclusions.end()
            && excluded->second.contains(version))
        {
            report("module " + module_path + " " + version + " is excluded by " + root.manifest.string());
            return false;
        }
        select_highest_version(versions, module_path, version);
        return true;
    }

    auto local_module_source(
        module const& root,
        std::string const& module_path,
        std::string const& version,
        std::filesystem::path& directory,
        std::string& source
    ) -> bool {
        auto replacement = root.replacements.find({module_path, version});
        if (replacement == root.replacements.end()) {
            replacement = root.replacements.find({module_path, {}});
        }
        if (replacement != root.replacements.end()) {
            directory = replacement->second;
            source = "replace:" + relative_path_for_lock(root.root, directory);
            return true;
        }

        directory = root.root / "vendor";
        auto remainder = std::string_view{module_path};
        while (!remainder.empty()) {
            auto slash = remainder.find('/');
            directory /= std::string{remainder.substr(0, slash)};
            remainder = slash == std::string_view::npos ? std::string_view{} : remainder.substr(slash + 1);
        }
        auto error = std::error_code{};
        if (!std::filesystem::is_directory(directory, error) || error) {
            report("module '" + module_path + "' requires a local replace or vendored copy; remote downloads are disabled");
            return false;
        }
        source = "vendor";
        return true;
    }

    auto resolve_module_graph(module& root) -> bool {
        selected_versions.clear();
        resolved_modules.clear();

        for (auto iteration = 0; iteration != 256; ++iteration) {
            auto next_versions = std::map<std::string, std::string>{};
            for (auto const& [path, version] : root.requirements) {
                if (!add_graph_requirement(root, next_versions, path, version)) {
                    return false;
                }
            }
            for (auto const& [_, resolved] : resolved_modules) {
                for (auto const& [path, version] : resolved.owner->requirements) {
                    if (!add_graph_requirement(root, next_versions, path, version)) {
                        return false;
                    }
                }
            }

            auto next_modules = std::map<std::string, resolved_module>{};
            for (auto const& [path, version] : next_versions) {
                auto directory = std::filesystem::path{};
                auto source = std::string{};
                if (!local_module_source(root, path, version, directory, source)) {
                    return false;
                }
                auto owner = load_module(directory, path);
                if (!owner) {
                    return false;
                }
                next_modules.emplace(path, resolved_module{owner, version, std::move(source)});
            }

            auto stable = selected_versions == next_versions && resolved_modules.size() == next_modules.size();
            if (stable) {
                for (auto const& [path, resolved] : next_modules) {
                    auto previous = resolved_modules.find(path);
                    if (previous == resolved_modules.end()
                        || previous->second.owner != resolved.owner
                        || previous->second.version != resolved.version
                        || previous->second.source != resolved.source)
                    {
                        stable = false;
                        break;
                    }
                }
            }
            selected_versions = std::move(next_versions);
            resolved_modules = std::move(next_modules);
            if (stable) {
                return true;
            }
        }

        report("module version resolution did not converge");
        return false;
    }

    struct resolved_import {
        module* owner = nullptr;
        std::filesystem::path directory;
    };

    auto resolve_import(package const& current, std::string_view import_path) -> resolved_import {
        auto& owner = modules.at(current.module_key);
        auto dependency_path = std::string{};
        auto dependency_module = static_cast<module*>(nullptr);

        if (is_module_prefix(import_path, owner.name)) {
            dependency_path = owner.name;
            dependency_module = &owner;
        }
        for (auto const& [path, resolved] : resolved_modules) {
            if (is_module_prefix(import_path, path)
                && (!dependency_module || path.size() > dependency_path.size()))
            {
                dependency_path = path;
                dependency_module = resolved.owner;
            }
        }
        if (!dependency_module) {
            report("import '" + std::string{import_path} + "' is outside the resolved module graph");
            return {};
        }

        auto relative = import_path.substr(dependency_path.size());
        auto directory = dependency_module->root;
        while (!relative.empty()) {
            if (relative.front() == '/') {
                relative.remove_prefix(1);
            }
            auto slash = relative.find('/');
            auto part = relative.substr(0, slash);
            if (part.empty() || part == "." || part == "..") {
                report("invalid import path '" + std::string{import_path} + "'");
                return {};
            }
            directory /= std::string{part};
            relative = slash == std::string_view::npos ? std::string_view{} : relative.substr(slash + 1);
        }
        return {dependency_module, std::move(directory)};
    }

    static auto fnv1a_append(std::uint64_t& value, std::string_view text) -> void {
        for (auto const ch : text) {
            value ^= static_cast<unsigned char>(ch);
            value *= 1099511628211ull;
        }
    }

    auto module_fingerprint(module const& current, std::string& fingerprint) -> bool {
        auto files = std::vector<std::filesystem::path>{current.manifest};
        auto error = std::error_code{};
        auto iterator = std::filesystem::recursive_directory_iterator{
            current.root,
            std::filesystem::directory_options::skip_permission_denied,
            error
        };
        auto const end = std::filesystem::recursive_directory_iterator{};
        while (!error && iterator != end) {
            auto const path = iterator->path();
            auto const status = iterator->status(error);
            if (error) {
                break;
            }
            if (std::filesystem::is_directory(status) && path.filename() == "vendor") {
                iterator.disable_recursion_pending();
            }
            else if (std::filesystem::is_regular_file(status) && is_go2_package_source(path)
                && (path.extension() == ".go2" || source_declares_package(path)))
            {
                files.push_back(path);
            }
            iterator.increment(error);
        }
        if (error) {
            report("could not enumerate module sources in " + current.root.string());
            return false;
        }

        std::sort(files.begin(), files.end(), [&](auto const& left, auto const& right) {
            return relative_path_for_lock(current.root, left) < relative_path_for_lock(current.root, right);
        });
        auto value = std::uint64_t{14695981039346656037ull};
        for (auto const& path : files) {
            auto input = std::ifstream{path, std::ios::binary};
            if (!input.is_open()) {
                report("could not read module source while creating go2.lock: " + path.string());
                return false;
            }
            fnv1a_append(value, relative_path_for_lock(current.root, path));
            fnv1a_append(value, "\n");
            auto ch = char{};
            while (input.get(ch)) {
                if (ch == '\r') {
                    if (input.peek() == '\n') {
                        continue;
                    }
                    ch = '\n';
                }
                value ^= static_cast<unsigned char>(ch);
                value *= 1099511628211ull;
            }
            if (!input.eof()) {
                report("could not finish reading module source while creating go2.lock: " + path.string());
                return false;
            }
            fnv1a_append(value, "\n");
        }

        auto output = std::ostringstream{};
        output << "fnv1a64:" << std::hex << std::setfill('0') << std::setw(16) << value;
        fingerprint = std::move(output).str();
        return true;
    }

    auto write_lock(module const& root) -> bool {
        auto output = std::ostringstream{};
        output << "# This file is generated by cppfront. Do not edit.\n";
        output << "format = 1\n";

        auto append_module = [&](module const& current, std::string_view version, std::string_view source) {
            auto fingerprint = std::string{};
            if (!module_fingerprint(current, fingerprint)) {
                return false;
            }
            output << "\n[[module]]\n";
            output << "path = " << lock_quote(current.name) << "\n";
            output << "version = " << lock_quote(version) << "\n";
            output << "source = " << lock_quote(source) << "\n";
            output << "manifest = " << lock_quote(relative_path_for_lock(root.root, current.manifest)) << "\n";
            output << "fingerprint = " << lock_quote(fingerprint) << "\n";
            return true;
        };

        if (!append_module(root, "workspace", "workspace")) {
            return false;
        }
        for (auto const& [_, resolved] : resolved_modules) {
            if (!append_module(*resolved.owner, resolved.version, resolved.source)) {
                return false;
            }
        }

        auto const lock_path = root.root / "go2.lock";
        auto const text = std::move(output).str();
        auto existing = std::ifstream{lock_path, std::ios::binary};
        if (existing.is_open()) {
            auto previous = std::ostringstream{};
            previous << existing.rdbuf();
            if (previous.str() == text) {
                return true;
            }
        }

        auto lock = std::ofstream{lock_path, std::ios::binary | std::ios::trunc};
        if (!lock.is_open()) {
            report("could not write lock file " + lock_path.string());
            return false;
        }
        lock << text;
        if (!lock) {
            report("could not finish writing lock file " + lock_path.string());
            return false;
        }
        return true;
    }

    auto parse_import(
        std::string_view text,
        source_file& file,
        std::size_t line
    ) -> bool {
        text = go2_trim(text);
        auto alias = std::string{};
        if (!text.starts_with('"')) {
            auto space = text.find_first_of(" \t");
            if (space == std::string_view::npos) {
                report(file.path.string() + ": malformed import on line " + std::to_string(line + 1));
                return false;
            }
            alias = std::string{go2_trim(text.substr(0, space))};
            text = go2_trim(text.substr(space + 1));
        }

        if (!text.starts_with('"')) {
            report(file.path.string() + ": import path must be quoted on line " + std::to_string(line + 1));
            return false;
        }
        auto close = text.find('"', 1);
        if (close == std::string_view::npos) {
            report(file.path.string() + ": unterminated import path on line " + std::to_string(line + 1));
            return false;
        }
        auto tail = go2_trim(text.substr(close + 1));
        if (!tail.empty() && !tail.starts_with("//")) {
            report(file.path.string() + ": unexpected text after import path on line " + std::to_string(line + 1));
            return false;
        }

        file.imports.push_back({std::string{text.substr(1, close - 1)}, std::move(alias)});
        return true;
    }

    auto read_source_file(std::filesystem::path const& path, source_file& file) -> bool {
        file.path = path;
        auto input = std::ifstream{path};
        if (!input.is_open()) {
            report("could not read package source " + path.string());
            return false;
        }

        auto line = std::string{};
        while (std::getline(input, line)) {
            file.lines.push_back(line);
        }
        file.skipped.resize(file.lines.size());
        file.cpp1_passthrough.resize(file.lines.size());

        auto in_import_group = false;
        for (auto index = std::size_t{0}; index < file.lines.size(); ++index) {
            auto code = go2_trim(file.lines[index]);
            if (code.empty() || code.starts_with("//")) {
                continue;
            }

            if (in_import_group) {
                file.skipped[index] = true;
                if (code == ")") {
                    in_import_group = false;
                    continue;
                }
                if (!parse_import(code, file, index)) {
                    return false;
                }
                continue;
            }

            if (starts_with_keyword(code, "package")) {
                if (!file.package_name.empty()) {
                    report(path.string() + ": duplicate package declaration");
                    return false;
                }
                auto name = go2_trim(code.substr(7));
                auto end = name.find_first_of(" \t/");
                if (end != std::string_view::npos && name.substr(end).starts_with("//")) {
                    name = name.substr(0, end);
                }
                if (!is_identifier(name)) {
                    report(path.string() + ": invalid package name");
                    return false;
                }
                file.package_name = std::string{name};
                file.skipped[index] = true;
                continue;
            }

            if (starts_with_keyword(code, "import")) {
                auto imports = go2_trim(code.substr(6));
                file.skipped[index] = true;
                if (imports == "(") {
                    in_import_group = true;
                    continue;
                }
                if (!parse_import(imports, file, index)) {
                    return false;
                }
            }
        }

        if (in_import_group) {
            report(path.string() + ": unterminated import group");
            return false;
        }
        if (file.package_name.empty()) {
            report(path.string() + ": missing package declaration");
            return false;
        }
        return true;
    }

    auto add_binding(package& current, import_binding binding) -> bool {
        for (auto const& existing : current.bindings) {
            if (existing.alias == binding.alias) {
                if (existing.target != binding.target || existing.standard != binding.standard) {
                    report(current.directory.string() + ": import alias '" + binding.alias + "' refers to multiple packages");
                    return false;
                }
                return true;
            }
        }
        current.bindings.push_back(std::move(binding));
        return true;
    }

    auto load_package(module& owner, std::filesystem::path const& directory, bool is_entry = false) -> package* {
        auto key = package_key(directory);
        if (auto found = packages.find(key); found != packages.end()) {
            if (found->second.module_key != package_key(owner.root)) {
                report("package directory " + directory.string() + " belongs to multiple modules");
                return nullptr;
            }
            if (found->second.state == package_state::loading) {
                report("cyclic package import involving " + found->second.directory.string());
                return nullptr;
            }
            return &found->second;
        }

        auto error = std::error_code{};
        if (!std::filesystem::is_directory(directory, error) || error) {
            report("could not resolve package directory " + directory.string());
            return nullptr;
        }

        auto [position, inserted] = packages.emplace(key, package{});
        assert(inserted);
        auto& current = position->second;
        current.entry = is_entry;
        current.module_key = package_key(owner.root);
        current.directory = std::filesystem::absolute(directory, error).lexically_normal();
        if (error) {
            current.directory = directory.lexically_normal();
        }
        auto relative = std::filesystem::relative(current.directory, owner.root, error);
        if (error || relative.empty() || relative.generic_string().starts_with("..")) {
            report("package directory " + current.directory.string() + " is outside module " + owner.root.string());
            return nullptr;
        }
        current.import_path = owner.name;
        if (relative != ".") {
            current.import_path += "/" + relative.generic_string();
        }
        current.cpp_namespace = cpp_namespace_for(current.import_path);

        auto paths = std::vector<std::filesystem::path>{};
        for (auto iterator = std::filesystem::directory_iterator{current.directory, error}; !error && iterator != std::filesystem::directory_iterator{}; iterator.increment(error)) {
            auto const& path = iterator->path();
            if (iterator->is_regular_file(error) && is_go2_package_source(path) && !filename_ends_with(path, "_test.go2")
                && (path.extension() == ".go2" || source_declares_package(path)))
            {
                paths.push_back(path);
            }
        }
        if (error) {
            report("could not enumerate package directory " + current.directory.string());
            return nullptr;
        }
        std::sort(paths.begin(), paths.end());
        if (paths.empty()) {
            report("package directory " + current.directory.string() + " contains no .go2 source files");
            return nullptr;
        }

        for (auto const& path : paths) {
            auto file = source_file{};
            if (!read_source_file(path, file)) {
                return nullptr;
            }
            if (current.name.empty()) {
                current.name = file.package_name;
            }
            else if (current.name != file.package_name) {
                report(current.directory.string() + ": files in one package must use the same package name");
                return nullptr;
            }
            current.files.push_back(std::move(file));
        }

        for (auto& file : current.files) {
            auto in_cpp1 = false;
            for (auto index = std::size_t{0}; index < file.lines.size(); ++index) {
                auto const code = go2_trim(file.lines[index]);
                if (!current.entry && code == "//go2:cpp1-begin") {
                    if (in_cpp1) {
                        report(file.path.string() + ": nested Cpp1 passthrough region");
                        return nullptr;
                    }
                    in_cpp1 = true;
                    file.skipped[index] = true;
                    continue;
                }
                if (!current.entry && code == "//go2:cpp1-end") {
                    if (!in_cpp1) {
                        report(file.path.string() + ": Cpp1 passthrough region ends before it begins");
                        return nullptr;
                    }
                    in_cpp1 = false;
                    file.skipped[index] = true;
                    continue;
                }
                if (!current.entry && in_cpp1) {
                    file.skipped[index] = true;
                    if (code.starts_with("#")) {
                        hoisted_headers.insert(file.lines[index]);
                    }
                    else {
                        file.cpp1_passthrough[index] = true;
                    }
                    continue;
                }
                if (!current.entry && code.starts_with("#")) {
                    hoisted_headers.insert(file.lines[index]);
                    file.skipped[index] = true;
                }
            }
            if (!current.entry && in_cpp1) {
                report(file.path.string() + ": unterminated Cpp1 passthrough region");
                return nullptr;
            }

            for (auto const& imported : file.imports) {
                auto alias = imported.alias;
                if (imported.path == "fmt") {
                    if (alias.empty()) {
                        alias = "fmt";
                    }
                    if (!is_identifier(alias)) {
                        report(file.path.string() + ": unsupported import alias '" + alias + "' for fmt");
                        return nullptr;
                    }
                    needs_fmt = true;
                    if (!add_binding(current, {std::move(alias), "fmt", true})) {
                        return nullptr;
                    }
                    continue;
                }

                auto resolved = resolve_import(current, imported.path);
                if (!resolved.owner) {
                    return nullptr;
                }
                auto dependency = load_package(*resolved.owner, resolved.directory);
                if (!dependency) {
                    return nullptr;
                }
                if (dependency->name == "main") {
                    report("package '" + imported.path + "' cannot be imported because it declares package main");
                    return nullptr;
                }
                if (alias.empty()) {
                    alias = dependency->name;
                }
                if (!is_identifier(alias)) {
                    report(file.path.string() + ": unsupported import alias '" + alias + "'");
                    return nullptr;
                }
                if (!add_binding(current, {std::move(alias), dependency->cpp_namespace, false})) {
                    return nullptr;
                }
            }
        }

        current.state = package_state::loaded;
        output_order.push_back(key);
        return &current;
    }

    static auto rewrite_import_selectors(std::string_view line, package const& current) -> std::string {
        auto bindings = std::unordered_map<std::string, import_binding const*>{};
        for (auto const& binding : current.bindings) {
            bindings.emplace(binding.alias, &binding);
        }

        auto output = std::string{};
        auto quote = char{};
        auto escaped = false;
        for (auto index = std::size_t{0}; index < line.size();) {
            auto const ch = line[index];
            if (quote != '\0') {
                output += ch;
                if (escaped) {
                    escaped = false;
                }
                else if (ch == '\\') {
                    escaped = true;
                }
                else if (ch == quote) {
                    quote = '\0';
                }
                ++index;
                continue;
            }
            if (ch == '/' && index + 1 < line.size() && line[index + 1] == '/') {
                output += line.substr(index);
                break;
            }
            if (ch == '\'' || ch == '"' || ch == '`') {
                quote = ch;
                output += ch;
                ++index;
                continue;
            }
            if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '_') {
                auto end = index + 1;
                while (end < line.size() && (std::isalnum(static_cast<unsigned char>(line[end])) || line[end] == '_')) {
                    ++end;
                }
                auto name = line.substr(index, end - index);
                if (end < line.size() && line[end] == '.') {
                    if (auto binding = bindings.find(std::string{name}); binding != bindings.end()) {
                        output += binding->second->standard ? binding->second->target + "." : std::string{name} + "::";
                        index = end + 1;
                        continue;
                    }
                }
                output += name;
                index = end;
                continue;
            }
            output += ch;
            ++index;
        }
        return output;
    }

    static auto append_normalized_source(std::ostringstream& output, package const& current) -> void {
        for (auto const& file : current.files) {
            auto normalizer = go2_normalizer{};
            for (auto index = std::size_t{0}; index < file.lines.size(); ++index) {
                if (file.skipped[index]) {
                    output << '\n';
                    continue;
                }
                auto translated = normalizer.normalize(rewrite_import_selectors(file.lines[index], current));
                if (translated.kind != go2_line_kind::ignored) {
                    output << translated.text;
                }
                output << '\n';
            }
        }
    }

    static auto append_aliases(std::ostringstream& output, package const& current) -> void {
        for (auto const& binding : current.bindings) {
            if (!binding.standard && binding.alias != binding.target) {
                output << binding.alias << ": namespace == " << binding.target << ";\n";
            }
        }
    }

    static auto append_cpp1_passthrough(std::ostringstream& output, package const& current) -> void {
        auto has_cpp1 = false;
        for (auto const& file : current.files) {
            has_cpp1 = has_cpp1 || std::any_of(file.cpp1_passthrough.begin(), file.cpp1_passthrough.end(), [](bool line) { return line; });
        }
        if (!has_cpp1) {
            return;
        }

        output << "namespace " << current.cpp_namespace << " {\n";
        for (auto const& file : current.files) {
            for (auto index = std::size_t{0}; index < file.lines.size(); ++index) {
                if (file.cpp1_passthrough[index]) {
                    output << file.lines[index] << '\n';
                }
            }
        }
        output << "}\n";
    }

    static auto append_package(std::ostringstream& output, package const& current, bool wrap_in_namespace) -> void {
        if (wrap_in_namespace) {
            output << current.cpp_namespace << ": namespace = {\n";
        }
        append_aliases(output, current);
        append_normalized_source(output, current);
        if (wrap_in_namespace) {
            output << "}\n";
        }
        append_cpp1_passthrough(output, current);
    }

public:
    explicit go2_module_loader(std::vector<error_entry>& errors_)
        : errors{errors_}
    { }

    auto load(std::string const& filename) -> go2_module_load_result {
        if (filename == "stdin") {
            return {};
        }

        auto error = std::error_code{};
        entry_file = std::filesystem::absolute(filename, error).lexically_normal();
        if (error) {
            return {};
        }

        auto manifest = std::filesystem::path{};
        for (auto directory = entry_file.parent_path(); !directory.empty();) {
            auto go2_manifest = directory / "go2.mod";
            auto go_manifest = directory / "go.mod";
            if (std::filesystem::is_regular_file(go2_manifest, error) && !error) {
                manifest = go2_manifest;
                break;
            }
            error.clear();
            if (std::filesystem::is_regular_file(go_manifest, error) && !error) {
                manifest = go_manifest;
                break;
            }
            error.clear();
            auto parent = directory.parent_path();
            if (parent == directory) {
                break;
            }
            directory = std::move(parent);
        }

        if (manifest.empty()) {
            return {};
        }
        if (!source_declares_package(entry_file)) {
            return {};
        }

        auto result = go2_module_load_result{};
        result.handled = true;
        auto root_module = load_module(manifest.parent_path());
        if (!root_module) {
            return result;
        }
        if (!resolve_module_graph(*root_module)) {
            return result;
        }

        auto root = load_package(*root_module, entry_file.parent_path(), true);
        if (!root) {
            return result;
        }
        if (!write_lock(*root_module)) {
            return result;
        }

        auto output = std::ostringstream{};
        output << "#include \"go2util.h\"\n#include <string>\n";
        if (needs_fmt) {
            output << "#include <iostream>\n";
        }
        for (auto const& header : hoisted_headers) {
            output << header << '\n';
        }

        for (auto const& key : output_order) {
            auto const& current = packages.at(key);
            append_package(output, current, !current.entry || current.name != "main");
        }

        result.success = true;
        result.text = std::move(output).str();
        return result;
    }
};

}

#endif
