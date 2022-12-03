#include "pch.hpp"

constexpr u64 ToPriority(char c) {
    return 1_u64 << ((c >= 'a') ? (1 + c - 'a') : (27 + c - 'A'));
}
constexpr u64 CompartmentContents(std::string_view compartment) {
    return ranges::accumulate(compartment, 0_u64, std::bit_or{}, ToPriority);
}
int main() {
    std::ifstream input_file{"input.txt"};
    std::vector   input(std::istream_iterator<std::string>{input_file}, {});
    u64           sum1 = ranges::accumulate(input, 0_u64, std::plus{}, [](auto&& rucksack) {
        return std::countr_zero(CompartmentContents(rucksack | views::take(rucksack.size() / 2)) &
                                          CompartmentContents(rucksack | views::drop(rucksack.size() / 2)));
    });
    u64           sum2 = ranges::accumulate(input | views::chunk(3), 0_u64, std::plus{}, [](auto&& sacks) {
        return std::countr_zero(ranges::accumulate(sacks, ~0_u64, std::bit_and{}, CompartmentContents));
    });
    fmt::print("Part 1: {}\nPart 2: {}\n", sum1, sum2);
}
