#ifndef NEROLL_SCRIPT_PARSER_H
#define NEROLL_SCRIPT_PARSER_H

#include <cassert>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <print>
#include <string>
#include <utility>
#include "detail/array.h"
#include "detail/ast.h"
#include "detail/lexer.h"
#include "detail/ring_buffer.h"
#include "exception.h"
#include "variable.h"

namespace neroll::script {

using namespace detail;

class parser {
 public:
    parser(detail::lexer &&lexer)
        : lexer_(std::move(lexer)) {
        for (std::size_t i = 0; i < buffer_.capacity(); i++) {
            get_token();
        }
    }

    std::shared_ptr<statement_node> parse() {
        return nullptr;
    }

 private:
 public:
    constexpr static std::size_t look_ahead_count = 2;

    lexer lexer_;
    ring_buffer<detail::token, look_ahead_count> buffer_;

    std::shared_ptr<expr_node> parse_additive() {
        std::shared_ptr<expr_node> lhs = parse_multiplicative();
        std::shared_ptr<expr_node> rhs = nullptr;
        std::shared_ptr<expr_node> op = nullptr;
        while (current_token_type() == token_type::plus || current_token_type() == token_type::minus) {
            if (current_token_type() == token_type::plus) {
                match(token_type::plus);
                rhs = parse_multiplicative();
                op = std::make_shared<add_node>(std::move(lhs), std::move(rhs));
            } else  {
                match(token_type::minus);
                rhs = parse_multiplicative();
                op = std::make_shared<minus_node>(std::move(lhs), std::move(rhs));
            }
            lhs = op;
            op = nullptr;
        }
        return lhs;
    }

    std::shared_ptr<expr_node> parse_multiplicative() {
        std::shared_ptr<expr_node> lhs = parse_cast();
        std::shared_ptr<expr_node> rhs = nullptr;
        std::shared_ptr<expr_node> op = nullptr;
        while (current_token_type() == token_type::asterisk || current_token_type() == token_type::slash ||
               current_token_type() == token_type::mod) {
            if (current_token_type() == token_type::asterisk) {
                match(token_type::asterisk);
                rhs = parse_cast();
                op = std::make_shared<multiply_node>(std::move(lhs), std::move(rhs));
            } else if (current_token_type() == token_type::slash) {
                match(token_type::slash);
                rhs = parse_cast();
                op = std::make_shared<divide_node>(std::move(lhs), std::move(rhs));
            } else {
                match(token_type::mod);
                rhs = parse_cast();
                op = std::make_shared<modulus_node>(std::move(lhs), std::move(rhs));
            }
            lhs = op;
            op = nullptr;
        }
        return lhs;
    }

    std::shared_ptr<expr_node> parse_cast() {
        if (current_token_type() == token_type::left_parenthesis) {
            match(token_type::left_parenthesis);
            token_type type_name = current_token_type();

            match("type name", {
                token_type::keyword_int, token_type::keyword_float,
                token_type::keyword_boolean, token_type::keyword_string,
                token_type::keyword_char
            });
            match(token_type::right_parenthesis);

            std::shared_ptr<expr_node> expr = parse_cast();

            switch (type_name) {
                case token_type::keyword_int:
                    return std::make_shared<type_cast_node>(std::move(expr), variable_type::integer);
                case token_type::keyword_float:
                    return std::make_shared<type_cast_node>(std::move(expr), variable_type::floating);
                case token_type::keyword_boolean:
                    return std::make_shared<type_cast_node>(std::move(expr), variable_type::boolean);
                case token_type::keyword_string:
                    return std::make_shared<type_cast_node>(std::move(expr), variable_type::string);
                case token_type::keyword_char:
                    return std::make_shared<type_cast_node>(std::move(expr), variable_type::character);
                default:
                    std::unreachable();
            }
            std::unreachable();
        }
        return parse_unary();
    }

