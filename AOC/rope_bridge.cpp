#include "pch.hpp"

struct Instruction {
    char direction;
    s8   distance;
};
template <>
struct scn::scanner<Instruction> : scn::empty_parser {
    template <typename Context>
    scn::error scan(Instruction& val, Context& ctx) {
        auto ret = scn::scan_usertype(ctx, "{} {}", val.direction, val.distance);
        return ret;
    }
};

static std::pair<s32, s32> MovementVector(char direction) {
    switch (direction) {
        case 'R': return {1, 0};
        case 'L': return {-1, 0};
        case 'D': return {0, 1};
        case 'U': return {0, -1};
        default:
            assert(false);
            UB();
            break;
    }
}

static s32 Sign(s32 x) {
    return (x > 0) - (x < 0);
}

template <usize ROPE_LENGTH>
[[gnu::target("avx2")]] usize Solve(std::span<Instruction> instructions) {
    std::vector<std::bitset<512>>                tail_positions(512);
    std::array<std::pair<s32, s32>, ROPE_LENGTH> knots{};
    for (const auto [direction, distance] : instructions) {
        auto [horizontal, vertical] = MovementVector(direction);
        for (int step = 0; step < distance; ++step) {
            knots.front().first += horizontal;
            knots.front().second += vertical;
            for (auto h = knots.begin(), t = h + 1; t != knots.end(); ++h, ++t) {
                auto dx  = h->first - t->first;
                auto sdx = Sign(dx);
                auto dy  = h->second - t->second;
                auto sdy = Sign(dy);
                auto adx = std::abs(dx);
                auto ady = std::abs(dy);
                if (adx > 1 || ady > 1) {
                    t->first += sdx;
                    t->second += sdy;
                }
            }
            tail_positions[knots.back().second + 255][knots.back().first + 255] = true;
        }
    }
    return ranges::accumulate(tail_positions, usize{}, std::plus{}, &std::bitset<512>::count);
}

int main() {
    scn::owning_file         input_file{"input.txt", "r"};
    std::vector<Instruction> instructions{};
    scn::scan_list(input_file, instructions);
    fmt::print("Part 1: {}\n", Solve<2>(instructions));
    auto start = std::chrono::steady_clock::now();
    auto p2    = Solve<10>(instructions);
    auto stop  = std::chrono::steady_clock::now();
    fmt::print("Part 2: {} - {}\n", p2, stop - start);
}
