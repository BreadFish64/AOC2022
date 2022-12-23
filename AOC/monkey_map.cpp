#include "pch.hpp"

#include <boost/icl/interval.hpp>

enum Facing : u8 {
    RIGHT,
    DOWN,
    LEFT,
    UP,
    MASK = 3,
};

struct Position {
    s64    y{};
    s64    x{};
    Facing facing{};
};

template <bool VERTICAL>
struct Wrapper {
    Wrapper(const Wrapper&) = default;
    Wrapper(s64 begin, s64 end, s64 idx) : interval{begin, end} {
        left_wrap = {
            .y      = VERTICAL ? end : idx,
            .x      = VERTICAL ? idx : end,
            .facing = VERTICAL ? UP : LEFT,
        };
        right_wrap = {
            .y      = VERTICAL ? begin : idx,
            .x      = VERTICAL ? idx : begin,
            .facing = VERTICAL ? DOWN : RIGHT,
        };
    };

    boost::icl::closed_interval<s64> interval;
    Position                         left_wrap;
    Position                         right_wrap;
};

void RunPart(std::vector<std::string> lines, std::span<const Wrapper<true>> y_wrappers,
             std::span<const Wrapper<false>> x_wrappers, std::string_view directions) {
    Position pos{
        .facing = RIGHT,
    };
    auto x_wrapper = x_wrappers[0];
    pos.x          = x_wrapper.interval.lower();
    auto y_wrapper = y_wrappers[pos.x];
    pos.y          = y_wrapper.interval.lower();
    while (!directions.empty()) {
        if (directions.front() == 'L') {
            pos.facing = static_cast<Facing>((pos.facing - 1) & Facing::MASK);
            directions = directions.substr(1);
        } else if (directions.front() == 'R') {
            pos.facing = static_cast<Facing>((pos.facing + 1) & Facing::MASK);
            directions = directions.substr(1);
        } else {
            s64 distance{};
            const auto [end_ptr, errc] =
                std::from_chars(directions.data(), directions.data() + directions.size(), distance);
            assert(errc == std::errc{});
            directions = {end_ptr, directions.data() + directions.size()};
            while (distance-- > 0) {
                lines[pos.y][pos.x] = std::array{'>', 'v', '<', '^'}[pos.facing];
                auto next_pos       = pos;
                switch (pos.facing) {
                    case RIGHT: {
                        ++next_pos.x;
                        if (next_pos.x > x_wrapper.interval.upper())
                            next_pos = x_wrapper.right_wrap;
                    } break;
                    case DOWN: {
                        ++next_pos.y;
                        if (next_pos.y > y_wrapper.interval.upper())
                            next_pos = y_wrapper.right_wrap;
                    } break;
                    case LEFT: {
                        --next_pos.x;
                        if (next_pos.x < x_wrapper.interval.lower())
                            next_pos = x_wrapper.left_wrap;
                    } break;
                    case UP: {
                        --next_pos.y;
                        if (next_pos.y < y_wrapper.interval.lower())
                            next_pos = y_wrapper.left_wrap;
                    } break;
                    default: assert(false); break;
                }
                if (lines[next_pos.y][next_pos.x] == '#') break;
                pos       = next_pos;
                x_wrapper = x_wrappers[pos.y];
                y_wrapper = y_wrappers[pos.x];
            }
        }
    }
    lines[pos.y][pos.x] = 'X';

    for (const auto& line : lines)
        fmt::print("{}\n", line);

    auto row    = pos.y + 1;
    auto column = pos.x + 1;
    fmt::print("Row: {}\nCol: {}\nDir: {}\nPassword: {}\n\n", row, column, pos.facing,
               row * 1000 + column * 4 + pos.facing);
}

