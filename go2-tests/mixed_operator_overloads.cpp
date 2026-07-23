package main

#include <iostream>

type Box class {
    length float64
    breadth float64
    height float64

    func GetVolume() float64 {
        return this.length * this.breadth * this.height
    }

    func SetLength(length float64) {
        this.length = length
    }

    func SetBreadth(breadth float64) {
        this.breadth = breadth
    }

    func SetHeight(height float64) {
        this.height = height
    }

    func operator+(other Box) Box {
        result := Box{}
        result.length = this.length + other.length
        result.breadth = this.breadth + other.breadth
        result.height = this.height + other.height
        return result
    }
}

type OperatorProbe class {
    value int

    func Set(value int) {
        this.value = value
    }

    func operator[](index int) int {
        return this.value + index
    }

    func operator()(factor int) int {
        return this.value * factor
    }

    func operator++() int {
        this.value = this.value + 1
        return this.value
    }

    func operator+=(other OperatorProbe) int {
        this.value = this.value + other.value
        return this.value
    }

    func operator<(other OperatorProbe) bool {
        return this.value < other.value
    }

    func operator!() bool {
        return this.value == 0
    }
}

//go2:cpp1-begin
auto main() -> int {
    Box first {};
    Box second {};
    first.SetLength(6.0);
    first.SetBreadth(7.0);
    first.SetHeight(5.0);
    second.SetLength(12.0);
    second.SetBreadth(13.0);
    second.SetHeight(10.0);

    auto combined = first + second;
    OperatorProbe probe {};
    OperatorProbe other {};
    OperatorProbe empty {};
    probe.Set(5);
    other.Set(8);
    auto const subscript = probe[2] == 7;
    auto const call = probe(3) == 15;
    auto const increment = ++probe == 6;
    auto const relation = probe < other;
    probe += other;
    auto const compound_assignment = probe(2) == 28;

    std::cout << first.GetVolume() << " " << second.GetVolume() << " " << combined.GetVolume() << " "
        << subscript << " " << call << " " << increment << " " << relation << " "
        << compound_assignment << " " << !empty << "\n";
}
//go2:cpp1-end
