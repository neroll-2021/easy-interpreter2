#include <cstdint>
#include <memory>
#include <print>
#include <fstream>
#include "parser.h"
#include "variable.h"
using namespace neroll::script;
using namespace neroll::script::detail;
int main() {
    std::ifstream fin("../../../../script/main.txt");
    if (!fin.is_open()) {
        std::println("cannot open file");
        exit(EXIT_FAILURE);
    }
    parser psr{detail::lexer{detail::input_stream_adapter{fin}}};

    try {
        auto node = psr.parse_expression();


        node->evaluate();
        // assert(node->eval_type() == variable_type::integer);
        // auto arr1 = std::dynamic_pointer_cast<array_value_node>(node);
        // assert(arr1 != nullptr);
        // assert(arr1->eval_type() == variable_type::integer);
        // auto arr2 = std::dynamic_pointer_cast<array_value_node>(arr1->get_array_node());
        // assert(arr2 != nullptr);
        // assert(arr2->eval_type() == variable_type::array);
        std::println("value: {}", node->get<int32_t>());

        // auto arr1 = node->get<array>();
        // for (std::size_t i = 0; i < arr1.size(); i++) {
        //     auto arr2 = std::get<array>(arr1[i]);
        //     for (std::size_t j = 0; j < arr2.size(); j++) {
        //         std::print("{} ", std::get<int32_t>(arr2[j]));
        //     }
        //     std::println("");
        // }
    } catch (std::exception &e) {
        std::println("{}", e.what());
    }
}