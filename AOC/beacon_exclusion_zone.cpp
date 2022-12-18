#include "pch.hpp"

#include <boost/icl/interval_set.hpp>

using Interval = boost::icl::closed_interval<s64>;
using Point    = Eigen::Vector<s64, 2>;

struct Sensor {
    Point position{};
    Point closest_beacon{};
    s64   beacon_distance{};

    void updateBeaconDistance() {
        beacon_distance = ranges::accumulate(closest_beacon - position, 0_s64, std::plus{}, std::abs<s64>);
    }
    Interval exclusionZoneAt(s64 y) const {
        auto y_diff          = std::abs(y - position[1]);
        auto x_diff          = beacon_distance - y_diff;
        if (x_diff < 0) return {};
        return {position[0] - x_diff, position[0] + x_diff};
    }
    Interval exclusionZoneY() const {
        return {position[1] - beacon_distance, position[1] + beacon_distance};
    }
};

template <>
struct scn::scanner<Sensor> : scn::empty_parser {
    template <typename Context>
    scn::error scan(Sensor& val, Context& ctx) {
        auto ret = scn::scan_usertype(ctx, "Sensor at x={}, y={}: closest beacon is at x={}, y={}\n", val.position[0],
                                  val.position[1], val.closest_beacon[0], val.closest_beacon[1]);
        val.updateBeaconDistance();
        return ret;
    }
};

int main() {
    scn::owning_file    input_file{"input.txt", "r"};
    std::vector<Sensor> sensors{};
    scn::scan_list(input_file, sensors);
    constexpr s64  check_row = 2000000;
    const Interval search_space{0, 4000000};
    auto start = std::chrono::steady_clock::now();
    // Generate a map of exclusion zones at all valid y
    std::vector<boost::icl::interval_set<s64, std::less, Interval>> row_occcupancy(boost::icl::size(search_space));
    for (const auto& sensor : sensors) {
        Interval exclusion_zone_y = sensor.exclusionZoneY() & search_space;
        for (s64 y : views::iota(exclusion_zone_y.lower(), exclusion_zone_y.upper() + 1))
            row_occcupancy[y] += sensor.exclusionZoneAt(y);
    }
    // Find the gap in the search space
    Point distressed_beacon{0, search_space.lower()};
    for (; distressed_beacon[1] <= search_space.upper(); ++distressed_beacon[1]) {
        auto row = row_occcupancy[distressed_beacon[1]] & search_space;
        if (row.size() != boost::icl::size(search_space)) {
            distressed_beacon[0] = (row ^ search_space).begin()->lower();
            break;
        }
    }
    // Remove existing beacon locations to solve part 1
    for (const auto& sensor : sensors)
        if (boost::icl::contains(search_space, sensor.closest_beacon[1]))
            row_occcupancy[sensor.closest_beacon[1]] -= sensor.closest_beacon[0];
    auto stop = std::chrono::steady_clock::now();
    fmt::print("Solve Time: {:.2}\n", std::chrono::duration<double, std::milli>{stop - start});
    fmt::print("Part 1: {}\n", row_occcupancy[check_row].size());
    fmt::print("Part 2: ({}, {}) -> {}\n", distressed_beacon[0], distressed_beacon[1],
               distressed_beacon[0] * 4000000 + distressed_beacon[1]);
}
