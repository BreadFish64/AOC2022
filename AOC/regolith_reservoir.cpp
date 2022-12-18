#include "pch.hpp"

struct Coord : std::pair<s64, s64> {
    using PairT = std::pair<s64, s64>;
    using PairT::PairT;
    auto&       x() { return first; }
    auto&       y() { return second; }
    auto        x() const { return first; }
    auto        y() const { return second; }
    auto&       pair() { return static_cast<PairT&>(*this); };
    const auto& pair() const { return static_cast<const PairT&>(*this); };

    Coord down() const { return {x(), y() + 1}; }
    Coord left() const { return {x() - 1, y()}; }
    Coord right() const { return {x() + 1, y()}; }
};

template <>
struct scn::scanner<Coord> : scn::empty_parser {
    template <typename Context>
    scn::error scan(Coord& val, Context& ctx) {
        return scn::scan_usertype(ctx, "{},{}", val.x(), val.y());
    }
};

int main() {
    std::ifstream                   input_file{"input.txt"};
    std::istringstream              test_input{R"(498,4 498,6 496,6
503,4 502,4 502,9 494,9
)"};
    std::istream&                   input = input_file;
    std::ofstream                   output_file{"sand.txt"};

    std::string                     line_str;
    std::vector<std::vector<Coord>> lines;
    while (std::getline(input, line_str)) {
        lines.emplace_back();
        scn::scan_list(line_str, lines.back());
    }

    Coord min{std::numeric_limits<s64>::max(), std::numeric_limits<s64>::max()};
    Coord max{std::numeric_limits<s64>::min(), std::numeric_limits<s64>::min()};
    for (const auto& line : lines) {
        for (const auto& coord : line) {
            if (coord.x() < min.x()) min.x() = coord.x();
            if (coord.y() < min.y()) min.y() = coord.y();
            if (coord.x() > max.x()) max.x() = coord.x();
            if (coord.y() > max.y()) max.y() = coord.y();
        }
    }
    min.x() = 0;
    max.x() = 1000;

    const s64 width  = max.x() + 1 - min.x();
    const s64 height = max.y() + 2;

    fmt::print("({}, {})\n", min.pair(), max.pair());
    std::string grid(width * height, ' ');
    const auto  Cell = [&](Coord c) -> char& {
        return grid[c.y() * width + c.x() - min.x()];
    };

    for (const auto& line : lines) {
        for (auto first = line.begin(), second = first + 1; second != line.end(); first = second, ++second) {
            if (first->x() == second->x()) {
                for (auto y = std::min(first->y(), second->y()); y <= std::max(first->y(), second->y()); ++y)
                    Cell({first->x(), y}) = '#';
            } else {
                for (auto x = std::min(first->x(), second->x()); x <= std::max(first->x(), second->x()); ++x)
                    Cell({x, first->y()}) = '#';
            }
        }
    }
    grid[500 - min.x()] = '+';
    const auto backup_grid = grid;

    {
        s64 rounds = -1;
        while (true) {
            rounds++;
            Coord sand_coord{500, 0};
            while (true) {
                if (sand_coord.y() >= max.y()) goto ABYSS;
                auto down = sand_coord.down();
                if (Cell(down) == ' ') {
                    sand_coord = down;
                    continue;
                }
                if (auto left = down.left(); Cell(left) == ' ') {
                    sand_coord = left;
                    continue;
                }
                if (auto right = down.right(); Cell(right) == ' ') {
                    sand_coord = right;
                    continue;
                }
                break;
            }
            Cell(sand_coord) = 'o';
        }
    ABYSS:
        for (auto y : views::iota(0_s64, height))
            fmt::print(output_file, "{}\n", std::string_view{grid}.substr(y * width, width));
        fmt::print(output_file, "\nPart 1: {}\n", rounds);
    }
    grid = backup_grid;
    {
        const auto floor  = max.y() + 1;
        s64 rounds = 0;
        while (Cell({500, 0}) == '+') {
            rounds++;
            Coord sand_coord{500, 0};
            while (true) {
                if (sand_coord.y() >= floor) break;
                auto down = sand_coord.down();
                if (Cell(down) == ' ') {
                    sand_coord = down;
                    continue;
                }
                if (auto left = down.left(); Cell(left) == ' ') {
                    sand_coord = left;
                    continue;
                }
                if (auto right = down.right(); Cell(right) == ' ') {
                    sand_coord = right;
                    continue;
                }
                break;
            }
            Cell(sand_coord) = 'o';
        }
    PILED:
        for (auto y : views::iota(0_s64, height))
            fmt::print(output_file, "{}\n", std::string_view{grid}.substr(y * width, width));
        fmt::print(output_file, "\nPart 1: {}\n", rounds);
    }
}