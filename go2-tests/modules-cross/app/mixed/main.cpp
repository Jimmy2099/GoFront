package main

import "example.com/go2-math/arith"

#include <iostream>

auto cpp1_seed() -> int {
    return 2;
}

cpp2_factor: () -> int = {
    return 2;
}

func go2_apply(left int, right int) int {
    ledger := arith.Build(left, right)
    return arith.Score(ledger) * cpp2_factor()
}

auto main() -> int {
    std::cout << go2_apply(cpp1_seed(), 3) << "\n";
}
