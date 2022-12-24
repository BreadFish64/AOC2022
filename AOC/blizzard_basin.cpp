#include "pch.hpp"

std::istringstream test{R"(#.######
#>>.<^<#
#.<..<<#
#>v.><>#
#<^v^^>#
######.#
)"};

using Coord = Eigen::Vector2i;

struct Blizzard {
    Eigen::Vector2i coord;
    char            direction;
};

static Coord DirectionVec(char direction) {
    switch (direction) {
        case '>': return {1, 0};
        case 'v': return {0, 1};
        case '<': return {-1, 0};
        case '^': return {0, -1};
        default: assert(false); break;
    }
}

struct BlizzardMap {
    ssize                 width;
    ssize                 stride;
    ssize                 height;
    std::vector<Blizzard> blizzards;
    std::string           map;

    Coord beginCoord() const { return {1, 0}; }
    Coord endCoord() const { return {width - 2, height - 1}; }

    BlizzardMap(std::istream& is) {
        map    = {std::istreambuf_iterator{is}, {}};
        width  = map.find('\n');
        stride = width + 1;
        height = map.size() / stride;

        for (ssize y = 1; y < height - 1; ++y) {
            for (ssize x = 1; x < width - 1; ++x) {
                auto& cell = map[y * stride + x];
                if (">v<^"sv.contains(cell)) {
                    blizzards.emplace_back(Blizzard{
                        .coord     = {x, y},
                        .direction = cell,
                    });
                    cell = '.';
                }
            }
        }
    };

    char& Cell(std::string& map, const Coord& coord) {
        assert(coord[0] >= 0 && coord[0] < width && coord[1] >= 0 && coord[1] < height);
        return map[coord[1] * stride + coord[0]];
    }

    bool advance(bool backtrack) {
        auto new_map = map;
        for (ssize y = 1; y < height - 1; ++y) {
            for (ssize x = 1; x < width - 1; ++x) {
                Coord coord{x, y};
                for (const auto& vec : {
                         Coord{0, 0},
                         Coord{0, -1},
                         Coord{-1, 0},
                         Coord{1, 0},
                         Coord{0, 1},
                     }) {
                    if (Cell(map, coord + vec) == '&') {
                        Cell(new_map, coord) = '&';
                    }
                }
            }
        }
        if (Cell(map, {1, 1}) == '&') {
            Cell(new_map, beginCoord()) = '&';
        }
        if (Cell(map, {width - 2, height - 2}) == '&') {
            Cell(new_map, endCoord()) = '&';
        }
        map = std::move(new_map);

        for (Blizzard& blizzard : blizzards) {
            switch (blizzard.direction) {
                case '>': {
                    if (++blizzard.coord[0] >= width - 1) blizzard.coord[0] = 1;
                } break;
                case 'v': {
                    if (++blizzard.coord[1] >= height - 1) blizzard.coord[1] = 1;
                } break;
                case '<': {
                    if (--blizzard.coord[0] < 1) blizzard.coord[0] = width - 2;
                } break;
                case '^': {
                    if (--blizzard.coord[1] < 1) blizzard.coord[1] = height - 2;
                } break;
                default: assert(false); break;
            }
            Cell(map, blizzard.coord) = blizzard.direction;
        }
        // fmt::print("{}\n\n", map);
        if (backtrack) {
            return Cell(map, beginCoord()) == '&';
        } else {
            return Cell(map, endCoord()) == '&';
        }
    }

    void clear() {
        for (auto& c : map) {
            if (c == '&') c = '.';
        }
    }
};

int main() {
    std::ifstream input_file{"input.txt"};
    BlizzardMap   blizzard_map{input_file};

    auto start_time                                                = std::chrono::steady_clock::now();
    s32 rounds                                                     = 0;
    blizzard_map.Cell(blizzard_map.map, blizzard_map.beginCoord()) = '&';
    while (!blizzard_map.advance(false))
        ++rounds;
    ++rounds;
    s32 part1 = rounds;
    blizzard_map.clear();
    blizzard_map.Cell(blizzard_map.map, blizzard_map.endCoord()) = '&';
    while (!blizzard_map.advance(true))
        ++rounds;
    ++rounds;
    blizzard_map.clear();
    blizzard_map.Cell(blizzard_map.map, blizzard_map.beginCoord()) = '&';
    while (!blizzard_map.advance(false))
        ++rounds;
    ++rounds;
    auto end_time = std::chrono::steady_clock::now();

    fmt::print("Part 1:   {} rounds\n", part1);
    fmt::print("Part 2:   {} rounds\n", rounds);
    fmt::print("Simulation Time: {}\n", end_time - start_time);
}