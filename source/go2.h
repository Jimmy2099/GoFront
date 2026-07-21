//  Copyright 2026 Herb Sutter
//  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

//===========================================================================
//  Go2 surface syntax normalization
//===========================================================================

#ifndef CPP2_GO2_H
#define CPP2_GO2_H

namespace cpp2 {

enum class go2_line_kind : u8 { cpp2, preprocessor, ignored };

struct go2_line {
    std::string    text;
    go2_line_kind  kind = go2_line_kind::cpp2;
};

auto go2_trim(std::string_view text) -> std::string_view
{
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) {
        text.remove_prefix(1);
    }
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) {
        text.remove_suffix(1);
    }
    return text;
}

auto go2_indent(std::string_view text) -> std::string_view
{
    auto first = text.find_first_not_of(" \t");
    return first == std::string_view::npos ? std::string_view{} : text.substr(0, first);
}

auto go2_type(std::string_view type) -> std::string
{
    type = go2_trim(type);
    if (type.starts_with("*")) {
        return "* " + go2_type(type.substr(1));
    }
    if (type.starts_with("[]")) {
        return "std::vector<" + go2_type(type.substr(2)) + ">";
    }
    if (type.starts_with("map[")) {
        auto close = type.find(']');
        if (close != std::string_view::npos) {
            return "std::map<" + go2_type(type.substr(4, close - 4)) + ", " + go2_type(type.substr(close + 1)) + ">";
        }
    }
    if (type.starts_with("[")) {
        auto close = type.find(']');
        if (close != std::string_view::npos) {
            return "std::array<" + go2_type(type.substr(close + 1)) + ", " + std::string{type.substr(1, close - 1)} + ">";
        }
    }
    if (type == "string") { return "std::string"; }
    if (type == "byte")   { return "unsigned char"; }
    if (type == "rune")   { return "char32_t"; }
    return std::string{type};
}

auto go2_parameters(std::string_view text) -> std::string
{
    auto result = std::string{};
    auto first = true;

    while (!text.empty()) {
        auto comma = text.find(',');
        auto parameter = go2_trim(text.substr(0, comma));
        text = comma == std::string_view::npos ? std::string_view{} : text.substr(comma + 1);
        if (parameter.empty()) {
            continue;
        }

        auto space = parameter.find_first_of(" \t");
        if (space == std::string_view::npos) {
            // An unnamed parameter is valid Cpp2 and uses the same spelling.
            if (!first) { result += ", "; }
            result += go2_type(parameter);
            first = false;
            continue;
        }

        auto name = go2_trim(parameter.substr(0, space));
        auto type = go2_trim(parameter.substr(space + 1));
        if (!first) { result += ", "; }
        result += "copy " + std::string{name} + ": " + go2_type(type);
        first = false;
    }

    return result;
}

auto go2_println(std::string_view indent, std::string_view arguments) -> std::string
{
    auto result = std::string{indent} + "std::cout";
    auto first = true;
    while (!arguments.empty()) {
        auto comma = arguments.find(',');
        auto argument = go2_trim(arguments.substr(0, comma));
        arguments = comma == std::string_view::npos ? std::string_view{} : arguments.substr(comma + 1);
        if (argument.empty()) {
            continue;
        }
        result += " << ";
        if (!first) {
            result += "\" \" << ";
        }
        result += argument;
        first = false;
    }
    return result + " << '\\n';";
}

// This deliberately accepts only the Go constructs that lower without
// changing C++ ownership, concurrency, or runtime semantics.
class go2_normalizer
{
    bool in_cpp1 = false;
    int  cpp2_brace_depth = 0;
    int  cpp1_brace_depth = 0;
    bool in_go_struct = false;
    std::vector<std::string> pointer_names;

