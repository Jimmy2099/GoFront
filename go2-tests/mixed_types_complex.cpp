package main

#include <array>
#include <complex>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <type_traits>
#include <vector>

type Go2TypeReader interface {
    read(value int) int
}

type Go2TypeRecord struct {
    flag bool
    unsigned8 uint8
    unsigned16 uint16
    unsigned32 uint32
    unsigned64 uint64
    signed8 int8
    signed16 int16
    signed32 int32
    signed64 int64
    signedWord int
    byteValue byte
    runeValue rune
    unsignedWord uint
    address uintptr
    decimal32 float32
    decimal64 float64
    complex32 complex64
    complex64 complex128
    text string
    fixed [3]int
    slice []int
    bytes []byte
    runes []rune
    labels map[string]int
    transform func(int) int
    go2Value int
    cpp2Value int
    cpp1Value int
    readerValue int
    functionValue int
    channelValue int
    driver int
}

//go2:cpp1-begin
struct Cpp1TypeReader final : Go2TypeReader {
    auto read(int value) -> int override {
        return value + 1;
    }
};

Cpp1TypeReader cpp1_type_reader {};
Go2TypeRecord go2_type_workspace {};

auto go2_populate_type_case(Go2TypeRecord* record, Go2TypeReader* reader, int driver) -> int;

auto cpp1_double(int value) -> int {
    return value * 2;
}

auto cpp1_float32_value() -> float {
    return 1.25F;
}

auto cpp1_float64_value() -> double {
    return 2.5;
}

auto cpp1_complex64_value() -> std::complex<float> {
    return {1.5F, -2.0F};
}

auto cpp1_complex128_value() -> std::complex<double> {
    return {3.5, 4.5};
}

auto cpp1_begin_type_case(int driver) -> Go2TypeRecord* {
    go2_type_workspace = Go2TypeRecord{};
    go2_type_workspace.driver = driver;
    return &go2_type_workspace;
}

auto cpp1_type_reader_instance() -> Go2TypeReader* {
    return &cpp1_type_reader;
}

auto cpp1_finish_type_case(Go2TypeRecord* record, int driver) -> int {
    static_assert(std::is_same_v<decltype(record->flag), bool>);
    static_assert(std::is_same_v<decltype(record->unsigned8), std::uint8_t>);
    static_assert(std::is_same_v<decltype(record->unsigned16), std::uint16_t>);
    static_assert(std::is_same_v<decltype(record->unsigned32), std::uint32_t>);
    static_assert(std::is_same_v<decltype(record->unsigned64), std::uint64_t>);
    static_assert(std::is_same_v<decltype(record->signed8), std::int8_t>);
    static_assert(std::is_same_v<decltype(record->signed16), std::int16_t>);
    static_assert(std::is_same_v<decltype(record->signed32), std::int32_t>);
    static_assert(std::is_same_v<decltype(record->signed64), std::int64_t>);
    static_assert(std::is_same_v<decltype(record->signedWord), int>);
    static_assert(std::is_same_v<decltype(record->byteValue), std::uint8_t>);
    static_assert(std::is_same_v<decltype(record->runeValue), char32_t>);
    static_assert(std::is_same_v<decltype(record->unsignedWord), go2::uint>);
    static_assert(sizeof(record->unsignedWord) == sizeof(int));
    static_assert(std::is_same_v<decltype(record->address), std::uintptr_t>);
    static_assert(std::is_same_v<decltype(record->decimal32), float>);
    static_assert(std::is_same_v<decltype(record->decimal64), double>);
    static_assert(std::is_same_v<decltype(record->complex32), std::complex<float>>);
    static_assert(std::is_same_v<decltype(record->complex64), std::complex<double>>);
    static_assert(std::is_same_v<decltype(record->text), std::string>);
    static_assert(std::is_same_v<decltype(record->fixed), std::array<int, 3>>);
    static_assert(std::is_same_v<decltype(record->slice), std::vector<int>>);
    static_assert(std::is_same_v<decltype(record->bytes), std::vector<std::uint8_t>>);
    static_assert(std::is_same_v<decltype(record->runes), std::vector<char32_t>>);
    static_assert(std::is_same_v<decltype(record->labels), std::map<std::string, int>>);
    static_assert(std::is_invocable_r_v<int, decltype(record->transform), int>);

    record->cpp1Value = 300 + driver;
    record->labels["cpp1"] = record->cpp1Value;

    return
        record->flag
        && record->unsigned8 == 8
        && record->unsigned16 == 16
        && record->unsigned32 == 32
        && record->unsigned64 == 64
        && record->signed8 == -8
        && record->signed16 == -16
        && record->signed32 == -32
        && record->signed64 == -64
        && record->signedWord == -77
        && record->byteValue == 65
        && record->runeValue == static_cast<char32_t>(0x4E2D)
        && record->unsignedWord == 77
        && record->address == static_cast<std::uintptr_t>(4096)
        && record->decimal32 == 1.25F
        && record->decimal64 == 2.5
        && record->complex32 == std::complex<float>{1.5F, -2.0F}
        && record->complex64 == std::complex<double>{3.5, 4.5}
        && record->text == "Go2"
        && record->fixed[0] == 7
        && record->fixed[1] == 8
        && record->fixed[2] == 9
        && record->slice.size() == 3
        && record->slice[0] == 3 + driver
        && record->slice[1] == 8
        && record->slice[2] == 5
        && record->bytes.size() == 3
        && record->bytes[0] == 65
        && record->bytes[2] == 67
        && record->runes.size() == 2
        && record->runes[1] == static_cast<char32_t>(0x4E2D)
        && record->labels["go2"] == driver
        && record->labels["cpp2"] == 200 + driver
        && record->labels["cpp1"] == 300 + driver
        && record->transform(6) == 12
        && record->go2Value == 100 + driver
        && record->cpp2Value == 200 + driver
        && record->cpp1Value == 300 + driver
        && record->readerValue == 21
        && record->functionValue == 12
        && record->channelValue == 13
        && record->driver == driver;
}

