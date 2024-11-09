#include <print>
#include "detail/array.h"
using namespace neroll::script;
using namespace neroll::script::detail;
int main() {
    array a1;
    for (int i = 0; i < 10; i++) {
        array a2;
        for (int j = 0; j < 15; j++) {
            a2.push_back(0);
        }
        a1.push_back(a2);
    }
    std::println("empty: {}", a1.empty());
    std::println("size: {}", a1.size());
}