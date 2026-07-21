package main
import "fmt"

#include <map>
#include <string>
#include <vector>

//go2:cpp1-begin
auto cpp1_keyword_seed() -> int {
    return 1;
}
//go2:cpp1-end

cpp2_keyword_seed: () -> int = {
    return cpp1_keyword_seed() + 1;
}

type KeywordRecord struct {
    total int
    value int
}

type KeywordReader interface {
    read(value int) int
}

struct Cpp1KeywordReader final : KeywordReader {
    auto read(int value) -> int override {
        return value + 1;
    }
};

func go2_keyword_seed() int {
    return cpp2_keyword_seed() + 1
}

func go2_keyword_import() int {
    return 1
}

func go2_keyword_var() int {
    var value int = 4
    return value
}

func go2_keyword_const() int {
    const value int = 5
    return value
}

func go2_keyword_type() int {
    record := KeywordRecord{}
    record.total = 3
    return record.total
}

func go2_keyword_return() int {
    return 7
}

func go2_keyword_if_else(value int) int {
    if value == 1 {
        return 1
    } else {
        return 2
    }
}

func go2_keyword_for() int {
    total := 0
    for i := 0; i < 3; i++ {
        total = total + 1
    }
    return total
}

func go2_keyword_range() int {
    values := []int{1, 2, 3}
    total := 0
    for _, value := range values {
        total = total + value
    }
    return total
}

func go2_keyword_switch(value int) int {
    result := 0
    switch value {
    case 1:
        result = 10
        fallthrough
    case 2:
        result = result + 2
    default:
        result = 99
    }
    return result
}

func go2_keyword_break() int {
    value := 0
    for {
        value = value + 1
        break
    }
    return value
}

func go2_keyword_continue() int {
    total := 0
    for i := 0; i < 3; i++ {
        if i == 1 {
            continue
        }
        total = total + i
    }
    return total
}

func go2_keyword_goto() int {
    value := 0
    goto done
    value = -1
done:
    value = value + 1
    return value
}

func go2_keyword_defer() {
    defer fmt.Println("1")
}

func go2_keyword_worker(ch chan int, value int) {
    ch <- value
}

func go2_keyword_concurrency() int {
    ch := make(chan int, 1)
    go go2_keyword_worker(ch, 9)
    select {
    case value := <-ch:
        return value
    }
    return 0
}

func go2_keyword_struct() int {
    record := KeywordRecord{}
    record.value = 8
    return record.value
}

func go2_keyword_interface(reader *KeywordReader) int {
    return reader.read(4)
}

func go2_keyword_map() int {
    values := map[string]int{}
    values["go2"] = 6
    return values["go2"]
}

cpp2_call_go2: () -> int = {
    return go2_keyword_return();
}

//go2:cpp1-begin
auto main() -> int {
    Cpp1KeywordReader reader {};

    std::cout << (go2_keyword_seed() == 3) << "\n";
    std::cout << (go2_keyword_import() == 1) << "\n";
    std::cout << (go2_keyword_var() == 4) << "\n";
    std::cout << (go2_keyword_const() == 5) << "\n";
    std::cout << (go2_keyword_type() == 3) << "\n";
    std::cout << (cpp2_call_go2() == 7) << "\n";
    std::cout << (go2_keyword_if_else(1) == 1) << "\n";
    std::cout << (go2_keyword_if_else(0) == 2) << "\n";
    std::cout << (go2_keyword_for() == 3) << "\n";
    std::cout << (go2_keyword_range() == 6) << "\n";
    std::cout << (go2_keyword_switch(1) == 12) << "\n";
    std::cout << (go2_keyword_switch(2) == 2) << "\n";
    std::cout << (go2_keyword_switch(3) == 99) << "\n";
    std::cout << (go2_keyword_switch(1) == 12) << "\n";
    std::cout << (go2_keyword_break() == 1) << "\n";
    std::cout << (go2_keyword_continue() == 2) << "\n";
    std::cout << (go2_keyword_goto() == 1) << "\n";
    std::cout << (go2_keyword_return() == 7) << "\n";
    go2_keyword_defer();
    std::cout << (go2_keyword_concurrency() == 9) << "\n";
    std::cout << (go2_keyword_concurrency() == 9) << "\n";
    std::cout << (go2_keyword_concurrency() == 9) << "\n";
    std::cout << (go2_keyword_struct() == 8) << "\n";
    std::cout << (go2_keyword_interface(&reader) == 5) << "\n";
    std::cout << (go2_keyword_map() == 6) << "\n";
}
//go2:cpp1-end
