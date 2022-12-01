#include "pch.hpp"

int main() {
    std::ifstream input_file{"input.txt"};
    std::string   line;

    std::vector<u64> elves;
    u64              calories{};

    while (std::getline(input_file, line)) {
        if (line.empty()) {
            elves.emplace_back(calories);
            calories = 0;
        } else {
            calories += std::stoull(line);
        }
    }

    ranges::sort(elves, std::greater{});
    fmt::print("Elves:           {}\n", elves.size());
    fmt::print("Most Calories:   {}\n", elves[0]);
    fmt::print("3 Most Calories: {}\n", elves[0] + elves[1] + elves[2]);
}
