// Does not work

#include "pch.hpp"

#include "cuda.hpp"

struct Interval {
    s32                      lower;
    s32                      upper;
    __host__ __device__ bool empty() const { return lower >= upper; }
};
__host__ __device__ bool Intersect(const Interval& lhs, const Interval& rhs) {
    return lhs.lower <= rhs.upper && rhs.lower <= lhs.upper;
}
__host__ __device__ bool Touch(const Interval& lhs, const Interval& rhs) {
    return Intersect(lhs, {rhs.lower - 1, rhs.upper + 1});
}

using Point = Eigen::Vector<s32, 2>;

struct Sensor {
    Point position{0, 0};
    Point closest_beacon{0, 0};
    s32   beacon_distance{};

    __host__ __device__ void updateBeaconDistance() { beacon_distance = (closest_beacon - position).cwiseAbs().sum(); }
    __host__ __device__ Interval exclusionZoneAt(s32 y) const {
        auto y_diff = std::abs(y - position[1]);
        auto x_diff = beacon_distance - y_diff;
        if (x_diff < 0) return {0,0};
        return {position[0] - x_diff, position[0] + x_diff};
    }
    __host__ __device__ Interval exclusionZoneY() const {
        return {position[1] - beacon_distance, position[1] + beacon_distance};
    }

    static bool LowerX(const Sensor& lhs, const Sensor& rhs) { return lhs.position[0] < rhs.position[0]; }
};

constexpr s32 check_row = 2000000;
constexpr s32 max_row   = 4000000;
constexpr s32 row_count = max_row + 1;

__global__ void UpdateBeacons(Sensor* sensors, u32 sensor_count) {
    u32 index = blockDim.x * blockIdx.x + threadIdx.x;
    if (index < sensor_count) sensors[index].updateBeaconDistance();
}

__global__ void Evaluate(Sensor* sensors, u32 sensor_count) {
    s32 y = blockDim.x * blockIdx.x + threadIdx.x;
    if (y > max_row) return;

    Interval intervals[16];
    u32      interval_count = 0;

    for (u32 s = 0; s < sensor_count; ++s) {
        Interval y_ranges = sensors[s].exclusionZoneY();
        if (y < y_ranges.lower || y > y_ranges.upper) continue;
        Interval interval = sensors[s].exclusionZoneAt(y);
        if (interval.lower < 0) interval.lower = 0;
        if (interval.upper > max_row) interval.upper = max_row;
        if (interval.empty()) continue;
        for (u32 i = 0; i < interval_count; ++i) {
            Interval existing_interval = intervals[i];
            if (Touch(interval, existing_interval)) {
                intervals[i] = {
                    std::min(existing_interval.lower, interval.lower),
                    std::max(existing_interval.upper, interval.upper),
                };
                goto END_SENSOR_LOOP;
            }
        }
        intervals[interval_count++] = interval;
    END_SENSOR_LOOP:
    }

    s32 maybe_empty = intervals[0].upper + 1;
    for (u32 i = 1; i < interval_count; ++i) {
        Interval interval = intervals[1];
        if (interval.lower > maybe_empty) {
            sensors[sensor_count].position = {maybe_empty, y};
            return;
        }
        maybe_empty = std::max(maybe_empty, interval.upper + 1);
    }
}

int main() {
    std::array<Sensor, 34> sensors{{
        {{2302110, 2237242}, {2348729, 1239977}}, {{47903, 2473047}, {432198, 2000000}},
        {{2363579, 1547888}, {2348729, 1239977}}, {{3619841, 520506}, {2348729, 1239977}},
        {{3941908, 3526118}, {3772294, 3485243}}, {{3206, 1564595}, {432198, 2000000}},
        {{3123411, 3392077}, {2977835, 3592946}}, {{3279053, 3984688}, {2977835, 3592946}},
        {{2968162, 3938490}, {2977835, 3592946}}, {{1772120, 2862246}, {2017966, 3158243}},
        {{3283241, 2619168}, {3172577, 2521434}}, {{2471642, 3890150}, {2977835, 3592946}},
        {{3163348, 3743489}, {2977835, 3592946}}, {{2933313, 2919047}, {3172577, 2521434}},
        {{2780640, 3629927}, {2977835, 3592946}}, {{3986978, 2079918}, {3998497, 2812428}},
        {{315464, 370694}, {550536, 260566}},     {{3957316, 3968366}, {3772294, 3485243}},
        {{2118533, 1074658}, {2348729, 1239977}}, {{3494855, 3378533}, {3772294, 3485243}},
        {{2575727, 210553}, {2348729, 1239977}},  {{3999990, 2813525}, {3998497, 2812428}},
        {{3658837, 3026912}, {3998497, 2812428}}, {{1551619, 1701155}, {2348729, 1239977}},
        {{2625855, 3330422}, {2977835, 3592946}}, {{3476946, 2445098}, {3172577, 2521434}},
        {{2915568, 1714113}, {2348729, 1239977}}, {{729668, 3723377}, {997494, 3617758}},
        {{3631681, 3801747}, {3772294, 3485243}}, {{2270816, 3197807}, {2017966, 3158243}},
        {{3999999, 2810929}, {3998497, 2812428}}, {{3978805, 3296024}, {3772294, 3485243}},
        {{1054910, 811769}, {2348729, 1239977}},  Sensor{},
    }};
    for (auto& sensor : sensors)
        sensor.updateBeaconDistance();

    auto start = std::chrono::steady_clock::now();
    std::sort(sensors.begin(), sensors.end() - 1, &Sensor::LowerX);
    s64 result{};
    {
        auto sensor_buffer = MakeDeviceBuffer<Sensor>(sensors.size());
        Check(cudaMemcpy(sensor_buffer.get(), sensors.data(), sensors.size() * sizeof(Sensor), cudaMemcpyHostToDevice));
        //UpdateBeacons<<<DivCeil<u32>(sensors.size(), 256), 256>>>(sensor_buffer.get(), sensors.size() - 1);
        Evaluate<<<DivCeil<u32>(row_count, 256), 256>>>(sensor_buffer.get(), sensors.size() - 1);
        Check(cudaMemcpy(sensors.data(), sensor_buffer.get(), sensors.size() * sizeof(Sensor), cudaMemcpyDeviceToHost));
        result = static_cast<s64>(sensors.back().position[1]) * max_row + sensors.back().position[0];
    }
    auto stop = std::chrono::steady_clock::now();
    fmt::print("Solve Time: {:.2}\n", std::chrono::duration<double, std::milli>{stop - start});
    fmt::print("Part 2: ({}, {}) -> {}\n", sensors.back().position[0], sensors.back().position[1], result);
}
