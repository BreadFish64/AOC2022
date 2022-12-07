#include "pch.hpp"

#include <boost/icl/interval.hpp>

struct Instructions {
    size_t count;
    size_t from;
    size_t to;
};
template <>
struct scn::scanner<Instructions> : scn::empty_parser {
    template <typename Context>
    scn::error scan(Instructions& val, Context& ctx) {
        auto ret = scn::scan_usertype(ctx, "move {} from {} to {}", val.count, val.from, val.to);
        --val.from;
        --val.to;
        return ret;
    }
};

int main() {
    std::ifstream input_file{"input.txt", std::ios::binary};
    std::string   input(std::istreambuf_iterator<char>{input_file}, {});

    std::array<std::vector<char>, 9> og_stacks;
    const auto                       Top = [](const auto& stacks) {
        std::string top;
        for (const auto& stack : stacks) {
            top += stack.back();
        }
        return top;
    };
    for (usize x = 0; x < 9; ++x) {
        for (ssize y = 7; y >= 0; --y) {
            char c = input[y * (9 * 4) + 4 * x + 1];
            if (c != ' ') {
                og_stacks[x].emplace_back(c);
            }
        }
    }
    std::string_view          instructions_str{input.data() + 4 * 9 * 9, input.size() - 4 * 9 * 9};
    std::vector<Instructions> instructions{};
    scn::scan_list(instructions_str, instructions);
    {
        auto stacks = og_stacks;
        for (auto [count, from, to] : instructions) {
            auto& from_stack  = stacks[from];
            auto& to_stack    = stacks[to];
            auto  erase_point = from_stack.rbegin() + count;
            to_stack.insert(to_stack.end(), from_stack.rbegin(), erase_point);
            from_stack.erase(erase_point.base(), from_stack.end());
        }
        fmt::print("Part 1: {}\n", Top(stacks));
    }
    {
        auto stacks = og_stacks;
        for (auto [count, from, to] : instructions) {
            auto& from_stack  = stacks[from];
            auto& to_stack    = stacks[to];
            auto  erase_point = (from_stack.rbegin() + count).base();
            to_stack.insert(to_stack.end(), erase_point, from_stack.end());
            from_stack.erase(erase_point, from_stack.end());
        }
        fmt::print("Part 2: {}\n", Top(stacks));
    }
}