int main() {
    std::ifstream            input_file{"input.txt"};
    std::istream&            input = input_file;
    std::vector<std::string> lines;
    {
        std::string line;
        while (std::getline(input, line) && !line.empty()) {
            lines.emplace_back(std::move(line));
            line = {};
        }
    }
    std::string direction_line;
    std::getline(input, direction_line);

    std::vector<Wrapper<false>> x_wrappers;
    s64                         max_line_width = 0;
    for (const auto& line : lines) {
        x_wrappers.emplace_back(ranges::find_if(line, [](char c) { return c != ' '; }) - line.begin(), line.size() - 1,
                                x_wrappers.size());
        max_line_width = std::max<s64>(max_line_width, line.size());
    }
    std::vector<Wrapper<true>> y_wrappers;
    for (s64 x = 0; x < max_line_width; ++x) {
        s64 y_start = 0;
        while (y_start < x_wrappers.size() && !boost::icl::contains(x_wrappers[y_start].interval, x))
            ++y_start;
        s64 y_end = y_start + 1;
        while (y_end < x_wrappers.size() && boost::icl::contains(x_wrappers[y_end].interval, x))
            ++y_end;
        y_wrappers.emplace_back(y_start, y_end - 1, y_wrappers.size());
    }

    RunPart(lines, y_wrappers, x_wrappers, direction_line);

    // Hardcoded for real input
    for (const auto upper_y : views::iota(0_s64, 50_s64)) {
        const auto lower_y = 149 - upper_y;
        auto&      upper   = x_wrappers[upper_y];
        auto&      lower   = x_wrappers[lower_y];
        upper.right_wrap   = {
              .y      = lower_y,
              .x      = lower.interval.upper(),
              .facing = LEFT,
        };
        upper.left_wrap = {
            .y      = lower_y,
            .x      = lower.interval.lower(),
            .facing = RIGHT,
        };
        lower.right_wrap = {
            .y      = upper_y,
            .x      = upper.interval.upper(),
            .facing = LEFT,
        };
        lower.left_wrap = {
            .y      = upper_y,
            .x      = upper.interval.lower(),
            .facing = RIGHT,
        };
    }
    for (const auto i : views::iota(0_s64, 50_s64)) {
        const auto x     = 100 + i;
        const auto y     = 50 + i;
        auto&      upper = y_wrappers[x];
        auto&      lower = x_wrappers[y];
        upper.right_wrap = {
            .y      = y,
            .x      = lower.interval.upper(),
            .facing = LEFT,
        };
        lower.right_wrap = {
            .y      = upper.interval.upper(),
            .x      = x,
            .facing = UP,
        };
    }
    for (const auto i : views::iota(0_s64, 50_s64)) {
        const auto x     = i;
        const auto y     = 50 + i;
        auto&      lower = y_wrappers[x];
        auto&      upper = x_wrappers[y];
        upper.left_wrap  = {
             .y      = lower.interval.lower(),
             .x      = x,
             .facing = DOWN,
        };
        lower.left_wrap = {
            .y      = y,
            .x      = upper.interval.lower(),
            .facing = RIGHT,
        };
    }

    for (const auto i : views::iota(0_s64, 50_s64)) {
        const auto x     = 50 + i;
        const auto y     = 150 + i;
        auto&      lower = x_wrappers[y];
        auto&      upper = y_wrappers[x];
        lower.right_wrap = {
            .y      = upper.interval.upper(),
            .x      = x,
            .facing = UP,
        };
        lower.left_wrap = {
            .y      = upper.interval.lower(),
            .x      = x,
            .facing = DOWN,
        };
        upper.right_wrap = {
            .y      = y,
            .x      = lower.interval.upper(),
            .facing = LEFT,
        };
        upper.left_wrap = {
            .y      = y,
            .x      = lower.interval.lower(),
            .facing = RIGHT,
        };
    }
    for (const auto lower_x : views::iota(0_s64, 50_s64)) {
        const auto upper_x = 100 + lower_x;
        auto&      lower   = y_wrappers[lower_x];
        auto&      upper   = y_wrappers[upper_x];
        lower.right_wrap   = {
              .y      = upper.interval.lower(),
              .x      = upper_x,
              .facing = DOWN,
        };
        upper.left_wrap = {
            .y      = lower.interval.upper(),
            .x      = lower_x,
            .facing = UP,
        };
    }

    RunPart(lines, y_wrappers, x_wrappers, direction_line);
}
