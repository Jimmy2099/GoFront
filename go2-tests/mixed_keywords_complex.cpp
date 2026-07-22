package main
import "fmt"

#include <array>
#include <iostream>
#include <map>
#include <string>
#include <vector>

//go2:cpp1-begin
struct KeywordEntry {
    std::string language;
    int value;
};

struct KeywordWorkspace {
    std::array<KeywordEntry, 3> entries;
    std::array<int, 3> checkpoints;
    std::map<std::string, int> totals;
    int revision;
};

KeywordWorkspace keyword_workspace {};

auto go2_keyword_package_case(int driver) -> int;
auto go2_keyword_import_case(int driver) -> int;
auto go2_keyword_var_case(int driver) -> int;
auto go2_keyword_const_case(int driver) -> int;
auto go2_keyword_type_case(int driver) -> int;
auto go2_keyword_func_case(int driver) -> int;
auto go2_keyword_if_case(int driver) -> int;
auto go2_keyword_else_case(int driver) -> int;
auto go2_keyword_for_case(int driver) -> int;
auto go2_keyword_range_case(int driver) -> int;
auto go2_keyword_switch_case(int driver) -> int;
auto go2_keyword_case_case(int driver) -> int;
auto go2_keyword_default_case(int driver) -> int;
auto go2_keyword_fallthrough_case(int driver) -> int;
auto go2_keyword_break_case(int driver) -> int;
auto go2_keyword_continue_case(int driver) -> int;
auto go2_keyword_goto_case(int driver) -> int;
auto go2_keyword_return_case(int driver) -> int;
auto go2_keyword_defer_case(int driver) -> int;
auto go2_keyword_go_case(int driver) -> int;
auto go2_keyword_chan_case(int driver) -> int;
auto go2_keyword_select_case(int driver) -> int;
auto go2_keyword_struct_case(int driver) -> int;
auto go2_keyword_interface_case(int driver) -> int;
auto go2_keyword_map_case(int driver) -> int;

auto cpp1_expected_marker(int keyword) -> int {
    return 100 + keyword;
}

auto cpp1_begin_keyword_case(int driver, int keyword) -> void {
    keyword_workspace.entries[0] = {"cpp1", 0};
    keyword_workspace.entries[1] = {"cpp2", 0};
    keyword_workspace.entries[2] = {"go2", 0};
    keyword_workspace.checkpoints = {0, 0, 0};
    keyword_workspace.totals.clear();
    keyword_workspace.totals["driver"] = driver;
    keyword_workspace.totals["keyword"] = keyword;
    keyword_workspace.revision = 0;
}

auto cpp1_verify_keyword_case(int driver, int keyword, int marker) -> int {
    auto const go2_value = marker + driver + keyword;
    auto const cpp2_value = marker + 2 * driver + keyword;
    auto const cpp1_value = marker + 3 * driver + keyword;

    return
        marker == cpp1_expected_marker(keyword)
        && keyword_workspace.entries[0].language == "cpp1"
        && keyword_workspace.entries[1].language == "cpp2"
        && keyword_workspace.entries[2].language == "go2"
        && keyword_workspace.entries[0].value == cpp1_value
        && keyword_workspace.entries[1].value == cpp2_value
        && keyword_workspace.entries[2].value == go2_value
        && keyword_workspace.checkpoints[0] == cpp1_value
        && keyword_workspace.checkpoints[1] == cpp2_value
        && keyword_workspace.checkpoints[2] == go2_value
        && keyword_workspace.totals["cpp1"] == cpp1_value
        && keyword_workspace.totals["cpp2"] == cpp2_value
        && keyword_workspace.totals["go2"] == go2_value
        && keyword_workspace.totals["driver"] == driver
        && keyword_workspace.totals["keyword"] == keyword
        && keyword_workspace.totals["entry"] == 100 * driver + keyword
        && keyword_workspace.revision == 3;
}

auto cpp1_finish_keyword_case(int driver, int keyword, int marker) -> int {
    auto const value = marker + 3 * driver + keyword;
    keyword_workspace.entries[0].value = value;
    keyword_workspace.checkpoints[0] = value;
    keyword_workspace.totals["cpp1"] = value;
    ++keyword_workspace.revision;
    return cpp1_verify_keyword_case(driver, keyword, marker);
}

