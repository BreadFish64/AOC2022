#include "pch.hpp"

enum : int { ROCK, PAPER, SCISSORS };
constexpr unsigned LosesAgainst(unsigned c) {
    switch (c) {
        case ROCK: return SCISSORS;
        case PAPER: return ROCK;
        case SCISSORS: return PAPER;
        default: assert(false); return 0;
    }
}
constexpr unsigned WinsAgainst(unsigned c) {
    switch (c) {
        case ROCK: return PAPER;
        case PAPER: return SCISSORS;
        case SCISSORS: return ROCK;
        default: assert(false); return 0;
    }
}
static void Run(std::span<const char> input, const int part) {
    unsigned score = 0;
    auto c     = input.begin();
    while (c != input.end()) {
        unsigned opponent = *c++ - 'A';
        unsigned you      = *c++ - 'X';
        if (part == 2) {
            switch (you) {
                case 0: you = LosesAgainst(opponent); break;
                case 1: you = opponent; break;
                case 2: you = WinsAgainst(opponent); break;
                default: assert(false);
            }
        }
        score += you + 1;
        if (you == opponent) score += 3;
        if (you == WinsAgainst(opponent)) score += 6;
    }
    fmt::print("Part {}: {}\n", part, score);
}
int main() {
    std::ifstream     input_file{"input.txt"};
    std::vector<char> input{std::istream_iterator<char>{input_file}, {}};
    Run(input, 1);
    Run(input, 2);
}
