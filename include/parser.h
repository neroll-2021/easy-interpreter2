#ifndef NEROLL_SCRIPT_PARSER_H
#define NEROLL_SCRIPT_PARSER_H

#include <cassert>
#include <cstdint>
#include <memory>
#include <print>
#include <string>
#include "detail/array.h"
#include "detail/ast.h"
#include "detail/lexer.h"
#include "detail/ring_buffer.h"
#include "exception.h"

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
};

}

#endif