auto cpp1_dispatch_keyword_case(int driver, int keyword) -> int {
    switch (keyword) {
    case 1:  return go2_keyword_package_case(driver);
    case 2:  return go2_keyword_import_case(driver);
    case 3:  return go2_keyword_var_case(driver);
    case 4:  return go2_keyword_const_case(driver);
    case 5:  return go2_keyword_type_case(driver);
    case 6:  return go2_keyword_func_case(driver);
    case 7:  return go2_keyword_if_case(driver);
    case 8:  return go2_keyword_else_case(driver);
    case 9:  return go2_keyword_for_case(driver);
    case 10: return go2_keyword_range_case(driver);
    case 11: return go2_keyword_switch_case(driver);
    case 12: return go2_keyword_case_case(driver);
    case 13: return go2_keyword_default_case(driver);
    case 14: return go2_keyword_fallthrough_case(driver);
    case 15: return go2_keyword_break_case(driver);
    case 16: return go2_keyword_continue_case(driver);
    case 17: return go2_keyword_goto_case(driver);
    case 18: return go2_keyword_return_case(driver);
    case 19: return go2_keyword_defer_case(driver);
    case 20: return go2_keyword_go_case(driver);
    case 21: return go2_keyword_chan_case(driver);
    case 22: return go2_keyword_select_case(driver);
    case 23: return go2_keyword_struct_case(driver);
    case 24: return go2_keyword_interface_case(driver);
    case 25: return go2_keyword_map_case(driver);
    default: return 0;
    }
}

auto cpp1_keyword_matrix_case(int keyword) -> int {
    cpp1_begin_keyword_case(1, keyword);
    keyword_workspace.totals["entry"] = 100 + keyword;
    return cpp1_dispatch_keyword_case(1, keyword);
}
//go2:cpp1-end

type KeywordTypeProbe struct {
    marker int
}

type KeywordStructProbe struct {
    marker int
}

type KeywordReader interface {
    read(value int) int
}

//go2:cpp1-begin
struct Cpp1KeywordReader final : KeywordReader {
    auto read(int value) -> int override {
        return value + 1;
    }
};

Cpp1KeywordReader keyword_reader {};
//go2:cpp1-end

cpp2_apply_keyword_case: (driver: int, keyword: int, marker: int) -> int = {
    value: int = marker + 2 * driver + keyword;
    keyword_workspace.entries[1].value = value;
    keyword_workspace.checkpoints[1] = value;
    keyword_workspace.totals["cpp2"] = value;
    keyword_workspace.revision = keyword_workspace.revision + 1;
    return cpp1_finish_keyword_case(driver, keyword, marker);
}

cpp2_keyword_matrix_case: (keyword: int) -> int = {
    cpp1_begin_keyword_case(2, keyword);
    keyword_workspace.totals["entry"] = 200 + keyword;
    return cpp1_dispatch_keyword_case(2, keyword);
}

func go2_apply_keyword_case(driver int, keyword int, marker int) int {
    value := marker + driver + keyword
    keyword_workspace.entries[2].value = value
    keyword_workspace.checkpoints[2] = value
    keyword_workspace.totals["go2"] = value
    keyword_workspace.revision = keyword_workspace.revision + 1
    return cpp2_apply_keyword_case(driver, keyword, marker)
}

func go2_keyword_package_case(driver int) int {
    return go2_apply_keyword_case(driver, 1, 101)
}

func go2_keyword_import_case(driver int) int {
    return go2_apply_keyword_case(driver, 2, 102)
}

func go2_keyword_var_case(driver int) int {
    var marker int = 103
    return go2_apply_keyword_case(driver, 3, marker)
}

func go2_keyword_const_case(driver int) int {
    const marker int = 104
    return go2_apply_keyword_case(driver, 4, marker)
}

func go2_keyword_type_case(driver int) int {
    probe := KeywordTypeProbe{}
    probe.marker = 105
    return go2_apply_keyword_case(driver, 5, probe.marker)
}

func go2_func_marker(value int) int {
    return value
}

func go2_keyword_func_case(driver int) int {
    return go2_apply_keyword_case(driver, 6, go2_func_marker(106))
}

func go2_keyword_if_case(driver int) int {
    marker := 0
    if driver > 0 {
        marker = 107
    }
    return go2_apply_keyword_case(driver, 7, marker)
}

