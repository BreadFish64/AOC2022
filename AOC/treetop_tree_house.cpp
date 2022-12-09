#include "pch.hpp"

int main() {
    std::basic_ifstream<s8> input_file{"input.txt"};
    std::vector<s8>         input{std::istreambuf_iterator{input_file}, {}};

    const ssize width      = ranges::find(input, '\n') - input.begin();
    const ssize stride     = width + 1;
    const ssize height     = input.size() / stride;
    const auto  to_columns = views::stride(stride);

    ssize      visible   = 0;
    const auto Penetrate = [&](auto&& range) {
        int highest = 0;
        for (auto& tree : range) {
            if (std::abs(tree) > highest) {
                if (tree > 0) {
                    ++visible;
                    tree = -tree;
                }
                highest = -tree;
            }
        }
    };
    for (ssize y = 0; y < height; ++y) {
        auto&& row = input | views::drop(y * stride) | views::take(width);
        Penetrate(row);
        Penetrate(row | views::reverse);
    }
    for (ssize x = 0; x < width; ++x) {
        auto&& column = input | views::drop(x) | to_columns;
        Penetrate(column);
        Penetrate(column | views::reverse);
    }
    fmt::print("Part 1: {}\n", visible);
    for (auto& c : input)
        c = std::abs(c);

    ssize      most_scenic  = 0;
    const auto forest_begin = input.begin() + stride + 1;
    const auto forest_end   = input.end() - stride - 2;
    for (auto row_begin = forest_begin; row_begin < forest_end; row_begin += stride) {
        const auto row_end = row_begin + width - 2;
        for (auto current_tree = row_begin; current_tree < row_end; ++current_tree) {
            const auto BlocksView = [current_tree = *current_tree](s8 tree) {
                return tree >= current_tree;
            };
            ssize  score       = 1;
            auto&& right_range = ranges::subrange(current_tree, row_end) | views::drop(1);
            score *= ranges::find_if(right_range, BlocksView) - right_range.begin() + 1;
            auto&& left_range = ranges::subrange(row_begin, current_tree) | views::reverse;
            score *= ranges::find_if(left_range, BlocksView) - left_range.begin() + 1;
            auto&& down_range = ranges::subrange(current_tree, forest_end) | views::drop(stride) | to_columns;
            score *= ranges::find_if(down_range, BlocksView) - down_range.begin() + 1;
            auto&& up_range = ranges::subrange(forest_begin, current_tree) | views::reverse | views::drop(width) | to_columns;
            score *= ranges::find_if(up_range, BlocksView) - up_range.begin() + 1;
            if (score > most_scenic) most_scenic = score;
        }
    }
    fmt::print("Part 2: {}\n", most_scenic);
}
