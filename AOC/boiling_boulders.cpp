#include "pch.hpp"

using CubePos = Eigen::Vector<int, 3>;
template <>
struct scn::scanner<CubePos> : scn::empty_parser {
    template <typename Context>
    scn::error scan(CubePos& val, Context& ctx) {
        return scn::scan_usertype(ctx, "{},{},{}", val[0], val[1], val[2]);
    }
};

constexpr u32 ROW_SIZE       = 26;
using Column                 = u32;
using Droplet                = std::array<std::array<Column, ROW_SIZE>, ROW_SIZE>;
static const auto FULL_RANGE = views::iota(0u, ROW_SIZE);
static const auto RANGE      = views::iota(1u, ROW_SIZE - 1);

u32 SurfaceArea(const Droplet& droplet) {
    u32 surface_area = 0;
    for (auto z : RANGE) {
        for (auto y : RANGE) {
            const auto center = droplet[z][y];
            std::array parts{
                center & ~droplet[z - 1][y], center & ~droplet[z + 1][y],    center & ~droplet[z][y - 1],
                center & ~droplet[z][y + 1], center & ~std::rotr(center, 1), center & ~std::rotl(center, 1),
            };
            u32 row_total = ranges::accumulate(parts, 0u, std::plus{}, std::popcount<u32>);
            surface_area += row_total;
#ifndef NDEBUG
            fmt::print("{:3} ", row_total);
#endif
        }
#ifndef NDEBUG
        fmt::print("\n");
#endif
    }
    return surface_area;
}

std::tuple<Droplet, u32> GenerateSteam(const Droplet& droplet) {
    Droplet steam{};
    // Add border for the surface area calculation
    // And another border for the steam to propagate inward from
    for (auto z : FULL_RANGE) {
        for (auto y : FULL_RANGE) {
            steam[z][y] |= Column{0xC0000003};
        }
    }
    for (auto z : FULL_RANGE) {
        for (auto x : FULL_RANGE) {
            steam[z][0] |= 1 << x;
            steam[z][1] |= 1 << x;
            steam[z][ROW_SIZE - 2] |= 1 << x;
            steam[z][ROW_SIZE - 1] |= 1 << x;
        }
    }
    for (auto y : FULL_RANGE) {
        for (auto x : FULL_RANGE) {
            steam[0][y] |= 1 << x;
            steam[1][y] |= 1 << x;
            steam[ROW_SIZE - 2][y] |= 1 << x;
            steam[ROW_SIZE - 1][y] |= 1 << x;
        }
    }
    // Expand to fill outside space
    u32 rounds = 0;
    u32 dirty  = true;
    while (dirty) {
        ++rounds;
        dirty = 0;
        for (const auto iz : RANGE) {
            // Alternate between beginning and end
            const auto z = iz & 1 ? 1 + iz / 2u : (ROW_SIZE - 1) - iz / 2u;
            for (const auto iy : RANGE) {
                const auto y       = iy & 1 ? 1 + iy / 2u : (ROW_SIZE - 1) - iy / 2u;
                const auto center  = steam[z][y];
                const auto RunFace = [&dirty](u32& steam_column, const u32 droplet_column, const u32 center) {
                    auto expansion = center & ~steam_column & ~droplet_column;
                    steam_column |= expansion;
                    dirty |= expansion;
                };
                RunFace(steam[z - 1][y], droplet[z - 1][y], center);
                RunFace(steam[z + 1][y], droplet[z + 1][y], center);
                RunFace(steam[z][y - 1], droplet[z][y - 1], center);
                RunFace(steam[z][y + 1], droplet[z][y + 1], center);
                RunFace(steam[z][y], droplet[z][y], std::rotr(center, 1));
                RunFace(steam[z][y], droplet[z][y], std::rotl(center, 1));
            }
        }
    }
    return {steam, rounds};
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
        droplet[cube[0]][cube[1]] |= 1 << cube[2];
    }
    const auto p1        = SurfaceArea(droplet);
    auto [steam, rounds] = GenerateSteam(droplet);
    const auto p2        = SurfaceArea(steam);
    auto stop_time       = std::chrono::steady_clock::now();
    fmt::print("\nSolve time for both parts: {}\n", stop_time - start_time);
    fmt::print("\nPart 1: {}\n", p1);
    fmt::print("\nPart 2: {}\n", p2);
    fmt::print("\nPart 2 took {} rounds\n", rounds);
}
