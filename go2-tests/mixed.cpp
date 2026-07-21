#include <iostream>

cpp2_add: (a: int, b: int) -> int = {
    return a + b;
}

func go2_multiply(a int, b int) int {
    return a * b
}

auto main() -> int {
    std::cout << cpp2_add(2, 3) << " " << go2_multiply(3, 4) << "\n";
}
