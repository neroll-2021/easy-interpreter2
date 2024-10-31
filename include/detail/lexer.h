#ifndef NEROLL_SCRIPT_DETAIL_LEXER_H
#define NEROLL_SCRIPT_DETAIL_LEXER_H

#include <string>           // string
#include <cstddef>          // size_t
#include <format>           // formatter
#include <unordered_map>    // unordered_map
#include <cassert>          // assert

#include "input_adapter.h"
#include "position_t.h"

namespace neroll {

namespace script {

namespace detail {

enum class token_type {
    keyword_int,        // int
    keyword_float,      // double
    keyword_boolean,    // boolean
    keyword_string,     // string
    keyword_char,       // char

    keyword_function,   // function

    keyword_if,         // if
    keyword_else,       // else

    keyword_for,        // for
    keyword_while,      // while

    keyword_continue,   // continue
    keyword_break,      // break
    keyword_return,     // return

    literal_int,        // 123
    literal_float,      // 1.23
    literal_true,       // true
    literal_false,      // false
    literal_string,     // "string"
    literal_char,       // 'c'

    plus,               // +
    minus,              // -
    asterisk,           // *
    slash,              // /
    mod,                // %

    bit_and,            // &
    bit_or,             // |
    bit_xor,            // ^
    bit_not,            // ~
    shift_left,         // <<
    shift_right,        // >>

    backslash,          /* \ */

    logical_and,        // &&
    logical_or,         // ||
    logical_not,        // !
    
    less,               // <
    less_equal,         // <=
    greater,            // >
    greater_equal,      // >=
    equal,              // ==
    not_equal,          // !=

    assign,             // =

    semicolon,          // ;
    colon,              // :
    comma,              // ,
    dot,                // .

    left_parenthesis,   // (
    right_parenthesis,  // )
    left_bracket,       // [
    right_bracket,      // ]
    left_brace,         // {
    right_brace,        // }

    single_quotation,   // '
    double_quotation,   // "

    end_of_input,       // eof
    parse_error
};

struct token {
    std::string_view content;
    token_type type{};
    std::size_t line{};
    std::size_t column{};

    token() = default;

    token(token_type type_, const position_t &position)
        : type(type_), line(position.lines_read + 1),
          column(position.chars_read_current_line) {}
};

const char *token_type_name(token_type type) {
    switch (type) {
        case token_type::keyword_int:
            return "int";
        case token_type::keyword_float:
            return "double";
        case token_type::keyword_boolean:
            return "boolean";
        case token_type::keyword_string:
            return "string";
        case token_type::keyword_char:
            return "char";
        case token_type::keyword_function:
            return "function";
        case token_type::keyword_if:
            return "if";
        case token_type::keyword_else:
            return "else";
        case token_type::keyword_for:
            return "for";
        case token_type::keyword_while:
            return "while";
        case token_type::keyword_continue:
            return "continue";
        case token_type::keyword_break:
            return "break";
        case token_type::keyword_return:
            return "return";
        case token_type::literal_int:
            return "literal int";
        case token_type::literal_float:
            return "literal double";
        case token_type::literal_true:
            return "true";
        case token_type::literal_false:
            return "false";
        case token_type::literal_string:
            return "literal string";
        case token_type::literal_char:
            return "literal char";
        case token_type::plus:
            return "+";
        case token_type::minus:
            return "-";
        case token_type::asterisk:
            return "*";
        case token_type::slash:
            return "/";
        case token_type::mod:
            return "%";
        case token_type::bit_and:
            return "&";
        case token_type::bit_or:
            return "|";
        case token_type::bit_xor:
            return "^";
        case token_type::bit_not:
            return "~";
        case token_type::shift_left:
            return "<<";
        case token_type::shift_right:
            return ">>";
        case token_type::backslash:
            return "\\";
        case token_type::logical_and:
            return "&&";
        case token_type::logical_or:
            return "||";
        case token_type::logical_not:
            return "!";
        case token_type::less:
            return "<";
        case token_type::less_equal:
            return "<=";
        case token_type::greater:
            return ">";
        case token_type::greater_equal:
            return ">=";
        case token_type::equal:
            return "==";
        case token_type::not_equal:
            return "!=";
        case token_type::assign:
            return "=";
        case token_type::semicolon:
            return ";";
        case token_type::colon:
            return ":";
        case token_type::comma:
            return ",";
        case token_type::dot:
            return ".";
        case token_type::left_parenthesis:
            return "(";
        case token_type::right_parenthesis:
            return ")";
        case token_type::left_bracket:
            return "[";
        case token_type::right_bracket:
            return "]";
        case token_type::left_brace:
            return "{";
        case token_type::right_brace:
            return "}";
        case token_type::single_quotation:
            return "\'";
        case token_type::double_quotation:
            return "\"";
        case token_type::end_of_input:
            return "<eof>";
        case token_type::parse_error:
            return "<error>";
        default:
            return "<unknown token>";
    }
}

class lexer {
 public:
    using char_type     = typename input_stream_adapter::char_type;
    using char_int_type = typename std::char_traits<char_type>::int_type;

