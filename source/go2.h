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

auto go2_type(std::string_view type) -> std::string;

auto go2_function_type_parameters(std::string_view text) -> std::string
{
    auto result = std::string{};
    auto index = 0;

    while (!text.empty()) {
        auto comma = text.find(',');
        auto parameter = go2_trim(text.substr(0, comma));
        text = comma == std::string_view::npos ? std::string_view{} : text.substr(comma + 1);
        if (parameter.empty()) {
            continue;
        }

        auto space = parameter.find_first_of(" \t");
        auto name = space == std::string_view::npos
            ? "go2_arg" + std::to_string(index)
            : std::string{go2_trim(parameter.substr(0, space))};
        auto type = space == std::string_view::npos
            ? parameter
            : go2_trim(parameter.substr(space + 1));
        if (!result.empty()) {
            result += ", ";
        }
        result += name + ": " + go2_type(type);
        ++index;
    }

    return result;
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
    if (type.starts_with("chan ")) {
        return "go2::channel<" + go2_type(type.substr(5)) + ">";
    }
    if (type.starts_with("[")) {
        auto close = type.find(']');
        if (close != std::string_view::npos) {
            return "std::array<" + go2_type(type.substr(close + 1)) + ", " + std::string{type.substr(1, close - 1)} + ">";
        }
    }
    if (type.starts_with("func(")) {
        auto close = type.find(')');
        if (close != std::string_view::npos) {
            auto result = go2_trim(type.substr(close + 1));
            return "std::function< (" + go2_function_type_parameters(type.substr(5, close - 5)) + ") -> "
                + (result.empty() ? "void" : go2_type(result)) + " >";
        }
    }
    if (type == "string") { return "std::string"; }
    if (type == "uint8" || type == "byte") { return "u8"; }
    if (type == "uint16") { return "u16"; }
    if (type == "uint32") { return "u32"; }
    if (type == "uint64") { return "u64"; }
    if (type == "int8")   { return "i8"; }
    if (type == "int16")  { return "i16"; }
    if (type == "int32")  { return "i32"; }
    if (type == "int64")  { return "i64"; }
    if (type == "float32") { return "float"; }
    if (type == "float64") { return "double"; }
    if (type == "complex64") { return "std::complex<float>"; }
    if (type == "complex128") { return "std::complex<double>"; }
    if (type == "uint") { return "go2::uint"; }
    if (type == "uintptr") { return "std::uintptr_t"; }
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
        result += (type.starts_with("chan ") ? "inout " : "copy ") + std::string{name} + ": " + go2_type(type);
        first = false;
    }

    return result;
}

auto go2_cpp1_type(std::string_view type) -> std::string
{
    type = go2_trim(type);
    if (type.starts_with("*")) {
        return go2_cpp1_type(type.substr(1)) + "*";
    }
    if (type.starts_with("[]")) {
        return "std::vector<" + go2_cpp1_type(type.substr(2)) + ">";
    }
    if (type.starts_with("map[")) {
        auto close = type.find(']');
        if (close != std::string_view::npos) {
            return "std::map<" + go2_cpp1_type(type.substr(4, close - 4)) + ", " + go2_cpp1_type(type.substr(close + 1)) + ">";
        }
    }
    if (type.starts_with("chan ")) {
        return "go2::channel<" + go2_cpp1_type(type.substr(5)) + ">";
    }
    if (type.starts_with("[")) {
        auto close = type.find(']');
        if (close != std::string_view::npos) {
            return "std::array<" + go2_cpp1_type(type.substr(close + 1)) + ", " + std::string{type.substr(1, close - 1)} + ">";
        }
    }
    return go2_type(type);
}

