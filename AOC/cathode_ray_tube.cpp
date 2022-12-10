#include "pch.hpp"

constexpr std::array<s64, 6> stop_cycles{20, 60, 100, 140, 180, 220};

int main() {
    std::string framebuffer(240, '.');
    auto        next_stop = stop_cycles.begin();
    s64         cycle = 0, x = 1, sum = 0;
    const auto  RunCycle = [&] {
        auto [row, col] = std::div(++cycle, 40_s64);
        if (col >= x && col < x + 3) framebuffer[cycle] = '#';
        if (next_stop != stop_cycles.end() && cycle == *(next_stop++)) sum += cycle * x;
    };
    std::ifstream input_file{"input.txt"};
    for (auto str = std::istream_iterator<std::string>{input_file}; str != decltype(str){}; ++str) {
        RunCycle();
        if (str->front() == 'a') {
            RunCycle();
            x += std::stoi(*++str);
        }
    }
    fmt::print("Part 1: {}\n\n", sum);
    for (auto y : views::iota(0_sz, 6_sz))
        fmt::print("{}\n", std::string_view{framebuffer}.substr(y * 40, 40));
}
