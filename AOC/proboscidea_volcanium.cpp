#include "pch.hpp"

#include <boost/container/static_vector.hpp>

using TunnelLengths = std::array<std::array<s8, 64>, 64>;

struct SlowValve {
    s16              self{-1};
    s64              flow_rate{};
    std::vector<s16> tunnels{};
};

constexpr u16 MakeId(std::string_view sv) {
    return (sv[1] - 'A') * 26 + (sv[0] - 'A');
}

constexpr auto TakeFrom(u64& x) {
    auto idx = std::countr_zero(x);
    x ^= 1_u64 << idx;
    return idx;
}

class Solver {
    TunnelLengths                           tunnel_lengths{};
    boost::container::static_vector<s8, 64> flow_rates;

    u64 PruneZeroFlow() const {
        auto reachable = (1_u64 << flow_rates.size()) - 1;
        for (usize i = 0; i < flow_rates.size(); ++i)
            if (flow_rates[i] == 0) reachable ^= 1_u64 << i;
        return reachable;
    }

    u64 GetReachable(usize src, s64 minutes_left, u64 closed_valves) const {
        auto timeout_tunnels = closed_valves;
        while (timeout_tunnels) {
            auto dst          = TakeFrom(timeout_tunnels);
            auto time_segment = tunnel_lengths[src][dst] + 1;
            if (time_segment >= minutes_left) closed_valves ^= 1_u64 << dst;
        }
        return closed_valves;
    }

    s64 Continue(usize src, s64 minutes_left, u64 reachable) const {
        s64  max_sub_relief = 0;
        auto tunnels        = reachable;
        while (tunnels) {
            auto dst = TakeFrom(tunnels);
            max_sub_relief =
                std::max(ReliefRecurse(dst, minutes_left - tunnel_lengths[src][dst] - 1, reachable ^ (1_u64 << dst)),
                         max_sub_relief);
        }
        return max_sub_relief;
    };

    s64 ReliefRecurse(usize src, s64 minutes_left, u64 reachable) const {
        reachable = GetReachable(src, minutes_left, reachable);
        return flow_rates[src] * minutes_left + Continue(src, minutes_left, reachable);
    }

    s64 ReliefWithElephantRecurse(usize you_src, usize elephant_src, s64 you_minutes_left, s64 elephant_minutes_left,
                                  u64 reachable_by_you, u64 reachable_by_elephant) const {
        assert(you_minutes_left > 0);
        assert(elephant_minutes_left > 0);
        reachable_by_you      = GetReachable(you_src, you_minutes_left, reachable_by_you);
        reachable_by_elephant = GetReachable(elephant_src, elephant_minutes_left, reachable_by_elephant);
        const s64 relief = flow_rates[you_src] * you_minutes_left + flow_rates[elephant_src] * elephant_minutes_left;
        if (!reachable_by_elephant) {
            return relief + Continue(you_src, you_minutes_left, reachable_by_you);
        } else if (!reachable_by_you) {
            return relief + Continue(elephant_src, elephant_minutes_left, reachable_by_elephant);
        } else {
            s64  max_sub_relief = 0;
            auto you_tunnels    = reachable_by_you;
            while (you_tunnels) {
                auto you_dst          = TakeFrom(you_tunnels);
                auto you_dst_mask     = ~(1_u64 << you_dst);
                auto elephant_tunnels = reachable_by_elephant & you_dst_mask;
                while (elephant_tunnels) {
                    auto elephant_dst = TakeFrom(elephant_tunnels);
                    auto dst_mask = you_dst_mask & ~(1_u64 << elephant_dst);
                    max_sub_relief =
                        std::max(ReliefWithElephantRecurse(
                                     you_dst, elephant_dst, you_minutes_left - tunnel_lengths[you_src][you_dst] - 1,
                                     elephant_minutes_left - tunnel_lengths[elephant_src][elephant_dst] - 1,
                                     reachable_by_you & dst_mask, reachable_by_elephant & dst_mask),
                                 max_sub_relief);
                }
            }
            return relief + max_sub_relief;
        }
    }

public:
    s64 Relief() const { return ReliefRecurse(0, 30, PruneZeroFlow()); }
    s64 ReliefWithElephant() const {
        auto reachable = PruneZeroFlow();
        return ReliefWithElephantRecurse(0, 0, 26, 26, reachable, reachable);
    }

