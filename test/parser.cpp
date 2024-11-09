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
        auto node = psr.parse_multiplicative();
        node->evaluate();
        std::println("{}", node->get<int32_t>());
    } catch (std::exception &e) {
        std::println("{}", e.what());
    }
}