#include <print>
#include "detail/ring_buffer.h"
#include "detail/lexer.h"

using namespace neroll::script::detail;

int main() {
    ring_buffer<token, 2> buffer;
    std::println("capacity: {}", buffer.capacity());
    token tok{"+", token_type::plus, {}};
    buffer.put(tok);
    buffer.put(tok);
    buffer.put(tok);
    buffer.put(tok);
    auto n = buffer.get_next(0);
    std::println("{} {}", n.content, n.type);
}