#include "pch.hpp"

using CubePos = Eigen::Vector<int, 3>;
template <>
struct scn::scanner<CubePos> : scn::empty_parser {
    template <typename Context>
    scn::error scan(CubePos& val, Context& ctx) {
        return scn::scan_usertype(ctx, "{},{},{}", val[0], val[1], val[2]);
    }
};

using Droplet           = std::array<std::array<std::bitset<32>, 32>, 32>;
static const auto RANGE = views::iota(1_sz, 31_sz);

s64 SurfaceArea(const Droplet& droplet) {
    s64 surface_area = 0;
    for (auto z : RANGE) {
        for (auto y : RANGE) {
            for (auto x : RANGE) {
                if (!droplet[z][y][x]) continue;
                surface_area += !droplet[z - 1][y][x];
                surface_area += !droplet[z][y - 1][x];
                surface_area += !droplet[z][y][x - 1];
                surface_area += !droplet[z][y][x + 1];
                surface_area += !droplet[z][y + 1][x];
                surface_area += !droplet[z + 1][y][x];
            }
        }
    }
    return surface_area;
}

int main() {
    scn::owning_file     input_file{"input.txt", "r"};
    std::vector<CubePos> cubes{};
    scn::scan_list(input_file, cubes);

    auto    start_time = std::chrono::steady_clock::now();
    Droplet droplet{};
    for (CubePos cube : cubes) {
        // This is a surprise tool that will help us later
        cube += CubePos{2, 2, 2};
        droplet[cube[0]][cube[1]][cube[2]] = true;
    }
    Droplet steam{};
    // Add border for the surface area calculation
    // And another border for the steam to propagate inward from
    for (auto z : RANGE) {
        for (auto y : RANGE) {
            steam[z][y][0]  = true;
            steam[z][y][1]  = true;
            steam[z][y][30] = true;
            steam[z][y][31] = true;
        }
    }
    for (auto z : RANGE) {
        for (auto x : RANGE) {
            steam[z][0][x]  = true;
            steam[z][1][x]  = true;
            steam[z][30][x] = true;
            steam[z][31][x] = true;
        }
    }
    for (auto y : RANGE) {
        for (auto x : RANGE) {
            steam[0][y][x]  = true;
            steam[1][y][x]  = true;
            steam[30][y][x] = true;
            steam[31][y][x] = true;
        }
    }
    // Expand to fill outside space
    bool dirty     = true;
    while (dirty) {
        dirty = false;
        for (auto z : RANGE) {
            for (auto y : RANGE) {
                for (auto x : RANGE) {
                    if (!steam[z][y][x]) continue;
                    if (!droplet[z - 1][y][x] && !steam[z - 1][y][x]) {
                        steam[z - 1][y][x] = true;
                        dirty              = true;
                    };
                    if (!droplet[z][y - 1][x] && !steam[z][y - 1][x]) {
                        steam[z][y - 1][x] = true;
                        dirty              = true;
                    };
                    if (!droplet[z][y][x - 1] && !steam[z][y][x - 1]) {
                        steam[z][y][x - 1] = true;
                        dirty              = true;
                    };
                    if (!droplet[z][y][x + 1] && !steam[z][y][x + 1]) {
                        steam[z][y][x + 1] = true;
                        dirty              = true;
                    };
                    if (!droplet[z][y + 1][x] && !steam[z][y + 1][x]) {
                        steam[z][y + 1][x] = true;
                        dirty              = true;
                    };
                    if (!droplet[z][y + 1][x] && !steam[z][y + 1][x]) {
                        steam[z][y + 1][x] = true;
                        dirty              = true;
                    };
                    if (!droplet[z + 1][y][x] && !steam[z + 1][y][x]) {
                        steam[z + 1][y][x] = true;
                        dirty              = true;
                    };
                }
            }
        }
    }
    auto stop_time = std::chrono::steady_clock::now();
    fmt::print("Solve time for both parts: {}\n", stop_time - start_time);
    fmt::print("Part 1: {}\n", SurfaceArea(droplet));
    fmt::print("Part 2: {}\n", SurfaceArea(steam));
}
