#include <iostream>

cpp2_increment: (value: int) -> int = {
    return value + 1;
}

func go2_double(value int) int {
    return value * 2
}

auto main() -> int {
    std::cout << go2_double(cpp2_increment(3)) << "\n";
}
