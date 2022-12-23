#include "pch.hpp"

struct Word {
    ssize shift;
    Word* prev;
    Word* next;

    static void Swap(Word* const lhs, Word* const rhs) {
        Word* const prev = lhs->prev;
        Word* const next = rhs->next;
        prev->next       = rhs;
        lhs->prev        = rhs;
        lhs->next        = next;
        rhs->prev        = prev;
        rhs->next        = lhs;
        next->prev       = lhs;
    }
};

ssize Decrypt(std::span<const ssize> shifts, const ssize key, const s32 rounds) {
    const ssize word_count = shifts.size();
    std::vector<Word>  words(word_count);
    for (ssize i = 0; i < word_count; ++i) {
        words[i] = {
            .shift = shifts[i] * key,
            .prev  = &words[i] - 1,
            .next  = &words[i] + 1,
        };
    }
    words.front().prev = &words.back();
    words.back().next  = &words.front();

    for (const auto round : views::iota(0, rounds)) {
        for (auto& word : words) {
            auto shift = word.shift % (word_count - 1);
            while (shift > 0) {
                Word::Swap(&word, word.next);
                --shift;
            }
            while (shift < 0) {
                Word::Swap(word.prev, &word);
                ++shift;
            }
        }
    }

    ssize       sum  = 0;
    const auto* zero = &*ranges::find(words, 0, [](const Word& w) { return w.shift; });
    for (const auto r : views::iota(0, 3)) {
        for (const auto i : views::iota(0, 1000 % word_count))
            zero = zero->next;
        sum += zero->shift;
    }
    return sum;
}

int main() {
    scn::owning_file   input_file{"input.txt", "r"};
    std::vector<ssize> shifts;
    scn::scan_list(input_file, shifts);

    fmt::print("Part 1: {}\n", Decrypt(shifts, 1, 1));
    fmt::print("Part 2: {}\n", Decrypt(shifts, 811589153, 10));
}