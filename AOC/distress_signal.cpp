#include "pch.hpp"

#include <variant>

class Node {
    std::variant<int, std::vector<Node>> val;

    static std::pair<Node, std::string_view> CreateImpl(std::string_view sv) {
        Node n;
        if (sv.front() == '[') {
            std::vector<Node> children{};
            sv = sv.substr(1);
            while (sv.front() != ']') {
                if (sv.front() == ',') sv = sv.substr(1);
                children.emplace_back();
                std::tie(children.back(), sv) = CreateImpl(sv);
            }
            n.val = std::move(children);
            return {std::move(n), sv.substr(1)};
        }
        int val{};
        auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), val);
        n.val          = val;
        return {std::move(n), std::string_view{ptr, sv.data() + sv.size()}};
    }

public:
    Node()         = default;
    Node(Node&& n) = default;
    Node(std::string_view sv) : Node{CreateImpl(sv).first} {}
    Node& operator=(Node&&) = default;

    friend std::strong_ordering operator<=>(const Node& lhs, const Node& rhs) {
        constexpr auto NodeToVals = [](const Node& node) {
            if (const auto* pval = std::get_if<int>(&node.val); pval)
                return std::make_tuple(*pval, std::span{&node, 1});
            if (const auto* pval = std::get_if<std::vector<Node>>(&node.val); pval)
                return std::make_tuple(-1, std::span{*pval});
            assert(false);
            UB();
        };
        auto [lhs_int, lhs_nodes] = NodeToVals(lhs);
        auto [rhs_int, rhs_nodes] = NodeToVals(rhs);
        if (lhs_int >= 0 && rhs_int >= 0) return lhs_int <=> rhs_int;

        for (auto lhs_it = lhs_nodes.begin(), rhs_it = rhs_nodes.begin();
             lhs_it != lhs_nodes.end() || rhs_it != rhs_nodes.end(); ++lhs_it, ++rhs_it) {
            if (lhs_it == lhs_nodes.end()) return std::strong_ordering::less;
            if (rhs_it == rhs_nodes.end()) return std::strong_ordering::greater;
            auto ord = *lhs_it <=> *rhs_it;
            if (ord != std::strong_ordering::equal) return ord;
        }
        return std::strong_ordering::equal;
    }
    friend bool operator==(const Node& lhs, const Node& rhs) { return lhs <=> rhs == std::strong_ordering::equal; }
};

int main() {
    std::ifstream     input_file{"input.txt"};
    std::istream&     is = input_file;
    std::vector<Node> nodes;
    std::string       line;
    while (std::getline(is, line)) {
        if (line.empty()) continue;
        nodes.emplace_back(line);
    }
    u64 sum = 0;
    for (usize idx = 0; idx < nodes.size();) {
        const auto& lhs = nodes[idx++];
        const auto& rhs = nodes[idx++];
        if (lhs < rhs) {
            fmt::print("Pair {} is correct\n", idx / 2);
            sum += idx / 2;
        }
    }
    fmt::print("Part 1: {}\n", sum);

    nodes.emplace_back(Node("[[2]]"));
    nodes.emplace_back(Node("[[6]]"));
    ranges::sort(nodes, std::less{});
    usize idx1 = ranges::find(nodes, Node("[[2]]")) - nodes.begin() + 1;
    usize idx2 = ranges::find(nodes, Node("[[6]]")) - nodes.begin() + 1;
    fmt::print("Part 2: {}\n", idx1 * idx2);
    return 0;
}
