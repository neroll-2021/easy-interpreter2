#ifndef NEROLL_SCRIPT_DETAIL_LEXER_H
#define NEROLL_SCRIPT_DETAIL_LEXER_H

#include <print>
#include <string>           // string
#include <cstddef>          // size_t
#include <format>           // formatter
#include <string_view>      // string_view
#include <unordered_map>    // unordered_map
#include <cassert>          // assert

#include "exception.h"
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

    identifier,

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
        : token("", type_, position) {}
    
    token(std::string_view content_, token_type type_, const position_t &position)
        : content(content_), type(type_), line(position.lines_read + 1),
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
        case token_type::identifier:
            return "identifier";
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
        skip_whitespace();

        switch (current_) {
            case '+':
                return token{"+", token_type::plus, position_};
            case '-':
                return token{"-", token_type::minus, position_};
            case '*':
                return token{"*", token_type::asterisk, position_};
            case '/':
                return token{"/", token_type::slash, position_};
            case '%':
                return token{"%", token_type::mod, position_};
            case '&':
                if (get() == '&')
                    return token{"&&", token_type::logical_and, position_};
                unget();
                return token{"&", token_type::bit_and, position_};
            case '|':
                if (get() == '|')
                    return token{"||", token_type::logical_or, position_};
                unget();
                return token{"|", token_type::bit_or, position_};
            case '^':
                return token{"^", token_type::bit_xor, position_};
            case '~':
                return token{"~", token_type::bit_not, position_};
            case '<': {
                char_int_type next = get();
                if (next == '<')
                    return token{"<<", token_type::shift_left, position_};
                if (next == '=')
                    return token{"<=", token_type::less_equal, position_};
                unget();
                return token{"<", token_type::less, position_};
            }
            case '>': {
                char_int_type next = get();
                if (next == '>')
                    return token{">>", token_type::shift_right, position_};
                if (next == '=')
                    return token{">=", token_type::greater_equal, position_};
                unget();
                return token{">", token_type::greater, position_};
            }
            case '\\':
                return token{"\\", token_type::backslash, position_};
            case '!':
                if (get() == '=')
                    return token{"!=", token_type::not_equal, position_};
                unget();
                return token{"!", token_type::logical_not, position_};
            case '=':
                if (get() == '=')
                    return token{"==", token_type::equal, position_};
                unget();
                return token{"=", token_type::assign, position_};
            case ';':
                return token{";", token_type::semicolon, position_};
            case ':':
                return token{":", token_type::colon, position_};
            case ',':
                return token{",", token_type::comma, position_};
            case '.':
                return token{".", token_type::dot, position_};
            case '(':
                return token{"(", token_type::left_parenthesis, position_};
            case ')':
                return token{")", token_type::right_parenthesis, position_};
            case '[':
                return token{"[", token_type::left_bracket, position_};
            case ']':
                return token{"]", token_type::right_bracket, position_};
            case '{':
                return token{"{", token_type::left_brace, position_};
            case '}':
                return token{"}", token_type::right_brace, position_};
            case '\'':
                return token{"'", token_type::single_quotation, position_};
            case '"':
                return token{"\"", token_type::double_quotation, position_};
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                return scan_number();
            case '\0':
            case std::char_traits<char_type>::eof():
                return token{"eof", token_type::end_of_input, position_};
            default:
                if (std::isalpha(current_) || current_ == '_') {
                    return scan_identifier();
                }
                throw_syntax_error("line {}, column {}: unknown token",
                    position_.lines_read + 1, position_.chars_read_current_line
                );
        }
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
    const static inline std::unordered_map<std::string_view, token_type> keywords_ = {
        {"int", token_type::keyword_int}, {"float", token_type::keyword_float},
        {"boolean", token_type::keyword_boolean}, {"string", token_type::keyword_string},
        {"char", token_type::keyword_char}, {"function", token_type::keyword_function},
        {"if", token_type::keyword_if}, {"else", token_type::keyword_else},
        {"for", token_type::keyword_for}, {"while", token_type::keyword_while},
        {"continue", token_type::keyword_continue}, {"break", token_type::keyword_break},
        {"return", token_type::keyword_return}
    };

    token scan_number() {
        reset();
        unget();
        int previous_state = -1;
        int state = 0;
        while (state != -1) {
            get();
            switch (state) {
                case 0:
                    switch (current_) {
                        case '0':
                            previous_state = state;
                            state = 2;
                            break;
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                            previous_state = state;
                            state = 3;
                            break;
                        default:
                            previous_state = state;
                            state = -1;
                            break;
                    }
                    break;
                case 1:
                    switch (current_) {
                        case '0':
                            previous_state = state;
                            state = 2;
                            break;
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                            previous_state = state;
                            state = 3;
                            break;
                        default:
                            previous_state = state;
                            state = -1;
                            break;
                    }
                    break;
                case 2:
                    switch (current_) {
                        case '.':
                            previous_state = state;
                            state = 4;
                            break;
                        default:
                            previous_state = state;
                            state = -1;
                            break;
                    }
                    break;
                case 3:
                    switch (current_) {
                        case '.':
                            previous_state = state;
                            state = 4;
                            break;
                        case 'e':
                        case 'E':
                            previous_state = state;
                            state = 6;
                            break;
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                            // add(current_);
                            previous_state = state;
                            // state = 3;
                            break;
                        default:
                            previous_state = state;
                            state = -1;
                            break;
                    }
                    break;
                case 4:
                    switch (current_) {
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                            // add(current_);
                            previous_state = state;
                            state = 5;
                            break;
                        default:
                            previous_state = state;
                            state = -1;
                            break;
                    }
                    break;
                case 5:
                    switch (current_) {
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                            // add(current_);
                            previous_state = state;
                            // state = 5;
                            break;
                        case 'e':
                        case 'E':
                            // add(current_);
                            previous_state = state;
                            state = 6;
                            break;
                        default:
                            previous_state = state;
                            state = -1;
                            break;
                    }
                    break;
                case 6:
                    switch (current_) {
                        case '+':
                        case '-':
                            // add(current_);
                            previous_state = state;
                            state = 7;
                            break;
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                            // add(current_);
                            previous_state = state;
                            state = 8;
                            break;
                        default:
                            previous_state = state;
                            state = -1;
                            break;
                    }
                    break;
                case 7:
                    switch (current_) {
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                            // add(current_);
                            previous_state = state;
                            state = 8;
                            break;
                        default:
                            // add(current_);
                            previous_state = state;
                            state = -1;
                            break;
                    }
                    break;
                case 8:
                    switch (current_) {
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                            // add(current_);
                            previous_state = state;
                            state = 8;
                            break;
                        default:
                            previous_state = state;
                            state = -1;
                            break;
                    }
                    break;
                default:
                    throw std::runtime_error("invaild state");
            }
        }
        // check invaid number such as 123a
        if (std::isalpha(current_)) {
            return token{token_string_, token_type::parse_error, position_};
        }
        unget();
        if (previous_state == 2 || previous_state == 3) {
            return token(token_string_, token_type::literal_int, position_);
        }
        if (previous_state == 5 || previous_state == 8) {
            return token(token_string_, token_type::literal_float, position_);
        }
        return token("invalid number literal", token_type::parse_error, position_);
    }

    token scan_identifier() {
        reset();
        while (std::isalnum(current_) || current_ == '_') {
            get();
        }
        unget();
        auto iter = keywords_.find(token_string_);
        if (iter != keywords_.end())
            return {iter->first, iter->second, position_};
        if (token_string_ == "true")
            return token{token_string_, token_type::literal_true, position_};
        if (token_string_ == "false")
            return token{token_string_, token_type::literal_false, position_};
        return token{token_string_, token_type::identifier, position_};
    }

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
        auto it = context.begin();
        if (it != context.end() && *it != '}') {
            throw std::format_error("invalid format specifier");
        }
        return it;
    }

    auto format(neroll::script::detail::token_type type, std::format_context &context) const {
        using neroll::script::detail::token_type_name;
        return std::format_to(context.out(), "{}", token_type_name(type));
    }
};

template <>
struct std::formatter<neroll::script::detail::token> {
    constexpr auto parse(std::format_parse_context &context) {
        auto it = context.begin();
        if (it != context.end() && *it != '}') {
            throw std::format_error("invalid format specifier");
        }
        return it;
    }

    auto format(const neroll::script::detail::token &token, std::format_context &context) const {
        using neroll::script::detail::token_type_name;
        return std::format_to(context.out(), "<{}, {}>", token.content, token_type_name(token.type));
    }
};

#endif