    std::shared_ptr<expr_node> parse_unary() {
        switch (current_token_type()) {
            case token_type::plus:
                match(token_type::plus);
                return parse_unary();
            case token_type::minus:
                match(token_type::minus);
                return std::make_shared<negative_node>(parse_unary());
            case token_type::bit_not:
                match(token_type::bit_not);
                return std::make_shared<bit_not_node>(parse_unary());
            case token_type::logical_not:
                match(token_type::logical_not);
                return std::make_shared<logical_not_node>(parse_unary());
            default:
                return parse_primary();
        }
    }

    std::shared_ptr<expr_node> parse_postfix() {
        // TODO
        return parse_primary();
    }

    std::shared_ptr<expr_node> parse_primary() {
        switch (current_token_type()) {
            case token_type::literal_int:
                return make_node_and_match<int32_t>(token_type::literal_int);
            case token_type::literal_float:
                return make_node_and_match<double>(token_type::literal_float);
            case token_type::literal_true:
                return make_node_and_match<bool>(token_type::literal_true);
            case token_type::literal_false:
                return make_node_and_match<bool>(token_type::literal_false);
            case token_type::literal_string:
                return make_node_and_match<std::string>(token_type::literal_string);
            case token_type::literal_char:
                return make_node_and_match<char>(token_type::literal_char);
            case token_type::identifier:
                // return parse_variable_or_function_call();
            default:
                throw_syntax_error("not supported");
        }
    }

    std::shared_ptr<expr_node> parse_variable_or_function_call() {
        match(token_type::identifier);
        if (current_token_type() == token_type::left_parenthesis) {
            return parse_function_call();
        }
        return parse_variable();
    }

    std::shared_ptr<expr_node> parse_variable() {
        // TODO
        std::println("parse variable");
        return nullptr;
    }

    std::shared_ptr<expr_node> parse_function_call() {
        // TODO
        std::println(("parse function call"));
        return nullptr;
    }

    template <typename T>
    std::shared_ptr<expr_node> make_node_and_match(token_type type) {
        static_type_check<T>();
        static_assert(!std::is_same_v<T, array>);
        std::shared_ptr<expr_node> node = make_node<T>(current_token());
        match(type);
        return node;
    }

    template <typename T>
    [[nodiscard]]
    std::shared_ptr<expr_node> make_node(const token &token) const {
        static_type_check<T>();
        static_assert(!std::is_same_v<T, array>);

        if constexpr (std::is_same_v<T, int32_t>) {
            int32_t value = std::stoi(token.content);
            return std::make_shared<int_node>(value);

        } else if constexpr (std::is_same_v<T, double>) {
            double value = std::stod(token.content);
            return std::make_shared<float_node>(value);

        } else if constexpr (std::is_same_v<T, bool>) {
            return std::make_shared<boolean_node>(token.type == token_type::literal_true);

        } else if constexpr (std::is_same_v<T, std::string>) {
            const std::string str = token.content;
            return std::make_shared<string_node>(str.substr(1, str.size() - 2));

        } else {    // char
            return std::make_shared<char_node>(token.content.at(0));
        }
    }

    [[nodiscard]]
    const token &current_token() const noexcept {
        return buffer_.get_next(0);
    }

    [[nodiscard]]
    token_type current_token_type() const noexcept {
        return buffer_.get_next(0).type;
    }

    void get_token() {
        buffer_.put(lexer_.next_token());
    }

    [[nodiscard]]
    const token &next_token(std::size_t distance) const noexcept {
        return buffer_.get_next(distance);
    }

    void match(token_type expect) {
        if (current_token_type() == expect) {
            get_token();
        } else {
            const auto &position = lexer_.position();
            throw_syntax_error(
                "line {}, column {}: expect '{}', found '{}'",
                position.lines_read + 1, position.chars_read_current_line + 1,
                expect, current_token_type()
            );
        }
    }

    void match(std::string_view expect_token, std::initializer_list<token_type> expect_types) {
        for (const auto type : expect_types) {
            if (current_token_type() == type) {
                match(type);
                return;
            }
        }
        const auto &position = lexer_.position();
        throw_syntax_error(
            "line {}, column {}: expect {}, found '{}'",
            position.lines_read + 1, position.chars_read_current_line + 1,
            expect_token, current_token_type()
        );
    }
};

}

#endif