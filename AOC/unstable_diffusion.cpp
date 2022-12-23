#include "pch.hpp"

std::istringstream test{R"(..............
..............
.......#......
.....###.#....
...#...#.#....
....#...##....
...#.###......
...##.#.##....
....#..#......
..............
..............
..............
)"};

using Coord = Eigen::Vector<ssize, 2>;

enum Direction : s8 {
    NORTH,
    SOUTH,
    WEST,
    EAST,
    MASK    = 3,
    NO_MOVE = 4,
    NONE    = 7,
};

static const std::array<Coord, 4> DIR_LUT{{
    {0, -1},
    {0, 1},
    {-1, 0},
    {1, 0},
}};

constexpr ssize GRID_SIZE   = 1024;
constexpr ssize GRID_CENTER = GRID_SIZE / 2;

int main() {
    std::ifstream input_file{"input.txt"};
    std::string   input{std::istreambuf_iterator{input_file}, {}};

    const ssize width  = input.find('\n');
    const ssize stride = width + 1;
    const ssize height = input.size() / stride;

    std::vector<std::array<s8, GRID_SIZE>> elves(GRID_SIZE);
    for (auto& row : elves)
        ranges::fill(row, NONE);

    const auto PrintElves = [&elves] {
        return;
        for (const auto& row : elves) {
            for (auto e : row) {
                fmt::print("{}", e == NONE ? '.' : '#');
            }
            fmt::print("\n");
        }
    };

    for (ssize y = 0; y < height; ++y) {
        for (ssize x = 0; x < width; ++x) {
            if (input[y * stride + x] == '#')
                elves[y + GRID_CENTER - (height / 2)][x + GRID_CENTER - (width / 2)] = NO_MOVE;
        }
    }
    PrintElves();

    bool       elves_moved = true;
    s8 round_direction = NORTH;
    s32        round_count     = 0;
    const auto DoRound         = [&] {
        ++round_count;
        elves_moved = false;
        std::vector<std::array<s8, GRID_SIZE>> proposed_moves(GRID_SIZE);
        for (const auto y : views::iota(ssize{1}, GRID_SIZE - 1)) {
            for (const auto x : views::iota(ssize{1}, GRID_SIZE - 1)) {
                auto& cc = elves[y][x];
                if (cc == NONE) continue;
                const auto     nw = elves[y - 1][x - 1] == NONE;
                const auto     nn = elves[y - 1][x] == NONE;
                const auto     ne = elves[y - 1][x + 1] == NONE;
                const auto     ww = elves[y][x - 1] == NONE;
                const auto     ee = elves[y][x + 1] == NONE;
                const auto     sw = elves[y + 1][x - 1] == NONE;
                const auto     ss = elves[y + 1][x] == NONE;
                const auto     se = elves[y + 1][x + 1] == NONE;
                std::bitset<4> valid_directions;
                valid_directions[NORTH] = nw && nn && ne;
                valid_directions[SOUTH] = sw && ss && se;
                valid_directions[WEST]  = nw && ww && sw;
                valid_directions[EAST]  = ne && ee && se;
                if (valid_directions.none() || valid_directions.all()) {
                    cc = NO_MOVE;
                    continue;
                }
                s8 proposed_direction = round_direction;
                while (true) {
                    if (valid_directions[proposed_direction]) {
                        cc = proposed_direction;
                        Coord proposed_coord{x, y};
                        proposed_coord += DIR_LUT[proposed_direction];
                        ++proposed_moves[proposed_coord[1]][proposed_coord[0]];
                        break;
                    }
                    proposed_direction = (proposed_direction + 1) & Direction::MASK;
                }
            }
        }
        std::vector<std::array<s8, GRID_SIZE>> new_elves(GRID_SIZE);
        for (auto& row : new_elves)
            ranges::fill(row, NONE);
        for (const auto y : views::iota(ssize{1}, GRID_SIZE - 1)) {
            for (const auto x : views::iota(ssize{1}, GRID_SIZE - 1)) {
                const auto proposed_direction = elves[y][x];
                if (proposed_direction == NONE) continue;
                if (proposed_direction == NO_MOVE) {
                    new_elves[y][x] = NO_MOVE;
                    continue;
                }
                Coord proposed_coord{x, y};
                proposed_coord += DIR_LUT[proposed_direction];
                if (proposed_moves[proposed_coord[1]][proposed_coord[0]] == 1) {
                    new_elves[proposed_coord[1]][proposed_coord[0]] = NO_MOVE;
                    elves_moved                                     = true;
                } else {
                    new_elves[y][x] = NO_MOVE;
                }
            }
        }
        elves           = std::move(new_elves);
        round_direction = (round_direction + 1) & Direction::MASK;
        PrintElves();
    };
    for (const auto round : views::iota(0, 10)) {
        DoRound();
    }
    ssize elf_count = 0;
    ssize min_x = GRID_SIZE, min_y = GRID_SIZE, max_x = 0, max_y = 0;
    for (ssize y = 0; y < GRID_SIZE; ++y) {
        for (ssize x = 0; x < GRID_SIZE; ++x) {
            if (elves[y][x] != NONE) {
                ++elf_count;
                min_x = std::min(min_x, x);
                min_y = std::min(min_y, y);
                max_x = std::max(max_x, x);
                max_y = std::max(max_y, y);
            }
        }
    }
    ssize empty_cells = (max_y - min_y + 1) * (max_x - min_x + 1) - elf_count;

    fmt::print("\nBounding Box: y = [{}, {}], x = [{}, {}]\n", min_y, max_y, min_x, max_x);
    fmt::print("Elves: {}\nEmpty Cells: {}\n", elf_count, empty_cells);

    while (elves_moved)
        DoRound();
    fmt::print("Elves stopped after {} rounds\n", round_count);
}