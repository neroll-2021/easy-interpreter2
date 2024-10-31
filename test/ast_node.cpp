#include <cstdint>
#include <format>
#include <memory>
#include <print>
#include "detail/lexer.h"
#include "detail/ast.h"

using namespace neroll::script::detail;

int main() {
    try {
        add_node add(std::make_shared<string_node>("hello"), std::make_shared<string_node>(" world!"));
        add.evaluate();
        std::println("{}", add.get<std::string>());

        logical_and_node logical_and(std::make_shared<boolean_node>(true), std::make_shared<boolean_node>(true));
        logical_and.evaluate();
        std::println("{}", logical_and.get<bool>());

        less_node less(std::make_shared<string_node>("aaa"), std::make_shared<string_node>("aab"));
        less.evaluate();
        std::println("{}", less.get<bool>());

        logical_not_node not_node(std::make_shared<boolean_node>(true));
        not_node.evaluate();
        std::println("{}", not_node.get<bool>());

        bit_and_node and_node(std::make_shared<int_node>(3), std::make_shared<int_node>(1));
        and_node.evaluate();
        std::println("{}", and_node.get<int32_t>());

        bit_or_node or_node(std::make_shared<int_node>(3), std::make_shared<int_node>(4));
        or_node.evaluate();
        std::println("{}", or_node.get<int32_t>());

        bit_xor_node xor_node(std::make_shared<int_node>(3), std::make_shared<int_node>(1));
        xor_node.evaluate();
        std::println("{}", xor_node.get<int32_t>());

        bit_not_node bnot(std::make_shared<int_node>(0));
        bnot.evaluate();
        std::println("{}", bnot.get<int32_t>());

        shift_left_node sleft(std::make_shared<int_node>(1), std::make_shared<int_node>(3));
        sleft.evaluate();
        std::println("{}", sleft.get<int32_t>());

        shift_right_node sright(std::make_shared<int_node>(8), std::make_shared<int_node>(3));
        sright.evaluate();
        std::println("{}", sright.get<int32_t>());
    }
    catch (std::exception &e) {
        std::println("{}", e.what());
    }
    
}