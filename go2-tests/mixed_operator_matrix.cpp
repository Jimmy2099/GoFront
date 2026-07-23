package main

#include <array>
#include <cstddef>
#include <iostream>
#include <new>

type OperatorMatrix class {
    value int

    func Set(value int) {
        this.value = value
    }

    func Value() int {
        return this.value
    }

    func operator+() int {
        return this.value
    }
    func operator+(other OperatorMatrix) int { return this.value + other.value }
    func operator-() int {
        result := 0 - this.value
        return result
    }
    func operator-(other OperatorMatrix) int { return this.value - other.value }
    func operator*() int { return this.value }
    func operator*(other OperatorMatrix) int { return this.value * other.value }
    func operator/(other OperatorMatrix) int { return this.value / other.value }
    func operator%(other OperatorMatrix) int { return this.value % other.value }

    func operator==(other OperatorMatrix) bool { return this.value == other.value }
    func operator!=(other OperatorMatrix) bool { return this.value != other.value }
    func operator<(other OperatorMatrix) bool { return this.value < other.value }
    func operator>(other OperatorMatrix) bool { return this.value > other.value }
    func operator<=(other OperatorMatrix) bool { return this.value <= other.value }
    func operator>=(other OperatorMatrix) bool { return this.value >= other.value }

    func operator||(other OperatorMatrix) bool { return this.value != 0 || other.value != 0 }
    func operator&&(other OperatorMatrix) bool { return this.value != 0 && other.value != 0 }
    func operator!() bool {
        return this.value == 0
    }

    func operator&() int {
        return this.value + 100
    }
    func operator&(other OperatorMatrix) int { return this.value & other.value }
    func operator|(other OperatorMatrix) int { return this.value | other.value }
    func operator~() int {
        return ~this.value
    }
    func operator^(other OperatorMatrix) int { return this.value ^ other.value }
    func operator<<(other OperatorMatrix) int { return this.value << other.value }
    func operator>>(other OperatorMatrix) int { return this.value >> other.value }

    func operator++() int { this.value = this.value + 1; return this.value }
    func operator--() int { this.value = this.value - 1; return this.value }

    func operator=(other OperatorMatrix) int {
        this.value = other.value
        return this.value
    }
    func operator+=(other OperatorMatrix) int { this.value = this.value + other.value; return this.value }
    func operator-=(other OperatorMatrix) int { this.value = this.value - other.value; return this.value }
    func operator*=(other OperatorMatrix) int { this.value = this.value * other.value; return this.value }
    func operator/=(other OperatorMatrix) int { this.value = this.value / other.value; return this.value }
    func operator%=(other OperatorMatrix) int { this.value = this.value % other.value; return this.value }
    func operator&=(other OperatorMatrix) int { this.value = this.value & other.value; return this.value }
    func operator|=(other OperatorMatrix) int { this.value = this.value | other.value; return this.value }
    func operator^=(other OperatorMatrix) int { this.value = this.value ^ other.value; return this.value }
    func operator<<=(other OperatorMatrix) int { this.value = this.value << other.value; return this.value }
    func operator>>=(other OperatorMatrix) int { this.value = this.value >> other.value; return this.value }

    func operator()(factor int) int { return this.value * factor }
    func operator[](index int) int { return this.value + index }
    func operator,(other OperatorMatrix) int { return this.value * 100 + other.value }
    func operator->() *OperatorMatrix { return this }
}

type AllocationProbe class {
    value int

    func Set(value int) {
        this.value = value
    }

    func Value() int {
        return this.value
    }

    func operator new(size std::size_t) *void {
        return ::operator new(size)
    }

    func operator delete(pointer *void) {
        ::operator delete(pointer)
    }

    func operator new[](size std::size_t) *void {
        return ::operator new[](size)
    }

    func operator delete[](pointer *void) {
        ::operator delete[](pointer)
    }
}

//go2:cpp1-begin
auto operator_matrix_value(int value) -> OperatorMatrix {
    OperatorMatrix result {};
    result.Set(value);
    return result;
}

auto main() -> int {
    auto six = operator_matrix_value(6);
    auto eight = operator_matrix_value(8);
    auto zero = operator_matrix_value(0);
    auto two = operator_matrix_value(2);
    auto three = operator_matrix_value(3);

    auto assigned = operator_matrix_value(1);
    assigned = eight;
    auto plus_assigned = operator_matrix_value(16);
    plus_assigned += two;
    auto minus_assigned = operator_matrix_value(16);
    minus_assigned -= two;
    auto multiply_assigned = operator_matrix_value(8);
    multiply_assigned *= two;
    auto divide_assigned = operator_matrix_value(16);
    divide_assigned /= two;
    auto modulo_assigned = operator_matrix_value(17);
    modulo_assigned %= three;
    auto and_assigned = operator_matrix_value(14);
    and_assigned &= six;
    auto or_assigned = operator_matrix_value(8);
    or_assigned |= six;
    auto xor_assigned = operator_matrix_value(14);
    xor_assigned ^= six;
    auto left_shift_assigned = operator_matrix_value(3);
    left_shift_assigned <<= two;
    auto right_shift_assigned = operator_matrix_value(16);
    right_shift_assigned >>= two;
    auto incremented = operator_matrix_value(6);
    auto decremented = operator_matrix_value(6);

    auto* allocated = new AllocationProbe {};
    allocated->Set(9);
    auto const scalar_allocation = allocated->Value() == 9;
    delete allocated;
    auto* allocated_array = new AllocationProbe[2];
    allocated_array[0].Set(7);
    auto const array_allocation = allocated_array[0].Value() == 7;
    delete[] allocated_array;

    auto const results = std::array<bool, 43> {
        six + eight == 14, six - eight == -2, six * eight == 48, eight / two == 4, eight % three == 2,
        six == six, six != eight, six < eight, eight > six, six <= six, eight >= six,
        zero || six, six && eight, !zero,
        +six == 6, -six == -6, *six == 6, &six == 106,
        ++incremented == 7, --decremented == 5,
        (six | eight) == 14, (six & eight) == 0, ~zero == -1, (six ^ eight) == 14, (three << two) == 12, (eight >> two) == 2,
        assigned.Value() == 8, plus_assigned.Value() == 18, minus_assigned.Value() == 14, multiply_assigned.Value() == 16,
        divide_assigned.Value() == 8, modulo_assigned.Value() == 2, and_assigned.Value() == 6, or_assigned.Value() == 14,
        xor_assigned.Value() == 8, left_shift_assigned.Value() == 12, right_shift_assigned.Value() == 4,
        six(3) == 18, six[2] == 8, (six, eight) == 608, six->Value() == 6,
        scalar_allocation, array_allocation
    };

    for (auto const result : results) {
        std::cout << result << "\n";
    }
}
//go2:cpp1-end