    Solver(std::span<SlowValve> slow_valves) {
        assert(slow_valves.size() <= 64);
        boost::container::static_vector<u64, 64> per_valve_tunnels(slow_valves.size());
        flow_rates.resize(slow_valves.size());

        // Concentrate the IDs to < 64 and convert the valves to use bitsets for tunnels
        std::array<u8, 26 * 26> id_mapper{};
        ranges::sort(slow_valves, std::less{}, [](const SlowValve& valve) { return valve.self; });

        for (usize i = 0; i < slow_valves.size(); ++i) {
            id_mapper[slow_valves[i].self] = i;
        }
        for (usize i = 0; i < slow_valves.size(); ++i) {
            const auto& sv = slow_valves[i];
            flow_rates[i]  = sv.flow_rate;
            for (auto tunnel : sv.tunnels)
                per_valve_tunnels[i] |= 1_u64 << id_mapper[tunnel];
        }

        // Generate a map of the maximum distance between any two valves
        for (auto& row : tunnel_lengths)
            ranges::fill(row, std::numeric_limits<s8>::max());

        for (usize src = 0; src < per_valve_tunnels.size(); ++src) {
            tunnel_lengths[src][src] = 0;
            auto tunnels             = per_valve_tunnels[src];
            while (tunnels) {
                auto dst                 = TakeFrom(tunnels);
                tunnel_lengths[src][dst] = 1;
            }
        }

        // Brute forced in 4ms in debug mode
        bool dirty = true;
        while (dirty) {
            dirty = false;
            for (usize src = 0; src < per_valve_tunnels.size(); ++src) {
                for (usize dst = 0; dst < per_valve_tunnels.size(); ++dst) {
                    s8& direct = tunnel_lengths[src][dst];
                    for (usize in = 0; in < per_valve_tunnels.size(); ++in) {
                        s32 segmented = tunnel_lengths[src][in];
                        segmented += tunnel_lengths[in][dst];
                        if (segmented < direct) {
                            direct = segmented;
                            dirty  = true;
                        }
                    }
                }
            }
        }
    }
};

int main() {
    std::ifstream          input_file{"input.txt"};
    std::string            input{std::istreambuf_iterator{input_file}, {}};
    std::vector<SlowValve> valves;

    std::string_view sv{input};
    while (!sv.empty()) {
        SlowValve   valve{};
        std::string id;
        std::string ign_tunnel;
        std::string ign_lead;
        std::string ign_valve;
        auto r1 = scn::scan(sv, "Valve {} has flow rate={}; {} {} to {} ", id, valve.flow_rate, ign_tunnel, ign_lead,
                            ign_valve);
        if (!r1.error()) break;
        sv         = r1.reconstruct();
        valve.self = MakeId(id);

        std::vector<std::string> other_ids;
        auto                     r2 = scn::scan_list_ex(sv, other_ids, scn::list_separator_and_until(',', '\n'));
        for (const auto& id : other_ids)
            valve.tunnels.emplace_back(MakeId(id));
        if (!r2.error()) break;
        sv = r2.reconstruct().substr(1);
        valves.emplace_back(std::move(valve));
    }

    auto   start_time = std::chrono::steady_clock::now();
    Solver solver{std::span{valves}};
    auto   setup_time = std::chrono::steady_clock::now();
    auto   p1         = solver.Relief();
    auto   p1_time    = std::chrono::steady_clock::now();
    auto   p2         = solver.ReliefWithElephant();
    auto   p2_time    = std::chrono::steady_clock::now();
    fmt::print("Distance Pre-calculation Time: {}\nPart 1 Time: {}\nPart 2 Time: {}\n", setup_time - start_time,
               p1_time - setup_time, p2_time - p1_time);
    fmt::print("Part 1: {}\nPart 2: {}\n", p1, p2);
}
