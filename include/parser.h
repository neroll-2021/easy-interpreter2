#ifndef NEROLL_SCRIPT_PARSER_H
#define NEROLL_SCRIPT_PARSER_H

#include <memory>
#include "detail/ast.h"
#include "detail/lexer.h"
#include "detail/ring_buffer.h"

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
    constexpr static std::size_t look_ahead_count = 2;

    lexer lexer_;
    ring_buffer<detail::token, look_ahead_count> buffer_;

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