package main

#include <iostream>
#include <string>

//go2:cpp1-begin
auto cpp1_pick(int value) -> int {
    return value + 1;
}

auto cpp1_pick(std::string const& value) -> int {
    return static_cast<int>(value.size()) + 10;
}
//go2:cpp1-end

cpp2_pick: (copy value: int) -> int = {
    return value + 10;
}

cpp2_pick: (copy value: std::string) -> int = {
    return value.size() + 20;
}

func go2_pick(value int) int {
    return value + 30
}

func go2_pick(value string) int {
    return value.size() + 30
}

func go2_pick(left int, right int) int {
    return left * 10 + right
}

func go2_bridge(value int) int {
    return cpp1_pick(value) + cpp2_pick(value) + go2_pick(value)
}

func go2_bridge(value string) int {
    return cpp1_pick(value) + cpp2_pick(value) + go2_pick(value)
}

type PrintData class {
    Prefix string

    func Print(value int) string {
        return this.Prefix + "int:" + std::to_string(value)
    }

    func Print(value float64) string {
        return this.Prefix + "float:" + std::to_string(value)
    }

    func Print(value string) string {
        return this.Prefix + "string:" + value
    }
}

//go2:cpp1-begin
auto main() -> int {
    PrintData printer {};
    printer.Prefix = "go2:";

    std::cout
        << (go2_bridge(5) == 56) << " "
        << (go2_bridge(std::string{"go"}) == 66) << " "
        << (go2_pick(3, 4) == 34) << " "
        << (printer.Print(5) == "go2:int:5") << " "
        << (printer.Print(500.263) == "go2:float:500.263000") << " "
        << (printer.Print(std::string{"Hello C++"}) == "go2:string:Hello C++") << "\n";
}
//go2:cpp1-end
