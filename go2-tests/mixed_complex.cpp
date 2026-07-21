#include <array>
#include <iostream>
#include <map>
#include <string>
#include <vector>

struct Cpp1Profile {
    std::string name;
    int score;
};

auto cpp1_compute(Cpp1Profile* profile) -> int {
    profile->name += "!";
    profile->score += 5;
    std::vector<int> numbers {3, 4, 5};
    numbers[1] = numbers[0] + numbers[2];
    std::array<int, 3> fixed {7, 8, 9};
    std::map<std::string, int> scores;
    scores[profile->name] = profile->score + numbers[1] + fixed[2];
    return scores[profile->name];
}

Cpp2Profile: @value type = {
    public name: std::string = ();
    public score: int = ();
}

cpp2_compute: (copy profile: * Cpp2Profile) -> int = {
    profile*.name = profile*.name + "!";
    profile*.score = profile*.score + 5;
    numbers: std::vector<int> = (3, 4, 5);
    numbers[1] = numbers[0] + numbers[2];
    fixed: std::array<int, 3> = (7, 8, 9);
    scores: std::map<std::string, int> = ();
    scores[profile*.name] = profile*.score + numbers[1] + fixed[2];
    return scores[profile*.name];
}

type Go2Profile struct {
    name string
    score int
}

func go2_compute(profile *Go2Profile) int {
    profile.name = profile.name + "!"
    profile.score = profile.score + 5

    numbers := []int{3, 4, 5}
    numbers[1] = numbers[0] + numbers[2]
    fixed := [3]int{7, 8, 9}
    scores := map[string]int{}
    scores[profile.name] = profile.score + numbers[1] + fixed[2]
    return scores[profile.name]
}

auto main() -> int {
    Cpp1Profile cpp1_profile {"Ada", 10};
    Cpp2Profile cpp2_profile {};
    cpp2_profile.name = "Ada";
    cpp2_profile.score = 10;
    Go2Profile go2_profile {};
    go2_profile.name = "Ada";
    go2_profile.score = 10;

    auto cpp1_total = cpp1_compute(&cpp1_profile);
    auto cpp2_total = cpp2_compute(&cpp2_profile);
    auto go2_total = go2_compute(&go2_profile);

    std::cout
        << cpp1_profile.name << " " << cpp1_profile.score << " " << cpp1_total << " | "
        << cpp2_profile.name << " " << cpp2_profile.score << " " << cpp2_total << " | "
        << go2_profile.name << " " << go2_profile.score << " " << go2_total << "\n";
}
