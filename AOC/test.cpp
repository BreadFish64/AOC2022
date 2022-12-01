#include "pch.hpp"

struct Box {
    s64 l{}, w{}, h{};
    s64 WrappingSize() const {
        const std::array areas{l * w, w * h, h * l};
        return 2LL * ranges::accumulate(areas, 0) + ranges::min(areas);
    }
    s64 BowLength() const { return l * w * h; }
    s64 RibbonLength() const {
        const std::array perimeters{l + w, w + h, h + l};
        return 2LL * ranges::min(perimeters) + BowLength();
    }
};
SCANNER(Box, "{}x{}x{}", l, w, h);

int main() {
    scn::owning_file input_file{"input.txt", "r"};
    std::vector<Box> boxes{};
    scn::scan_list(input_file, boxes);
    fmt::print("Need {} sqft of wrapping paper\n", ranges::accumulate(boxes, 0, std::plus{}, &Box::WrappingSize));
    fmt::print("Need {} ft of ribbon\n", ranges::accumulate(boxes, 0, std::plus{}, &Box::RibbonLength));
}
