#ifndef NEROLL_SCRIPT_VARIABLE_H
#define NEROLL_SCRIPT_VARIABLE_H

#include <format>   // formatter
#include <memory>   // shared_ptr
#include <array>    // array

#include "detail/lexer.h"

namespace neroll {

namespace script {

using namespace detail;

enum class variable_type {
    error = -1, integer, floating, boolean, string, character, array
};

const char *variable_type_name(variable_type type) {
    switch (type) {
        case variable_type::integer:
            return "int";
        case variable_type::floating:
            return "double";
        case variable_type::boolean:
            return "boolean";
        case variable_type::string:
            return "string";
        case variable_type::character:
            return "char";
        case variable_type::array:
            return "array";
        default:
            return "unknown";
    }
}

variable_type to_variable_type(token_type type) {
    switch (type) {
        case token_type::keyword_int:
            return variable_type::integer;
        case token_type::keyword_float:
            return variable_type::floating;
        case token_type::keyword_boolean:
            return variable_type::boolean;
        case token_type::keyword_string:
            return variable_type::string;
        case token_type::keyword_char:
            return variable_type::character;
        default:
            return variable_type::error;
    }
}

variable_type arithmetic_type_cast(variable_type lhs, variable_type rhs) {
    constexpr static std::array<std::array<int, 6>, 6> table{{
        {0,  1, -1, -1, -1, -1}, {1,  1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1}
    }};
    auto lhs_index = static_cast<std::size_t>(lhs);
    auto rhs_index = static_cast<std::size_t>(rhs);
    return static_cast<variable_type>(table[lhs_index][rhs_index]);
}

bool is_both_string(variable_type lhs, variable_type rhs) {
    return lhs == variable_type::string && rhs == variable_type::string;
}

bool is_both_int(variable_type lhs, variable_type rhs) {
    return lhs == variable_type::integer && rhs == variable_type::integer;
}

bool is_both_boolean(variable_type lhs, variable_type rhs) {
    return lhs == variable_type::boolean && rhs == variable_type::boolean;
}

bool can_compare(variable_type lhs, variable_type rhs) {
    constexpr static std::array<std::array<int, 6>, 6> table {{
        {1, 1, 0, 0, 0, 0}, {1, 1, 0, 0, 0, 0}, {0, 0, 1, 0, 0, 0},
        {0, 0, 0, 1, 0, 0}, {0, 0, 0, 0, 1, 0}, {0, 0, 0, 0, 0, 0}
    }};
    auto lhs_index = static_cast<std::size_t>(lhs);
    auto rhs_index = static_cast<std::size_t>(rhs);
    return table[lhs_index][rhs_index];
}

variable_type binary_expr_type(variable_type lhs, detail::token_type op, variable_type rhs) {
    using detail::token_type;
    switch (op) {
        case token_type::plus:
            if (is_both_string(lhs, rhs))
                return variable_type::string;
        case token_type::minus:
        case token_type::asterisk:
        case token_type::slash:
            return arithmetic_type_cast(lhs, rhs);
        case token_type::mod:
            if (!is_both_int(lhs, rhs))
                return variable_type::error;
            return variable_type::integer;
        case token_type::logical_and:
        case token_type::logical_or:
            if (!is_both_boolean(lhs, rhs))
                return variable_type::error;
            return variable_type::boolean;
        case token_type::equal:
        case token_type::not_equal:
            if (can_compare(lhs, rhs))
                return variable_type::boolean;
            return variable_type::error;
        case token_type::less:
        case token_type::less_equal:
        case token_type::greater:
        case token_type::greater_equal:
            if (is_both_boolean(lhs, rhs))
                return variable_type::error;
            if (can_compare(lhs, rhs))
                return variable_type::boolean;
            return variable_type::error;
        default:
            return variable_type::error;
    }
}

bool is_arithmetic_type(variable_type type) {
    switch (type) {
        case variable_type::integer:
        case variable_type::floating:
            return true;
        default:
            return false;
    }
}

class object {
 public:
    virtual variable_type type() const = 0;
    virtual ~object() {}
};

class variable : public object {
 public:
    variable(std::string_view name, std::shared_ptr<object> value)
        : name_(name), value_(value) {}

    const std::string name() const {
        return name_;
    }
    std::shared_ptr<object> value() const {
        return value_;
    }

 private:
    std::string name_;
    std::shared_ptr<object> value_;
};

class int_t : public object {
 public:
    int_t(int32_t value) : value_(value) {}

    variable_type type() const override {
        return variable_type::integer;
    }

 private:
    int32_t value_;
};

}   // namespace script

}   // namespace neroll

template <>
struct std::formatter<neroll::script::variable_type> {
    constexpr auto parse(std::format_parse_context &context) {
        auto it = context.begin();
        if (it != context.end() && *it != '}')
            throw std::format_error("invalid format specifier");
        return context.begin();
    }

    auto format(neroll::script::variable_type type, std::format_context &context) const {
        using neroll::script::variable_type_name;
        return std::format_to(context.out(), "{}", variable_type_name(type));
    }
};

#endif