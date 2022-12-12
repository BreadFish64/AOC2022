#include "pch.hpp"

struct Monkey {
    usize            id;
    std::vector<u64> items;
    char             operation;
    u64              rhs;
    u64              divisible;
    usize            true_id;
    usize            false_id;

    u64 inspected;
};

template <bool PART2>
usize Run(std::vector<Monkey> monkeys) {
    const u64 magic_number =
        PART2 ? ranges::accumulate(monkeys, 1_u64, std::multiplies{}, [](const Monkey& m) { return m.divisible; }) : 3;
    for (const auto round : views::iota(0, PART2 ? 10000 : 20)) {
        for (auto& monkey : monkeys) {
            for (auto item : monkey.items) {
                if (monkey.operation == '+') {
                    item += monkey.rhs;
                } else if (monkey.rhs) {
                    item *= monkey.rhs;
                } else {
                    item *= item;
                }
                if (PART2) {
                    item %= magic_number;
                } else {
                    item /= magic_number;
                }
                bool divisible = item % monkey.divisible == 0;
                monkeys[divisible ? monkey.true_id : monkey.false_id].items.emplace_back(item);
            }
            monkey.inspected += monkey.items.size();
            monkey.items.clear();
        }
    }
    u64 first{}, second{};
    for (auto& monkey : monkeys) {
        if (monkey.inspected > second) second = monkey.inspected;
        if (first < second) std::swap(first, second);
    }
    return first * second;
}

constexpr void RunItem(u64 val, usize monkey, std::span<u64, 8> inspected) {
    constexpr u64 cycle = 7_u64 * 3 * 2 * 11 * 17 * 5 * 13 * 19;
    for (int round = 0; round < 10000; ++round) {
        const auto MonkeyBuisiness = [&](int assigned_monkey, auto op, u64 rhs, u64 div, usize t, usize f) {
            if (assigned_monkey == monkey) {
                ++inspected[assigned_monkey];
                val = op(val, rhs);
                val %= cycle;
                monkey = val % div ? f : t;
            }
        };
        MonkeyBuisiness(0, std::multiplies{}, 13, 7, 1, 5);
        MonkeyBuisiness(1, std::multiplies{}, val, 3, 3, 5);
        MonkeyBuisiness(2, std::plus{}, 7, 2, 0, 4);
        MonkeyBuisiness(3, std::plus{}, 4, 11, 7, 6);
        MonkeyBuisiness(4, std::multiplies{}, 19, 17, 1, 0);
        MonkeyBuisiness(5, std::plus{}, 3, 5, 7, 3);
        MonkeyBuisiness(6, std::plus{}, 5, 13, 4, 2);
        MonkeyBuisiness(7, std::plus{}, 1, 19, 2, 6);
    }
}

u64 Run2Fast() {
    std::array<u64, 8> monkeys{};
    for (u64 v : {91, 58, 52, 69, 95, 54})
        RunItem(v, 0, monkeys);
    for (u64 v : {80, 80, 97, 84})
        RunItem(v, 1, monkeys);
    for (u64 v : {86, 92, 71})
        RunItem(v, 2, monkeys);
    for (u64 v : {96, 90, 99, 76, 79, 85, 98, 61})
        RunItem(v, 3, monkeys);
    for (u64 v : {60, 83, 68, 64, 73})
        RunItem(v, 4, monkeys);
    for (u64 v : {96, 52, 52, 94, 76, 51, 57})
        RunItem(v, 5, monkeys);
    for (u64 v : {75})
        RunItem(v, 6, monkeys);
    for (u64 v : {83, 75})
        RunItem(v, 7, monkeys);
    u64 first{}, second{};
    for (auto& monkey : monkeys) {
        if (monkey > second) second = monkey;
        if (first < second) std::swap(first, second);
    }
    return first * second;
}

int main() {
    std::ifstream       input_file{"input.txt"};
    std::string         input{std::istreambuf_iterator{input_file}, {}};
    std::vector<Monkey> monkeys{};

    std::string_view sv{input};
    while (true) {
        Monkey monkey{};
        auto   r1 = scn::scan(sv, R"(Monkey {}:
  Starting items: )",
                              monkey.id);
        if (!r1.error()) break;
        sv      = r1.reconstruct();
        auto r2 = scn::scan_list_ex(sv, monkey.items, scn::list_separator_and_until(',', '\n'));
        if (!r2.error()) break;
        sv = r2.reconstruct();
        std::string rhs;
        auto        r3 = scn::scan(sv, R"(  Operation: new = old {} {}
  Test: divisible by {}
    If true: throw to monkey {}
    If false: throw to monkey {}

)",
                                   monkey.operation, rhs, monkey.divisible, monkey.true_id, monkey.false_id);
        monkey.rhs     = isdigit(rhs.front()) ? std::stoll(rhs) : 0;
        if (!r3.error()) break;
        sv = r3.reconstruct();
        monkeys.emplace_back(std::move(monkey));
    }

    fmt::print("Part 1: {}\n", Run<false>(monkeys));
    auto t1          = std::chrono::steady_clock::now();
    auto result_flex = Run<true>(monkeys);
    auto t2          = std::chrono::steady_clock::now();
    auto result_hard = Run2Fast();
    auto t3          = std::chrono::steady_clock::now();
    fmt::print("Part 2:    {} - {}\n", result_flex, t2 - t1);
    fmt::print("Hardcoded: {} - {}\n", result_hard, t3 - t2);
}