    auto add_pointer_parameters(std::string_view text) -> void
    {
        while (!text.empty()) {
            auto comma = text.find(',');
            auto parameter = go2_trim(text.substr(0, comma));
            text = comma == std::string_view::npos ? std::string_view{} : text.substr(comma + 1);
            auto space = parameter.find_first_of(" \t");
            if (space == std::string_view::npos) {
                continue;
            }
            auto name = go2_trim(parameter.substr(0, space));
            auto type = go2_trim(parameter.substr(space + 1));
            if (type.starts_with("*")) {
                pointer_names.emplace_back(name);
            }
        }
    }

    auto rewrite_pointer_expressions(std::string text) const -> std::string
    {
        for (auto const& name : pointer_names) {
            auto start = size_t{0};
            while ((start = text.find(name + ".", start)) != std::string::npos) {
                auto before_is_identifier = start > 0 && (std::isalnum(static_cast<unsigned char>(text[start - 1])) || text[start - 1] == '_');
                if (!before_is_identifier) {
                    text.replace(start, name.size() + 1, name + "*.");
                    start += name.size() + 2;
                }
                else {
                    start += name.size() + 1;
                }
            }
        }

        auto start = size_t{0};
        while ((start = text.find('&', start)) != std::string::npos) {
            auto end = start + 1;
            while (end < text.size() && (std::isalnum(static_cast<unsigned char>(text[end])) || text[end] == '_')) {
                ++end;
            }
            if (end > start + 1) {
                auto name = text.substr(start + 1, end - start - 1);
                text.replace(start, end - start, name + "&");
                start += name.size() + 1;
            }
            else {
                ++start;
            }
        }
        return text;
    }

    auto container_declaration(std::string_view name, std::string_view value, std::string_view indent) const -> std::optional<std::string>
    {
        auto open = value.find('{');
        if (open == std::string_view::npos || !value.ends_with("}")) {
            return {};
        }
        auto type = go2_type(value.substr(0, open));
        if (!type.starts_with("std::vector<") && !type.starts_with("std::array<") && !type.starts_with("std::map<")) {
            return {};
        }
        auto values = go2_trim(value.substr(open + 1, value.size() - open - 2));
        if (type.starts_with("std::map<") && !values.empty()) {
            return {};
        }
        return std::string{indent} + std::string{name} + ": " + type + " = (" + std::string{values} + ");";
    }

    auto is_cpp2_declaration(std::string_view code) const -> bool
    {
        if (code.starts_with("public "))    { code.remove_prefix(7); }
        if (code.starts_with("protected ")) { code.remove_prefix(10); }
        if (code.starts_with("private "))   { code.remove_prefix(8); }

        auto name_end = size_t{0};
        if (code.starts_with("operator")) {
            name_end = 8;
            while (name_end < code.size() && !std::isspace(static_cast<unsigned char>(code[name_end]))) {
                ++name_end;
            }
        }
        else {
            while (
                name_end < code.size()
                && (std::isalnum(static_cast<unsigned char>(code[name_end])) || code[name_end] == '_')
                )
            {
                ++name_end;
            }
        }
        if (name_end == 0) {
            return false;
        }

        while (name_end < code.size() && std::isspace(static_cast<unsigned char>(code[name_end]))) {
            ++name_end;
        }
        if (name_end == code.size() || code[name_end] != ':') {
            return false;
        }
        ++name_end;
        while (name_end < code.size() && std::isspace(static_cast<unsigned char>(code[name_end]))) {
            ++name_end;
        }

        // `:=` is also Go's short-declaration syntax and must be normalized.
        return name_end == code.size() || (code[name_end] != ':' && code[name_end] != '=');
    }

    auto update_cpp2_brace_depth(std::string_view line) -> void
    {
        for (auto ch : line) {
            if (ch == '{') { ++cpp2_brace_depth; }
            if (ch == '}') { --cpp2_brace_depth; }
        }
    }

