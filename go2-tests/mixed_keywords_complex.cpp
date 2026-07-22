package main
import "fmt"

#include <array>
#include <iostream>
#include <map>
#include <string>

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

auto go2_update_keyword_workspace(int delta) -> int;

auto cpp1_reset_keyword_workspace() -> void {
    keyword_workspace.entries[0] = {"cpp1", 10};
    keyword_workspace.entries[1] = {"cpp2", 20};
    keyword_workspace.entries[2] = {"go2", 30};
    keyword_workspace.checkpoints = {1, 2, 3};
    keyword_workspace.totals.clear();
    keyword_workspace.revision = 0;
}

auto cpp1_finish_keyword_workspace() -> int {
    keyword_workspace.entries[0].value += 2;
    keyword_workspace.checkpoints[0] = keyword_workspace.entries[0].value;
    keyword_workspace.totals["cpp1"] = keyword_workspace.entries[0].value;
    ++keyword_workspace.revision;

    return
        keyword_workspace.entries[0].value
        + keyword_workspace.entries[1].value
        + keyword_workspace.entries[2].value
        + keyword_workspace.checkpoints[0]
        + keyword_workspace.checkpoints[1]
        + keyword_workspace.checkpoints[2]
        + keyword_workspace.totals["cpp1"]
        + keyword_workspace.totals["cpp2"]
        + keyword_workspace.totals["go2"]
        + keyword_workspace.revision;
}

auto cpp1_start_keyword_workspace() -> int {
    cpp1_reset_keyword_workspace();
    return go2_update_keyword_workspace(4);
}
//go2:cpp1-end

cpp2_update_keyword_workspace: (delta: int) -> int = {
    keyword_workspace.entries[1].value = keyword_workspace.entries[1].value + delta;
    keyword_workspace.checkpoints[1] = keyword_workspace.entries[1].value;
    keyword_workspace.totals["cpp2"] = keyword_workspace.entries[1].value;
    keyword_workspace.revision = keyword_workspace.revision + 1;
    return cpp1_finish_keyword_workspace();
}

func go2_update_keyword_workspace(delta int) int {
    keyword_workspace.entries[2].value = keyword_workspace.entries[2].value + delta
    keyword_workspace.checkpoints[2] = keyword_workspace.entries[2].value
    keyword_workspace.totals["go2"] = keyword_workspace.entries[2].value
    keyword_workspace.revision = keyword_workspace.revision + 1
    return cpp2_update_keyword_workspace(delta + 1)
}

//go2:cpp1-begin
auto main() -> int {
    auto const total = cpp1_start_keyword_workspace();

    std::cout
        << keyword_workspace.entries[0].language << ":" << keyword_workspace.entries[0].value << " "
        << keyword_workspace.entries[1].language << ":" << keyword_workspace.entries[1].value << " "
        << keyword_workspace.entries[2].language << ":" << keyword_workspace.entries[2].value << " | "
        << keyword_workspace.checkpoints[0] << " "
        << keyword_workspace.checkpoints[1] << " "
        << keyword_workspace.checkpoints[2] << " | "
        << keyword_workspace.totals["cpp1"] << " "
        << keyword_workspace.totals["cpp2"] << " "
        << keyword_workspace.totals["go2"] << " | "
        << keyword_workspace.revision << " | "
        << total << "\n";
}
//go2:cpp1-end
