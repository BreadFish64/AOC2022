#include "pch.hpp"

#include <boost/icl/interval.hpp>

struct Pair {
    boost::icl::closed_interval<int> a;
    boost::icl::closed_interval<int> b;
    bool FullyContained() const { return boost::icl::contains(a, b) || boost::icl::contains(b, a); };
    bool Intersects() const { return boost::icl::intersects(a, b); };
};

template <>
struct scn::scanner<Pair> : scn::empty_parser {
    template <typename Context>
    scn::error scan(Pair& val, Context& ctx) {
        int  al{}, ar{}, bl{}, br{};
        auto ret = scn::scan_usertype(ctx, "{}-{},{}-{}", al, ar, bl, br);
        val      = {{al, ar}, {bl, br}};
        return ret;
    }
};

int main() {
    scn::owning_file  input_file{"input.txt", "r"};
    std::vector<Pair> pairs{};
    scn::scan_list(input_file, pairs);
    fmt::print("Part 1: {}\n", ranges::accumulate(pairs, 0, std::plus{}, &Pair::FullyContained));
    fmt::print("Part 2: {}\n", ranges::accumulate(pairs, 0, std::plus{}, &Pair::Intersects));
}
