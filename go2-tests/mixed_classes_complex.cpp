#include <array>
#include <iostream>
#include <map>
#include <string>
#include <vector>

//go2:cpp1-begin
class Cpp1Ledger;
class Cpp2Ledger;
class Go2Ledger;

auto go2_class_run(Go2Ledger* go2, Cpp2Ledger* cpp2, Cpp1Ledger* cpp1, int delta) -> int;

class Cpp1Ledger {
    std::string owner_;
    std::vector<int> entries_;
    std::array<int, 3> checkpoints_ {};
    std::map<std::string, int> totals_;
    int revision_ = 0;

public:
    auto Reset(std::string owner) -> void {
        owner_ = std::move(owner);
        entries_ = {1, 2, 3};
        checkpoints_ = {4, 5, 6};
        totals_.clear();
        totals_[owner_] = 7;
        revision_ = 1;
    }

    auto Finalize(int value) -> int {
        entries_.push_back(value);
        checkpoints_[1] = entries_[0] + entries_[2];
        totals_[owner_] += value;
        ++revision_;
        return totals_[owner_] + checkpoints_[1] + revision_;
    }

    auto Snapshot() const -> int {
        return totals_.at(owner_) + checkpoints_[1] + revision_;
    }

    auto Start(Go2Ledger* go2, Cpp2Ledger* cpp2, int delta) -> int;
};
//go2:cpp1-end

Cpp2Ledger: type = {
    private owner: std::string = ();
    private entries: std::vector<int> = ();
    private checkpoints: std::array<int, 3> = ();
    private totals: std::map<std::string, int> = ();
    private revision: int = 0;

    public Reset: (inout this, copy owner_: std::string) = {
        owner = owner_;
        entries = (2, 4, 6);
        checkpoints = (5, 6, 7);
        totals.clear();
        totals[owner] = 11;
        revision = 1;
    }

    public Apply: (inout this, copy cpp1: * Cpp1Ledger, copy delta: int) -> int = {
        entries.push_back(delta);
        checkpoints[1] = entries[0] + entries[2];
        totals[owner] = totals[owner] + delta;
        revision = revision + 1;
        return cpp1*.Finalize(totals[owner] + checkpoints[1] + revision);
    }

    public Snapshot: (inout this) -> int = {
        return totals[owner] + checkpoints[1] + revision;
    }
}

cpp2_class_run: (copy cpp2: * Cpp2Ledger, copy cpp1: * Cpp1Ledger, copy delta: int) -> int = {
    return cpp2*.Apply(cpp1, delta);
}

type Go2Ledger class {
    owner string
    Owner string
    entries []int
    checkpoints [3]int
    totals map[string]int
    revision int

    func Reset(owner string) {
        this.owner = owner
        this.Owner = owner + "-public"
        this.entries.clear()
        this.entries.push_back(3)
        this.entries.push_back(4)
        this.entries.push_back(5)
        this.checkpoints[0] = 7
        this.checkpoints[1] = 8
        this.checkpoints[2] = 9
        this.totals.clear()
        this.totals[this.owner] = 12
        this.revision = 1
    }

    func Apply(cpp2 *Cpp2Ledger, cpp1 *Cpp1Ledger, delta int) int {
        this.entries.push_back(delta)
        var bonus int = 0
        for _, entry := range this.entries {
            if entry >= delta {
                bonus = bonus + 1
            }
        }
        this.checkpoints[1] = this.entries[0] + this.entries[2]
        this.totals[this.owner] = this.totals[this.owner] + delta
        this.revision = this.revision + 1
        return cpp2_class_run(cpp2, cpp1, this.totals[this.owner] + this.checkpoints[1] + this.revision + bonus)
    }

    func Snapshot() int {
        return this.totals[this.owner] + this.checkpoints[1] + this.revision
    }
}

func go2_class_run(go2 *Go2Ledger, cpp2 *Cpp2Ledger, cpp1 *Cpp1Ledger, delta int) int {
    return go2.Apply(cpp2, cpp1, delta)
}

//go2:cpp1-begin
auto Cpp1Ledger::Start(Go2Ledger* go2, Cpp2Ledger* cpp2, int delta) -> int {
    return go2_class_run(go2, cpp2, this, delta);
}

auto main() -> int {
    Cpp1Ledger cpp1 {};
    Cpp2Ledger cpp2 {};
    Go2Ledger go2 {};
    cpp1.Reset("cpp1");
    cpp2.Reset("cpp2");
    go2.Reset("go2");

    auto const result = cpp1.Start(&go2, &cpp2, 5);
    auto const passed = result == 63
        && cpp1.Snapshot() == 63
        && cpp2.Snapshot() == 50
        && go2.Snapshot() == 27
        && go2.Owner == "go2-public";
    std::cout << passed << " " << result << " " << cpp1.Snapshot() << " "
        << cpp2.Snapshot() << " " << go2.Snapshot() << "\n";
}
//go2:cpp1-end
