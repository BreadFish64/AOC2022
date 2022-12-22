#include "pch.hpp"

using cs64 = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<
    64, 64, boost::multiprecision::signed_magnitude, boost::multiprecision::checked, void>>;

constexpr auto test = R"(1
2
-3
3
-2
0
4
)"sv;

struct Word {
    s64 original_index;
    s64 shift;
};

constexpr s64 mod(s64 dividend, s64 divisor) {
    const s64 rem = dividend % divisor;
    return rem < 0 ? rem + divisor : rem;
}

void Print(std::span<const Word> words) {
    return;
    fmt::print("{}\n",
               words | views::transform([](const auto& word) { return word.shift; }) | ranges::to<std::vector<s64>>);
}

s64 Decrypt(std::span<const s64> shifts, const s64 key, const s32 rounds) {
    const s64 word_count = shifts.size();

    std::vector<s64>  indices(word_count);
    std::vector<Word> words(word_count);
    for (s64 i = 0; i < word_count; ++i) {
        indices[i] = i;
        words[i]   = {
              .original_index = i,
              .shift          = shifts[i] * key,
        };
    }
    Print(words);

    for (const auto round : views::iota(0, rounds)) {
        for (const auto idx : indices) {
            const auto word         = words[idx];
            auto       shift        = word.shift;
            s64        shift_adjust = 0;
            if (idx + shift >= word_count) {
                shift_adjust = (idx + shift) / word_count;
            }
            if (idx + shift <= 0) {
                shift_adjust = (idx + shift) / word_count - 1;
            }
            shift += shift_adjust;
            fmt::print("{}/{}\n", shift_adjust, word_count);
            auto nidx = mod(idx + shift, word_count);
            if (nidx > idx) {
                std::rotate(words.begin() + idx, words.begin() + idx + 1, words.begin() + nidx + 1);
            }
            if (nidx < idx) {
                std::rotate(words.begin() + nidx, words.begin() + idx, words.begin() + idx + 1);
            }
            for (s64 w = std::min({idx, nidx}); w <= std::max(idx, nidx); ++w) {
                indices[words[w].original_index] = w;
            }
        }
        Print(words);
    }
    const auto zero = ranges::find(words, 0, [](const Word& w) { return w.shift; }) - words.begin();
    return words[mod(zero + 1000, word_count)].shift + words[mod(zero + 2000, word_count)].shift +
           words[mod(zero + 3000, word_count)].shift;
}

int main() {
    scn::owning_file input_file{"input.txt", "r"};
    std::vector<s64> shifts;
    scn::scan_list(input_file, shifts);

    fmt::print("Part 1: {}\n", Decrypt(shifts, 1, 1));
    // fmt::print("Part 2: {}\n", Decrypt(shifts, 811589153, 10));
}