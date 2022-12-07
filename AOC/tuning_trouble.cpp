#include "pch.hpp"

[[gnu::target("avx2")]] usize DetectUniques(std::string_view input, const usize window) {
    std::array<s8, 256> counts{};
    auto                wend = input.begin();
    for (; wend != input.begin() + window; ++wend) {
        ++counts[*wend];
    }
    int multiples = ranges::count_if(counts, [](u8 count) { return count > 1; });
    for (auto wbegin = input.begin(); multiples != 0 && wend != input.end(); ++wbegin, ++wend) {
        if (--counts[*wbegin] == 1) --multiples;
        if (++counts[*wend] == 2) ++multiples;
    }
    return wend - input.begin();
}
int main() {
    std::ifstream input_file{"input.txt", std::ios::binary};
    std::string   input = {std::istreambuf_iterator{input_file}, {}};
    auto          start = std::chrono::steady_clock::now();
    usize         p1    = DetectUniques(input, 4);
    auto          stop  = std::chrono::steady_clock::now();
    fmt::print("Runtime: {}\n", stop - start);
    fmt::print("Part 1:  {}\n", p1);

    start    = std::chrono::steady_clock::now();
    usize p2 = DetectUniques(input, 14);
    stop     = std::chrono::steady_clock::now();
    fmt::print("Runtime: {}\n", stop - start);
    fmt::print("Part 2:  {}\n", p2);
}