func go2_keyword_else_case(driver int) int {
    marker := 0
    if driver == 0 {
        marker = 0
    } else {
        marker = 108
    }
    return go2_apply_keyword_case(driver, 8, marker)
}

func go2_keyword_for_case(driver int) int {
    marker := 100
    for i := 0; i < 9; i++ {
        marker = marker + 1
    }
    return go2_apply_keyword_case(driver, 9, marker)
}

func go2_keyword_range_case(driver int) int {
    values := []int{10, 20, 80}
    marker := 0
    for _, value := range values {
        marker = marker + value
    }
    return go2_apply_keyword_case(driver, 10, marker)
}

func go2_keyword_switch_case(driver int) int {
    marker := 0
    switch driver {
    case 1:
        marker = 111
    case 2:
        marker = 111
    default:
        marker = 111
    }
    return go2_apply_keyword_case(driver, 11, marker)
}

func go2_keyword_case_case(driver int) int {
    marker := 0
    switch 112 {
    case 112:
        marker = 112
    default:
        marker = 0
    }
    return go2_apply_keyword_case(driver, 12, marker)
}

func go2_keyword_default_case(driver int) int {
    marker := 0
    switch 0 {
    case 1:
        marker = 0
    default:
        marker = 113
    }
    return go2_apply_keyword_case(driver, 13, marker)
}

func go2_keyword_fallthrough_case(driver int) int {
    marker := 100
    switch 1 {
    case 1:
        marker = marker + 10
        fallthrough
    case 2:
        marker = marker + 4
    default:
        marker = 0
    }
    return go2_apply_keyword_case(driver, 14, marker)
}

func go2_keyword_break_case(driver int) int {
    marker := 0
    for {
        marker = 115
        break
    }
    return go2_apply_keyword_case(driver, 15, marker)
}

func go2_keyword_continue_case(driver int) int {
    marker := 0
    for i := 0; i < 3; i++ {
        if i == 1 {
            continue
        }
        marker = marker + 58
    }
    return go2_apply_keyword_case(driver, 16, marker)
}

func go2_keyword_goto_case(driver int) int {
    marker := 117
    goto apply
    marker = 0
apply:
    return go2_apply_keyword_case(driver, 17, marker)
}

func go2_keyword_return_case(driver int) int {
    return go2_apply_keyword_case(driver, 18, 118)
}

func go2_keyword_defer_case(driver int) int {
    defer fmt.Println("defer")
    return go2_apply_keyword_case(driver, 19, 119)
}

func go2_keyword_worker(ch chan int, marker int) {
    ch <- marker
}

func go2_keyword_go_case(driver int) int {
    ch := make(chan int, 1)
    go go2_keyword_worker(ch, 120)
    marker := <-ch
    return go2_apply_keyword_case(driver, 20, marker)
}

func go2_keyword_chan_case(driver int) int {
    ch := make(chan int, 1)
    ch <- 121
    marker := <-ch
    return go2_apply_keyword_case(driver, 21, marker)
}

func go2_keyword_select_case(driver int) int {
    ch := make(chan int, 1)
    ch <- 122
    select {
    case marker := <-ch:
        return go2_apply_keyword_case(driver, 22, marker)
    }
    return 0
}

func go2_keyword_struct_case(driver int) int {
    probe := KeywordStructProbe{}
    probe.marker = 123
    return go2_apply_keyword_case(driver, 23, probe.marker)
}

func go2_keyword_interface_case(driver int) int {
    marker := keyword_reader.read(123)
    return go2_apply_keyword_case(driver, 24, marker)
}

func go2_keyword_map_case(driver int) int {
    values := map[string]int{}
    values["marker"] = 125
    return go2_apply_keyword_case(driver, 25, values["marker"])
}

func go2_keyword_matrix_case(keyword int) int {
    cpp1_begin_keyword_case(3, keyword)
    keyword_workspace.totals["entry"] = 300 + keyword
    return cpp1_dispatch_keyword_case(3, keyword)
}

//go2:cpp1-begin
auto main() -> int {
    for (auto keyword = 1; keyword <= 25; ++keyword) {
        std::cout << cpp1_keyword_matrix_case(keyword) << "\n";
        std::cout << cpp2_keyword_matrix_case(keyword) << "\n";
        std::cout << go2_keyword_matrix_case(keyword) << "\n";
    }
}
//go2:cpp1-end