auto go2_cpp1_parameters(std::string_view text) -> std::string
{
    auto result = std::string{};
    auto first = true;
    while (!text.empty()) {
        auto comma = text.find(',');
        auto parameter = go2_trim(text.substr(0, comma));
        text = comma == std::string_view::npos ? std::string_view{} : text.substr(comma + 1);
        auto space = parameter.find_first_of(" \t");
        if (space == std::string_view::npos) {
            continue;
        }
        if (!first) { result += ", "; }
        result += go2_cpp1_type(go2_trim(parameter.substr(space + 1))) + " " + std::string{go2_trim(parameter.substr(0, space))};
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
    bool in_go_class = false;
    bool in_go_class_method = false;
    int  go_class_method_brace_depth = 0;
    bool in_go_interface = false;
    bool in_go_select = false;
    bool in_go_switch = false;
    bool go_switch_case_open = false;
    int  go_switch_number = 0;
    int  go_defer_number = 0;
    std::string go_switch_expression;
    std::vector<std::string> pointer_names;
    std::vector<std::string> go_class_pointer_names;
    bool buffer_functions = true;
    bool buffering_go_function = false;
    int  go_function_brace_depth = 0;
    std::vector<std::string> go_function_lines;

    auto go2_brace_delta(std::string_view line) const -> int
    {
        auto delta = 0;
        auto quote = char{};
        auto escaped = false;
        for (auto i = size_t{0}; i < line.size(); ++i) {
            auto const ch = line[i];
            if (quote != '\0') {
                if (escaped) {
                    escaped = false;
                }
                else if (ch == '\\') {
                    escaped = true;
                }
                else if (ch == quote) {
                    quote = '\0';
                }
                continue;
            }
            if (ch == '/' && i + 1 < line.size() && line[i + 1] == '/') {
                break;
            }
            if (ch == '\'' || ch == '\"') {
                quote = ch;
            }
            else if (ch == '{') {
                ++delta;
            }
            else if (ch == '}') {
                --delta;
            }
        }
        return delta;
    }

    auto go2_function_has_goto() const -> bool
    {
        return std::any_of(
            go_function_lines.begin(),
            go_function_lines.end(),
            [](std::string const& line) {
                return go2_trim(line).starts_with("goto ");
            }
        );
    }

    auto go2_cpp1_statement(std::string_view line) const -> std::string
    {
        auto const indent = go2_indent(line);
        auto const code = go2_trim(line);
        if (code.empty() || code.starts_with("//") || code == "}" || code == "} else {") {
            return std::string{line};
        }
        if (code.ends_with("{") || code.ends_with(":")) {
            return std::string{line};
        }
        if (code.starts_with("goto ") || code.starts_with("return")) {
            return std::string{indent} + std::string{code} + ";";
        }
        if (code.starts_with("var ")) {
            auto declaration = go2_trim(code.substr(4));
            auto equal = declaration.find('=');
            auto left = go2_trim(declaration.substr(0, equal));
            auto value = equal == std::string_view::npos ? std::string_view{} : go2_trim(declaration.substr(equal + 1));
            auto space = left.find_first_of(" \t");
            auto name = space == std::string_view::npos ? left : go2_trim(left.substr(0, space));
            auto type = space == std::string_view::npos ? std::string{"auto"} : go2_cpp1_type(go2_trim(left.substr(space + 1)));
            return std::string{indent} + type + " " + std::string{name}
                + (value.empty() ? std::string{"{};"} : " = " + std::string{value} + ";");
        }
        if (code.starts_with("const ")) {
            auto declaration = go2_trim(code.substr(6));
            auto equal = declaration.find('=');
            auto left = go2_trim(declaration.substr(0, equal));
            auto value = equal == std::string_view::npos ? std::string_view{} : go2_trim(declaration.substr(equal + 1));
            auto space = left.find_first_of(" \t");
            auto name = space == std::string_view::npos ? left : go2_trim(left.substr(0, space));
            auto type = space == std::string_view::npos ? std::string{"auto"} : go2_cpp1_type(go2_trim(left.substr(space + 1)));
            return std::string{indent} + "const " + type + " " + std::string{name} + " = " + std::string{value} + ";";
        }
        if (auto colon_equal = code.find(":="); colon_equal != std::string_view::npos) {
            return std::string{indent} + "auto " + std::string{go2_trim(code.substr(0, colon_equal))} + " = " + std::string{go2_trim(code.substr(colon_equal + 2))} + ";";
        }
        if (code.starts_with("fmt.Println(") && code.ends_with(")")) {
            return go2_println(indent, code.substr(12, code.size() - 13));
        }
        return std::string{indent} + std::string{code} + ";";
    }

    auto go2_cpp1_function() const -> std::string
    {
        assert(!go_function_lines.empty());
        auto const header = go2_trim(go_function_lines.front());
        auto const signature = header.substr(5);
        auto const open = signature.find('(');
        auto const close = signature.find(')', open);
        auto const brace = signature.rfind('{');
        if (open == std::string_view::npos || close == std::string_view::npos || brace == std::string_view::npos) {
            return {};
        }

        auto const name = go2_trim(signature.substr(0, open));
        auto const parameters = signature.substr(open + 1, close - open - 1);
        auto const result = go2_trim(signature.substr(close + 1, brace - close - 1));
        auto output = std::string{"auto "} + std::string{name} + "(" + go2_cpp1_parameters(parameters) + ") -> "
            + (result.empty() ? (name == "main" ? "int" : "void") : go2_cpp1_type(result)) + " {\n";

        for (auto i = size_t{1}; i < go_function_lines.size(); ++i) {
            output += go2_cpp1_statement(go_function_lines[i]);
            output += '\n';
        }
        return output;
    }

    auto normalize_buffered_function() -> go2_line;

    auto is_go2_exported_identifier(std::string_view name) const -> bool
    {
        return !name.empty() && std::isupper(static_cast<unsigned char>(name.front()));
    }

    auto add_go2_class_pointer_parameters(std::string_view text) -> void
    {
        go_class_pointer_names.clear();
        while (!text.empty()) {
            auto const comma = text.find(',');
            auto const parameter = go2_trim(text.substr(0, comma));
            text = comma == std::string_view::npos ? std::string_view{} : text.substr(comma + 1);
            auto const space = parameter.find_first_of(" \t");
            if (space == std::string_view::npos) {
                continue;
            }
            auto const name = go2_trim(parameter.substr(0, space));
            auto const type = go2_trim(parameter.substr(space + 1));
            if (type.starts_with("*")) {
                go_class_pointer_names.emplace_back(name);
            }
        }
    }

    auto rewrite_go2_class_expressions(std::string text) const -> std::string
    {
        auto rewrite = [&text](std::string const& name) {
            auto start = size_t{0};
            while ((start = text.find(name + ".", start)) != std::string::npos) {
                auto const before_is_identifier = start > 0
                    && (std::isalnum(static_cast<unsigned char>(text[start - 1])) || text[start - 1] == '_');
                if (!before_is_identifier) {
                    text.replace(start, name.size() + 1, name + "->");
                    start += name.size() + 2;
                }
                else {
                    start += name.size() + 1;
                }
            }
        };

        rewrite("this");
        for (auto const& name : go_class_pointer_names) {
            rewrite(name);
        }
        return text;
    }

    auto go2_cpp1_container_initializer(std::string_view value) const -> std::optional<std::string>
    {
        auto const open = value.find('{');
        if (open == std::string_view::npos || !value.ends_with("}")) {
            return {};
        }
        auto const type = go2_cpp1_type(value.substr(0, open));
        if (!type.starts_with("std::vector<") && !type.starts_with("std::array<") && !type.starts_with("std::map<")) {
            return {};
        }
        auto const values = go2_trim(value.substr(open + 1, value.size() - open - 2));
        if (type.starts_with("std::map<") && !values.empty()) {
            return {};
        }
        return type + "{" + std::string{values} + "}";
    }

    auto go2_cpp1_value(std::string_view value) const -> std::string
    {
        if (auto const container = go2_cpp1_container_initializer(value)) {
            return *container;
        }
        if (value.ends_with("{}") && value.find('{') != std::string_view::npos) {
            return go2_cpp1_type(value.substr(0, value.size() - 2)) + "{}";
        }
        return rewrite_go2_class_expressions(std::string{value});
    }

    auto go2_class_statement(std::string_view line) const -> std::string
    {
        auto const indent = go2_indent(line);
        auto const code = go2_trim(line);
        if (code.empty() || code.starts_with("//") || code == "}" || code == "} else {") {
            return std::string{line};
        }
        if (code.starts_with("if ") && code.ends_with("{")) {
            return std::string{indent} + "if (" + rewrite_go2_class_expressions(std::string{go2_trim(code.substr(3, code.size() - 4))}) + ") {";
        }
        if (code.starts_with("} else if ") && code.ends_with("{")) {
            return std::string{indent} + "} else if (" + rewrite_go2_class_expressions(std::string{go2_trim(code.substr(10, code.size() - 11))}) + ") {";
        }
        if (code.starts_with("for ")) {
            auto const header = go2_trim(code.substr(4));
            if (header == "{") {
                return std::string{indent} + "while (true) {";
            }
            if (auto const range = header.find(" := range "); range != std::string_view::npos && header.ends_with("{")) {
                auto const variables = go2_trim(header.substr(0, range));
                auto const comma = variables.find(',');
                auto const value = comma == std::string_view::npos ? variables : go2_trim(variables.substr(comma + 1));
                auto const container = go2_trim(header.substr(range + 10, header.size() - range - 11));
                return std::string{indent} + "for (auto const& " + std::string{value} + " : "
                    + rewrite_go2_class_expressions(std::string{container}) + ") {";
            }
            auto const brace = header.rfind('{');
            auto const first_semicolon = header.find(';');
            auto const second_semicolon = first_semicolon == std::string_view::npos ? std::string_view::npos : header.find(';', first_semicolon + 1);
            if (brace != std::string_view::npos && second_semicolon != std::string_view::npos) {
                auto const init = go2_trim(header.substr(0, first_semicolon));
                auto const condition = go2_trim(header.substr(first_semicolon + 1, second_semicolon - first_semicolon - 1));
                auto const next = go2_trim(header.substr(second_semicolon + 1, brace - second_semicolon - 1));
                auto const colon_equal = init.find(":=");
                auto const init_cpp1 = colon_equal == std::string_view::npos
                    ? rewrite_go2_class_expressions(std::string{init})
                    : "auto " + std::string{go2_trim(init.substr(0, colon_equal))} + " = " + go2_cpp1_value(go2_trim(init.substr(colon_equal + 2)));
                return std::string{indent} + "for (" + init_cpp1 + "; "
                    + rewrite_go2_class_expressions(std::string{condition}) + "; "
                    + rewrite_go2_class_expressions(std::string{next}) + ") {";
            }
            if (brace != std::string_view::npos) {
                return std::string{indent} + "while (" + rewrite_go2_class_expressions(std::string{go2_trim(header.substr(0, brace))}) + ") {";
            }
        }
        if (code.starts_with("var ")) {
            auto const declaration = go2_trim(code.substr(4));
            auto const equal = declaration.find('=');
            auto const left = go2_trim(declaration.substr(0, equal));
            auto const value = equal == std::string_view::npos ? std::string_view{} : go2_trim(declaration.substr(equal + 1));
            auto const space = left.find_first_of(" \t");
            auto const name = space == std::string_view::npos ? left : go2_trim(left.substr(0, space));
            auto const type = space == std::string_view::npos ? std::string{"auto"} : go2_cpp1_type(go2_trim(left.substr(space + 1)));
            return std::string{indent} + type + " " + std::string{name}
                + (value.empty() ? std::string{"{};"} : " = " + go2_cpp1_value(value) + ";");
        }
        if (code.starts_with("const ")) {
            auto const declaration = go2_trim(code.substr(6));
            auto const equal = declaration.find('=');
            auto const left = go2_trim(declaration.substr(0, equal));
            auto const value = equal == std::string_view::npos ? std::string_view{} : go2_trim(declaration.substr(equal + 1));
            auto const space = left.find_first_of(" \t");
            auto const name = space == std::string_view::npos ? left : go2_trim(left.substr(0, space));
            auto const type = space == std::string_view::npos ? std::string{"auto"} : go2_cpp1_type(go2_trim(left.substr(space + 1)));
            return std::string{indent} + "const " + type + " " + std::string{name} + " = " + go2_cpp1_value(value) + ";";
        }
        if (auto const colon_equal = code.find(":="); colon_equal != std::string_view::npos) {
            return std::string{indent} + "auto " + std::string{go2_trim(code.substr(0, colon_equal))} + " = "
                + go2_cpp1_value(go2_trim(code.substr(colon_equal + 2))) + ";";
        }
        if (code.starts_with("fmt.Println(") && code.ends_with(")")) {
            return go2_println(indent, rewrite_go2_class_expressions(std::string{code.substr(12, code.size() - 13)}));
        }
        if (code.starts_with("return")) {
            return std::string{indent} + rewrite_go2_class_expressions(std::string{code}) + ";";
        }
        if (code.ends_with("{") || code.ends_with(":")) {
            return std::string{line};
        }
        return std::string{indent} + rewrite_go2_class_expressions(std::string{code}) + ";";
    }

    auto normalize_go2_class_line(std::string_view line) -> go2_line
    {
        auto const indent = go2_indent(line);
        auto const code = go2_trim(line);

        if (in_go_class_method) {
            auto const output = go2_class_statement(line);
            go_class_method_brace_depth += go2_brace_delta(line);
            if (go_class_method_brace_depth <= 0) {
                in_go_class_method = false;
                go_class_pointer_names.clear();
            }
            return {output, go2_line_kind::cpp2};
        }

        if (code == "}") {
            in_go_class = false;
            return {std::string{indent} + "};", go2_line_kind::cpp2};
        }
        if (code.empty() || code.starts_with("//")) {
            return {std::string{line}, go2_line_kind::cpp2};
        }

        auto const member = code;
        if (member.starts_with("func ")) {
            auto const signature = member.substr(5);
            auto const open = signature.find('(');
            auto const close = signature.find(')', open);
            auto const brace = signature.rfind('{');
            if (open != std::string_view::npos && close != std::string_view::npos && brace != std::string_view::npos) {
                auto const name = go2_trim(signature.substr(0, open));
                auto const parameters = signature.substr(open + 1, close - open - 1);
                auto const result = go2_trim(signature.substr(close + 1, brace - close - 1));
                add_go2_class_pointer_parameters(parameters);
                in_go_class_method = true;
                go_class_method_brace_depth = go2_brace_delta(member);
                auto const method_access = is_go2_exported_identifier(name) ? "public" : "private";
                return {
                    std::string{indent} + method_access + ": auto " + std::string{name} + "(" + go2_cpp1_parameters(parameters) + ") -> "
                        + (result.empty() ? "void" : go2_cpp1_type(result)) + " {",
                    go2_line_kind::cpp2
                };
            }
        }

        auto const space = member.find_first_of(" \t");
        if (space != std::string_view::npos) {
            auto const name = go2_trim(member.substr(0, space));
            auto const type = go2_trim(member.substr(space + 1));
            auto const field_access = is_go2_exported_identifier(name) ? "public" : "private";
            return {
                std::string{indent} + field_access + ": " + go2_cpp1_type(type) + " " + std::string{name} + "{};",
                go2_line_kind::cpp2
            };
        }
        return {std::string{line}, go2_line_kind::cpp2};
    }

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
    auto normalize_line(std::string_view line) -> go2_line
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

        if (in_go_class) {
            return normalize_go2_class_line(line);
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

        if (in_go_interface) {
            if (code == "}") {
                in_go_interface = false;
                return {std::string{indent} + "};", go2_line_kind::cpp2};
            }
            auto open = code.find('(');
            auto close = code.find(')', open);
            if (open != std::string_view::npos && close != std::string_view::npos) {
                auto name = go2_trim(code.substr(0, open));
                auto parameters = code.substr(open + 1, close - open - 1);
                auto result = go2_trim(code.substr(close + 1));
                auto return_type = result.empty() ? "void" : go2_type(result);
                return {std::string{indent} + "virtual " + return_type + " " + std::string{name} + "(" + go2_cpp1_parameters(parameters) + ") = 0;", go2_line_kind::cpp2};
            }
        }

        if (in_go_select) {
            if (code == "}") {
                in_go_select = false;
                return {"", go2_line_kind::ignored};
            }
            if (code.starts_with("case ") && code.ends_with(":")) {
                auto receive = go2_trim(code.substr(5, code.size() - 6));
                auto assign = receive.find(":= <-");
                if (assign != std::string_view::npos) {
                    auto name = go2_trim(receive.substr(0, assign));
                    auto channel = go2_trim(receive.substr(assign + 5));
                    return {std::string{indent} + std::string{name} + ": _ = " + std::string{channel} + ".receive();", go2_line_kind::cpp2};
                }
            }
        }

        if (in_go_switch) {
            auto const matched = "go2_switch_matched_" + std::to_string(go_switch_number);
            auto const fell_through = "go2_switch_fallthrough_" + std::to_string(go_switch_number);
            if (code == "}") {
                in_go_switch = false;
                go_switch_case_open = false;
                return {std::string{indent} + "}", go2_line_kind::cpp2};
            }
            if (code == "fallthrough") {
                return {std::string{indent} + fell_through + " = true;", go2_line_kind::cpp2};
            }
            if ((code.starts_with("case ") && code.ends_with(":")) || code == "default:") {
                auto condition = std::string{"!"} + matched;
                if (code != "default:") {
                    auto value = go2_trim(code.substr(5, code.size() - 6));
                    condition = "((!" + matched + " && " + go_switch_expression + " == " + std::string{value} + ") || " + fell_through + ")";
                }
                auto prefix = go_switch_case_open ? "} " : "";
                go_switch_case_open = true;
                return {std::string{indent} + prefix + "if " + condition + " { " + matched + " = true; " + fell_through + " = false;", go2_line_kind::cpp2};
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
            return {std::string{indent} + "#include \"go2util.h\"\n#include <string>", go2_line_kind::preprocessor};
        }
        if (code == "import \"fmt\"") {
            return {std::string{indent} + "#include <iostream>", go2_line_kind::preprocessor};
        }

        if (code.starts_with("type ") && code.ends_with(" class {")) {
            auto const name = go2_trim(code.substr(5, code.size() - 13));
            in_go_class = true;
            return {std::string{indent} + "class " + std::string{name} + " {", go2_line_kind::cpp2};
        }

        if (code.starts_with("type ") && code.ends_with(" struct {")) {
            auto name = go2_trim(code.substr(5, code.size() - 13));
            in_go_struct = true;
            return {std::string{indent} + std::string{name} + ": @basic_value type = {", go2_line_kind::cpp2};
        }

        if (code.starts_with("type ") && code.ends_with(" interface {")) {
            auto name = go2_trim(code.substr(5, code.size() - 16));
            in_go_interface = true;
            return {std::string{indent} + "struct " + std::string{name} + " { virtual ~" + std::string{name} + "() = default;", go2_line_kind::cpp2};
        }

        if (code.starts_with("switch ") && code.ends_with("{")) {
            ++go_switch_number;
            go_switch_expression = std::string{go2_trim(code.substr(7, code.size() - 8))};
            in_go_switch = true;
            go_switch_case_open = false;
            auto const matched = "go2_switch_matched_" + std::to_string(go_switch_number);
            auto const fell_through = "go2_switch_fallthrough_" + std::to_string(go_switch_number);
            return {std::string{indent} + matched + ": bool = false; " + fell_through + ": bool = false;", go2_line_kind::cpp2};
        }

        if (code == "select {") {
            in_go_select = true;
            return {"", go2_line_kind::ignored};
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
        if (auto range = header.find(" := range "); range != std::string_view::npos && header.ends_with("{")) {
            auto variables = go2_trim(header.substr(0, range));
            auto comma = variables.find(',');
            auto value = comma == std::string_view::npos ? variables : go2_trim(variables.substr(comma + 1));
            auto container = go2_trim(header.substr(range + 10, header.size() - range - 11));
            return {std::string{indent} + "for " + std::string{container} + " do (copy " + std::string{value} + ") {", go2_line_kind::cpp2};
        }

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

        if (code.starts_with("defer fmt.Println(") && code.ends_with(")")) {
            auto argument = go2_trim(code.substr(18, code.size() - 19));
            ++go_defer_number;
            return {std::string{indent} + "go2_defer_" + std::to_string(go_defer_number) + ": go2::deferred_print = (" + std::string{argument} + ");", go2_line_kind::cpp2};
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

        if (code.starts_with("const ")) {
            auto declaration = go2_trim(code.substr(6));
            auto equal = declaration.find('=');
            auto left = go2_trim(declaration.substr(0, equal));
            auto value = equal == std::string_view::npos ? std::string_view{} : go2_trim(declaration.substr(equal + 1));
            auto space = left.find_first_of(" \t");
            auto name = space == std::string_view::npos ? left : go2_trim(left.substr(0, space));
            auto type = space == std::string_view::npos ? std::string_view{"_"} : go2_trim(left.substr(space + 1));
            return {
                std::string{indent} + std::string{name} + ": const " + go2_type(type) + " = " + rewrite_pointer_expressions(std::string{value}) + ";",
                go2_line_kind::cpp2
            };
        }

        if (auto colon_equal = code.find(":="); colon_equal != std::string_view::npos) {
            auto name = go2_trim(code.substr(0, colon_equal));
            auto value = go2_trim(code.substr(colon_equal + 2));
            if (value.starts_with("make(chan ") && value.ends_with(")")) {
                auto arguments = value.substr(10, value.size() - 11);
                auto comma = arguments.find(',');
                auto type = go2_trim(arguments.substr(0, comma));
                auto capacity = comma == std::string_view::npos ? std::string_view{"1"} : go2_trim(arguments.substr(comma + 1));
                return {std::string{indent} + std::string{name} + ": go2::channel<" + go2_type(type) + "> = (" + std::string{capacity} + ");", go2_line_kind::cpp2};
            }
            if (value.starts_with("<-")) {
                return {std::string{indent} + std::string{name} + ": _ = " + std::string{go2_trim(value.substr(2))} + ".receive();", go2_line_kind::cpp2};
            }
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
        if (code.starts_with("go ") && code.ends_with(")")) {
            auto call = go2_trim(code.substr(3));
            auto open = call.find('(');
            if (open != std::string_view::npos) {
                auto function = go2_trim(call.substr(0, open));
                auto arguments = go2_trim(call.substr(open + 1, call.size() - open - 2));
                return {std::string{indent} + "go2::go_call(" + std::string{function} + (arguments.empty() ? ");" : ", " + std::string{arguments} + ");"), go2_line_kind::cpp2};
            }
        }
        if (auto send = code.find(" <- "); send != std::string_view::npos) {
            return {std::string{indent} + std::string{go2_trim(code.substr(0, send))} + ".send(" + std::string{go2_trim(code.substr(send + 4))} + ");", go2_line_kind::cpp2};
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

    explicit go2_normalizer(bool buffer_functions_ = true)
        : buffer_functions{buffer_functions_}
    { }

    auto normalize(std::string_view line) -> go2_line
    {
        if (!buffer_functions) {
            return normalize_line(line);
        }

        auto const code = go2_trim(line);
        if (buffering_go_function) {
            go_function_lines.emplace_back(line);
            go_function_brace_depth += go2_brace_delta(line);
            if (go_function_brace_depth <= 0) {
                buffering_go_function = false;
                return normalize_buffered_function();
            }
            return {"", go2_line_kind::ignored};
        }

        if (
            !in_go_class
            &&
            code.starts_with("func ")
            && code.find('(') != std::string_view::npos
            && code.rfind('{') != std::string_view::npos
            )
        {
            buffering_go_function = true;
            go_function_brace_depth = go2_brace_delta(line);
            go_function_lines = {std::string{line}};
            if (go_function_brace_depth <= 0) {
                buffering_go_function = false;
                return normalize_buffered_function();
            }
            return {"", go2_line_kind::ignored};
        }

        return normalize_line(line);
    }
};

auto go2_normalizer::normalize_buffered_function() -> go2_line
{
    if (go2_function_has_goto()) {
        // Cpp2 intentionally has no unrestricted goto; preserve Go labels as Cpp1.
        auto result = go2_cpp1_function();
        go_function_lines.clear();
        return {std::move(result), go2_line_kind::cpp2};
    }

    auto normalizer = go2_normalizer{false};
    auto result = std::string{};
    for (auto const& line : go_function_lines) {
        auto translated = normalizer.normalize(line);
        if (translated.kind != go2_line_kind::ignored) {
            result += translated.text;
        }
        result += '\n';
    }
    go_function_lines.clear();
    return {std::move(result), go2_line_kind::cpp2};
}

}

#endif
