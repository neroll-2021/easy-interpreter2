#include <cstdlib>
#include <exception>
#include <print>
#include <fstream>

#include "detail/lexer.h"

using namespace neroll::script::detail;

int main() {
    std::ifstream fin("../../../../script/main.txt");
    if (!fin.is_open()) {
        std::println("cannot open file");
        exit(EXIT_FAILURE);
    }
    lexer lex(input_stream_adapter{fin});

    try {
        token tok = lex.next_token();
        while (tok.type != token_type::end_of_input) {
            std::println("{}", tok);
            tok = lex.next_token();
        }
    }
    catch (std::exception &e) {
        std::println("{}", e.what());
    }
    
}