    explicit lexer(input_stream_adapter &&adapter)
        : adapter_(std::move(adapter)) {}

    token next_token() {
        return {};
    }

    void rewind() {
        adapter_.rewind();
    }

 private:
    input_stream_adapter adapter_;
    position_t position_;
    bool next_unget_ = false;
    char_int_type current_ = std::char_traits<char_type>::eof();
    std::string token_string_;
    const static inline std::unordered_map<std::string_view, token_type> keywords = {
        {"int", token_type::keyword_int}, {"float", token_type::keyword_float},
        {"boolean", token_type::keyword_boolean}, {"string", token_type::keyword_string},
        {"char", token_type::keyword_char}, {"function", token_type::keyword_function},
        {"if", token_type::keyword_if}, {"else", token_type::keyword_else},
        {"for", token_type::keyword_for}, {"while", token_type::keyword_while},
        {"continue", token_type::keyword_continue}, {"break", token_type::keyword_break},
        {"return", token_type::keyword_return}
    };

    char_int_type get() {
        ++position_.chars_read_total;
        ++position_.chars_read_current_line;

        if (next_unget_) {
            next_unget_ = false;
        } else {
            current_ = adapter_.get_character();
        }

        if (current_ != std::char_traits<char_type>::eof()) [[likely]] {
            token_string_.push_back(std::char_traits<char_type>::to_char_type(current_));
        }

        if (current_ == '\n') {
            ++position_.lines_read;
            position_.chars_read_current_line = 0;
        }

        return current_;
    }

    void unget() {
        next_unget_ = true;

        position_.chars_read_total--;
        if (position_.chars_read_current_line == 0) {
            if (position_.lines_read > 0) {
                position_.lines_read--;
            }
        } else {
            position_.chars_read_current_line--;
        }

        if (current_ != std::char_traits<char_type>::eof()) [[likely]] {
            assert(!token_string_.empty());
            token_string_.pop_back();
        }
    }

    void reset() {
        token_string_.clear();
        token_string_.push_back(std::char_traits<char_type>::to_char_type(current_));
    }

    void add(char_int_type c) {
        token_string_.push_back(static_cast<std::string::value_type>(c));
    }

    void skip_whitespace() {
        do {
            get();
        } while (current_ == ' ' || current_ == '\n' || current_ == '\r' || current_ == '\t');
    }



};

}   // namespace detail

}   // namespace script

}   // namespace neroll

template <>
struct std::formatter<neroll::script::detail::token_type> {
    constexpr auto parse(std::format_parse_context &context) {
        if (context.begin() != context.end())
            throw std::format_error("invalid format specifier");
        return context.begin();
    }

    auto format(neroll::script::detail::token_type type, std::format_context &context) const {
        using neroll::script::detail::token_type_name;
        return std::format_to(context.out(), "{}", token_type_name(type));
    }
};

template <>
struct std::formatter<neroll::script::detail::token> {
    constexpr auto parse(std::format_parse_context &context) {
        if (context.begin() != context.end())
            throw std::format_error("invalid format specifier");
        return context.begin();
    }

    auto format(const neroll::script::detail::token &token, std::format_context &context) const {
        using neroll::script::detail::token_type_name;
        using neroll::script::detail::token_type;

        if (token.type == token_type::end_of_input)
            return std::format_to(context.out(), "<EOF, {}>", token_type_name(token.type));
        if (token.type == token_type::parse_error)
            return std::format_to(context.out(), "<parse error, {}>", token_type_name(token.type));
        return std::format_to(context.out(), "<{}, {}>", token.content, token_type_name(token.type));
    }
};

#endif