    auto is_cpp1_block_start(std::string_view code) const -> bool
    {
        if (!code.ends_with("{")) {
            return false;
        }
        if (code.starts_with("struct ") || code.starts_with("class ") || code.starts_with("namespace ")) {
            return true;
        }
        if (code.find('(') == std::string_view::npos) {
            return false;
        }
        return !code.starts_with("if ")
            && !code.starts_with("for ")
            && !code.starts_with("while ")
            && !code.starts_with("switch ")
            && !code.starts_with("catch ")
            ;
    }

    auto update_cpp1_brace_depth(std::string_view line) -> void
    {
        for (auto ch : line) {
            if (ch == '{') { ++cpp1_brace_depth; }
            if (ch == '}') { --cpp1_brace_depth; }
        }
    }

public:
    auto normalize(std::string_view line) -> go2_line
    {
        auto const indent = go2_indent(line);
        auto const code = go2_trim(line);

        if (code == "//go2:cpp1-begin") {
            in_cpp1 = true;
            return {"", go2_line_kind::ignored};
        }
        if (code == "//go2:cpp1-end") {
            in_cpp1 = false;
            return {"", go2_line_kind::ignored};
        }
        if (in_cpp1) {
            return {std::string{line}, go2_line_kind::cpp2};
        }

        if (in_go_struct) {
            if (code == "}") {
                in_go_struct = false;
                return {std::string{indent} + "}", go2_line_kind::cpp2};
            }
            auto space = code.find_first_of(" \t");
            if (space != std::string_view::npos) {
                auto name = go2_trim(code.substr(0, space));
                auto type = go2_trim(code.substr(space + 1));
                return {std::string{indent} + "public " + std::string{name} + ": " + go2_type(type) + " = ();", go2_line_kind::cpp2};
            }
        }

        if (cpp1_brace_depth > 0) {
            update_cpp1_brace_depth(line);
            return {std::string{line}, go2_line_kind::cpp2};
        }

        if (cpp2_brace_depth > 0) {
            update_cpp2_brace_depth(line);
            return {std::string{line}, go2_line_kind::cpp2};
        }
        if (is_cpp2_declaration(code)) {
            update_cpp2_brace_depth(line);
            return {std::string{line}, go2_line_kind::cpp2};
        }

        if (code.empty() || code.starts_with("//") || code.starts_with("/*") || code.starts_with("*") || code.starts_with("#")) {
            return {std::string{line}, go2_line_kind::cpp2};
        }
        if (code.starts_with("package ")) {
            return {std::string{indent} + "#include <string>", go2_line_kind::preprocessor};
        }
        if (code == "import \"fmt\"") {
            return {std::string{indent} + "#include <iostream>", go2_line_kind::preprocessor};
        }

        if (code.starts_with("type ") && code.ends_with(" struct {")) {
            auto name = go2_trim(code.substr(5, code.size() - 13));
            in_go_struct = true;
            return {std::string{indent} + std::string{name} + ": @value type = {", go2_line_kind::cpp2};
        }

        if (code.starts_with("func ")) {
        auto signature = code.substr(5);
        auto open = signature.find('(');
        auto close = signature.find(')', open);
        auto brace = signature.rfind('{');
        if (open != std::string_view::npos && close != std::string_view::npos && brace != std::string_view::npos) {
            auto name = go2_trim(signature.substr(0, open));
            auto params = signature.substr(open + 1, close - open - 1);
            auto returns = go2_trim(signature.substr(close + 1, brace - close - 1));
            auto return_type = returns.empty() ? (name == "main" ? "int" : "void") : go2_type(returns);
            pointer_names.clear();
            add_pointer_parameters(params);
            return {
                std::string{indent} + std::string{name} + ": (" + go2_parameters(params) + ") -> " + return_type + " = {",
                go2_line_kind::cpp2
            };
        }
        }

        if (code.starts_with("for ")) {
        auto header = go2_trim(code.substr(4));
        if (header == "{") {
            return {std::string{indent} + "while true {", go2_line_kind::cpp2};
        }
        auto brace = header.rfind('{');
        auto first_semicolon = header.find(';');
        auto second_semicolon = first_semicolon == std::string_view::npos ? std::string_view::npos : header.find(';', first_semicolon + 1);
        if (brace != std::string_view::npos && second_semicolon != std::string_view::npos) {
            auto init = go2_trim(header.substr(0, first_semicolon));
            auto condition = go2_trim(header.substr(first_semicolon + 1, second_semicolon - first_semicolon - 1));
            auto next = go2_trim(header.substr(second_semicolon + 1, brace - second_semicolon - 1));
            auto colon_equal = init.find(":=");
            auto init_cpp2 = colon_equal == std::string_view::npos
                ? std::string{init}
                : std::string{go2_trim(init.substr(0, colon_equal))} + ": _ = " + std::string{go2_trim(init.substr(colon_equal + 2))};
            return {
                std::string{indent} + init_cpp2 + "; while " + std::string{condition} + " next " + std::string{next} + " {",
                go2_line_kind::cpp2
            };
        }
        if (brace != std::string_view::npos) {
            return {std::string{indent} + "while " + std::string{go2_trim(header.substr(0, brace))} + " {", go2_line_kind::cpp2};
        }
        }

        if (code.starts_with("var ")) {
        auto declaration = go2_trim(code.substr(4));
        auto equal = declaration.find('=');
        auto left = go2_trim(declaration.substr(0, equal));
        auto value = equal == std::string_view::npos ? std::string_view{} : go2_trim(declaration.substr(equal + 1));
        auto space = left.find_first_of(" \t");
        auto name = space == std::string_view::npos ? left : go2_trim(left.substr(0, space));
        auto type = space == std::string_view::npos ? std::string_view{"_"} : go2_trim(left.substr(space + 1));
        if (auto container = container_declaration(name, value, indent)) {
            return {*container, go2_line_kind::cpp2};
        }
        return {
            std::string{indent} + std::string{name} + ": " + go2_type(type) + (value.empty() ? ";" : " = " + rewrite_pointer_expressions(std::string{value}) + ";"),
            go2_line_kind::cpp2
        };
        }

        if (auto colon_equal = code.find(":="); colon_equal != std::string_view::npos) {
            auto name = go2_trim(code.substr(0, colon_equal));
            auto value = go2_trim(code.substr(colon_equal + 2));
            if (auto container = container_declaration(name, value, indent)) {
                return {*container, go2_line_kind::cpp2};
            }
            if (value.ends_with("{}") && value.find('{') != std::string_view::npos) {
                auto type = go2_trim(value.substr(0, value.size() - 2));
                return {std::string{indent} + std::string{name} + ": " + go2_type(type) + " = ();", go2_line_kind::cpp2};
            }
            return {
                std::string{indent} + std::string{name} + ": _ = " + rewrite_pointer_expressions(std::string{value}) + ";",
                go2_line_kind::cpp2
            };
        }

        if (code.starts_with("fmt.Println(") && code.ends_with(")")) {
            return {go2_println(indent, rewrite_pointer_expressions(std::string{code.substr(12, code.size() - 13)})), go2_line_kind::cpp2};
        }
        if (code.starts_with("return")) {
            return {std::string{indent} + rewrite_pointer_expressions(std::string{code}) + ";", go2_line_kind::cpp2};
        }
        if (is_cpp1_block_start(code)) {
            update_cpp1_brace_depth(line);
            return {std::string{line}, go2_line_kind::cpp2};
        }
        if (code == "}" || code == "} else {" || code.ends_with("{")) {
            if (code == "}") {
                pointer_names.clear();
            }
            return {std::string{line}, go2_line_kind::cpp2};
        }

        // Assignment and expression statements in Go are terminated by a newline.
        return {std::string{indent} + rewrite_pointer_expressions(std::string{code}) + ";", go2_line_kind::cpp2};
    }
};

}

#endif
