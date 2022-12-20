#include "pch.hpp"

constexpr auto test =
    R"(Blueprint 1: Each ore robot costs 4 ore. Each clay robot costs 2 ore. Each obsidian robot costs 3 ore and 14 clay. Each geode robot costs 2 ore and 7 obsidian.
Blueprint 2: Each ore robot costs 2 ore. Each clay robot costs 3 ore. Each obsidian robot costs 3 ore and 8 clay. Each geode robot costs 3 ore and 12 obsidian.
)"sv;

enum RobotType : s8 {
    ORE,
    CLAY,
    OBSIDIAN,
    GEODE,
    END,
};

constexpr auto TakeFrom(u8& x) {
    auto idx = std::countr_zero(x);
    x ^= 1 << idx;
    return idx;
}

s32 QuadraticPlus(float a, float b, float c) {
    return (-b + std::sqrt(b * b - 4 * a * c)) / (2 * a);
}

struct Robot {
    RobotType             type{};
    Eigen::Vector<s16, 4> costs{0, 0, 0, 0};
};
struct Blueprint {
    s32 id{};

    std::array<Robot, 4> robots{
        Robot{.type = ORE},
        Robot{.type = CLAY},
        Robot{.type = OBSIDIAN},
        Robot{.type = GEODE},
    };
    Eigen::Vector<s16, 4> max_ore_costs{0, 0, 0, 0};
    s32                   minimum_obsidian_minutes;
    s32                   minimum_clay_minutes;

    void init() {
        for (const auto& robot : robots) {
            max_ore_costs = max_ore_costs.cwiseMax(robot.costs);
        }
        minimum_obsidian_minutes = QuadraticPlus(1, -3, 2 - 2 * robots[GEODE].costs[OBSIDIAN]);
        minimum_clay_minutes = minimum_obsidian_minutes + QuadraticPlus(1, -3, 2 - 2 * robots[OBSIDIAN].costs[CLAY]);
    }

    template <s32 M>
    s32 maxGeodes() const {
        s32 geodes = solveRecurse<M>({1, 0, 0, 0}, {0, 0, 0, 0});
        fmt::print("Blueprint {} results in {} geodes and quality {}\n", id, geodes, geodes * id);
        return geodes;
    }

private:
    template <s32 M>
    s16 solveRecurse(const Eigen::Vector<s16, 4> robot_counts, const Eigen::Vector<s16, 4> resources) const {
        constexpr s32 minutes_left = M - 1;
        if constexpr (minutes_left == 0) {
            return resources[GEODE] + robot_counts[GEODE];
        } else {
            if (robot_counts[CLAY] == 0 && minutes_left < minimum_clay_minutes) return 0;
            if (robot_counts[OBSIDIAN] == 0 && minutes_left < minimum_obsidian_minutes) return 0;

            const Eigen::Vector<s16, 4> excess     = robot_counts - max_ore_costs;
            s16                         max_geodes = 0;
            s8                          need       = 0;

#pragma GCC unroll 4
            for (s8 type = 0; type < END; ++type) {
                if (type != GEODE) {
                    if (excess[type] >= 0) continue;
                    if (type == CLAY && excess[OBSIDIAN] >= 0) continue;
                }
                ++need;
                const Eigen::Vector<s16, 4> new_resources = resources - robots[type].costs;
                if (new_resources.minCoeff() < 0) continue;
                if (minutes_left > 21) {
                    fmt::print("Blueprint {} - {} minutes - Robot {}\n", id, minutes_left, type);
                }
                --need;
                Eigen::Vector<s16, 4> new_robot_counts = robot_counts;
                ++new_robot_counts[type];
                max_geodes =
                    std::max(max_geodes, solveRecurse<minutes_left>(new_robot_counts, new_resources + robot_counts));
            }
            if (need > 0) {
                if (minutes_left > 21) {
                    fmt::print("Blueprint {} - {} minutes - Robot Skip\n", id, minutes_left);
                }
                max_geodes = std::max(max_geodes, solveRecurse<minutes_left>(robot_counts, resources + robot_counts));
            }
            return max_geodes;
        }
    }
};

template <>
struct scn::scanner<Blueprint> : scn::empty_parser {
    template <typename Context>
    scn::error scan(Blueprint& val, Context& ctx) {
        auto ret = scn::scan_usertype(
            ctx,
            R"(Blueprint {}: Each ore robot costs {} ore. Each clay robot costs {} ore. Each obsidian robot costs {} ore and {} clay. Each geode robot costs {} ore and {} obsidian.)",
            val.id, val.robots[ORE].costs[ORE], val.robots[CLAY].costs[ORE], val.robots[OBSIDIAN].costs[ORE],
            val.robots[OBSIDIAN].costs[CLAY], val.robots[GEODE].costs[ORE], val.robots[GEODE].costs[OBSIDIAN]);
        val.init();
        return ret;
    }
};

int main() {
    scn::owning_file       input_file{"input.txt", "r"};
    std::vector<Blueprint> blueprints;
    scn::scan_list(input_file, blueprints);

    {
        std::vector<std::future<s32>> futures;
        for (const auto& blueprint : blueprints) {
            futures.emplace_back(
                std::async(std::launch::async, [&blueprint]() { return blueprint.maxGeodes<24>() * blueprint.id; }));
        }
        std::vector<s32> qualities;
        for (auto& future : futures) {
            qualities.emplace_back(future.get());
        }
        fmt::print("{}\n", qualities);
        fmt::print("Part 1: {}\n", ranges::accumulate(qualities, s32{}));
    }
    {
        std::vector<std::future<s32>> futures;
        for (const auto& blueprint : blueprints | views::take(3)) {
            futures.emplace_back(std::async(std::launch::async, [&blueprint]() { return blueprint.maxGeodes<32>(); }));
        }
        std::vector<s32> geodes;
        for (auto& future : futures) {
            geodes.emplace_back(future.get());
        }
        fmt::print("{}\n", geodes);
        fmt::print("Part 2: {}\n", ranges::accumulate(geodes, 1_s64, std::multiplies{}));
    }
}