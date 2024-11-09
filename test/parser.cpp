#include <cstdint>
#include <exception>
#include <print>
#include <fstream>
#include <string>
#include "variable.h"
#include "parser.h"
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
        auto node = psr.parse_logical_or();
        node->evaluate();
        auto arr = node->get<array>();

        for (std::size_t i = 0; i < arr.size(); i++) {
            auto arr2 = std::get<array>(arr[i]);
            for (std::size_t j = 0; j < arr2.size(); j++) {
                std::print("{} ", std::get<int32_t>(arr2[j]));
            }
            std::println("");
        }
        
    } catch (std::exception &e) {
        std::println("{}", e.what());
    }
}