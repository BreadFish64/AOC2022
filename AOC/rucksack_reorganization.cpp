#include "pch.hpp"

constexpr int ToPriority(char c) {
    if (c >= 'a') {
        return 1 + c - 'a';
    } else {
        return 27 + c - 'A';
    }
}
constexpr u64 CompartmentContents(auto&& compartment) {
    u64 contents = 0;
    for (char item : compartment)
        contents |= 1_u64 << ToPriority(item);
    return contents;
}
int main() {
    std::ifstream input_file{"input.txt"};
    std::vector   input(std::istream_iterator<std::string>{input_file}, {});

    u64 sum = ranges::accumulate(input | views::transform([](auto&& rucksack) {
                                     auto common = CompartmentContents(rucksack | views::take(rucksack.size() / 2));
                                     common &= CompartmentContents(rucksack | views::drop(rucksack.size() / 2));
                                     return std::countr_zero(common);
                                 }),
                                 0_u64);
    fmt::print("Part 1: {}\n", sum);
    sum = ranges::accumulate(input | views::chunk(3) | views::transform([](auto&& sacks) {
                                 auto common = ~0_u64;
                                 for (auto&& rucksack : sacks)
                                     common &= CompartmentContents(rucksack);
                                 return std::countr_zero(common);
                             }),
                             0_u64);
    fmt::print("Part 2: {}\n", sum);
}