auto cpp1_type_case() -> int {
    return go2_populate_type_case(cpp1_begin_type_case(1), cpp1_type_reader_instance(), 1);
}
//go2:cpp1-end

cpp2_apply_type_case: (record: * Go2TypeRecord, driver: int) -> int = {
    record*.cpp2Value = 200 + driver;
    record*.labels["cpp2"] = record*.cpp2Value;
    record*.slice[0] = record*.slice[0] + driver;
    return cpp1_finish_type_case(record, driver);
}

cpp2_type_case: () -> int = {
    return go2_populate_type_case(cpp1_begin_type_case(2), cpp1_type_reader_instance(), 2);
}

func go2_populate_type_case(record *Go2TypeRecord, reader *Go2TypeReader, driver int) int {
    record.flag = true
    record.unsigned8 = 8
    record.unsigned16 = 16
    record.unsigned32 = 32
    record.unsigned64 = 64
    record.signed8 = -8
    record.signed16 = -16
    record.signed32 = -32
    record.signed64 = -64
    record.signedWord = -77
    record.byteValue = 65
    record.runeValue = 0x4E2D
    record.unsignedWord = 77
    record.address = 4096
    record.decimal32 = cpp1_float32_value()
    record.decimal64 = cpp1_float64_value()
    record.complex32 = cpp1_complex64_value()
    record.complex64 = cpp1_complex128_value()
    record.text = "Go2"

    fixed := [3]int{7, 8, 9}
    values := []int{3, 4, 5}
    values[1] = values[0] + values[2]
    bytes := []byte{65, 66, 67}
    runes := []rune{65, 0x4E2D}
    labels := map[string]int{}
    labels["go2"] = driver

    record.fixed = fixed
    record.slice = values
    record.bytes = bytes
    record.runes = runes
    record.labels = labels
    record.transform = cpp1_double
    record.go2Value = 100 + driver
    record.readerValue = reader.read(20)
    record.functionValue = record.transform(6)

    channel := make(chan int, 1)
    channel <- 13
    received := <-channel
    record.channelValue = received

    return cpp2_apply_type_case(record, driver)
}

func go2_type_case() int {
    return go2_populate_type_case(cpp1_begin_type_case(3), cpp1_type_reader_instance(), 3)
}

//go2:cpp1-begin
auto main() -> int {
    std::cout << cpp1_type_case() << "\n";
    std::cout << cpp2_type_case() << "\n";
    std::cout << go2_type_case() << "\n";
}
//go2:cpp1-end
