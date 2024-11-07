#include <exception>
#include <print>
#include <fstream>
#include "parser.h"
using namespace neroll::script;
int main() {
    std::ifstream fin("../../../../script/main.txt");
    if (!fin.is_open()) {
        std::println("cannot open file");
        exit(EXIT_FAILURE);
    }
    parser psr{detail::lexer{detail::input_stream_adapter{fin}}};

    try {
        
    } catch (std::exception &e) {
        std::println("{}", e.what());
    }
}