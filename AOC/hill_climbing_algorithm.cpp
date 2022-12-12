#include "pch.hpp"

struct Map {
    std::string map;
    const ssize width;
    const ssize stride;
    const ssize height;
    const ssize starting_index;
    const ssize ending_index;
    const ssize starting_y, starting_x;
    const ssize ending_y, ending_x;

    Map(std::istream& is)
        : map{std::istreambuf_iterator{is}, {}}, width(map.find('\n')), stride{width + 1}, height(map.size() / stride),
          starting_index(map.find('S')), ending_index(map.find('E')), starting_y{starting_index / stride},
          starting_x{starting_index % stride}, ending_y{ending_index / stride}, ending_x{ending_index & stride} {
        map[starting_index] = 'a';
        map[ending_index]   = 'z';
    }

    void FindShortestRecurse(ssize y, ssize x, int steps, char max_alt, std::span<int> lowest_steps) const {
        if (y < 0 || y >= height) return;
        if (x < 0 || x >= width) return;
        const ssize idx = y * stride + x;
        const char  c   = map[idx];
        if (c > max_alt) return;
        int& low = lowest_steps[idx];
        if (steps >= low) return;
        low     = steps++;
        max_alt = c + 1;
        FindShortestRecurse(y - 1, x, steps, max_alt, lowest_steps);
        FindShortestRecurse(y, x - 1, steps, max_alt, lowest_steps);
        FindShortestRecurse(y, x + 1, steps, max_alt, lowest_steps);
        FindShortestRecurse(y + 1, x, steps, max_alt, lowest_steps);
    }

    int Part1() const {
        std::vector<int> lowest_steps(map.size(), std::numeric_limits<int>::max());
        FindShortestRecurse(starting_y, starting_x, 0, 'a', lowest_steps);
        return lowest_steps[ending_index];
    }

    int Part2() const {
        std::vector<int> lowest_steps(map.size(), std::numeric_limits<int>::max());
        for (ssize pos = map.find('a'); pos != map.npos; pos = map.find('a', pos + 1)) {
            const auto [y, x] = std::div(pos, stride);
            FindShortestRecurse(y, x, 0, 'a', lowest_steps);
        }
        return lowest_steps[ending_index];
    }
};

int main() {
    std::ifstream input_file{"input.txt"};
    Map           m{input_file};
    fmt::print("{}\n", m.Part1());
    auto start = std::chrono::steady_clock::now();
    int  p2    = m.Part2();
    auto stop  = std::chrono::steady_clock::now();
    fmt::print("{} - {}\n", p2, stop - start);
}