#include "pch.hpp"

#include "monkey_math.inl"

std::istringstream test{R"(root: pppw + sjmn
dbpl: 5
cczh: sllz + lgvd
zczc: 2
ptdq: humn - dvpt
dvpt: 3
lfqf: 4
humn: 5
ljgn: 2
sjmn: drzm * dbpl
sllz: 4
pppw: cczh / lfqf
lgvd: ljgn * ptdq
drzm: hmdt - zczc
hmdt: 32
)"};

constexpr s32 Id(std::string_view sv) {
    assert(sv.size() == 4);
    s32 id = 0;
    for (char c : sv) {
        id *= 26;
        id += c - 'a';
    }
    return id;
}

struct Monkey {
    s32  lhs;
    s32  rhs;
    char op;
};

s64 Solve(std::span<const Monkey, Power<usize>(26, 4)> monkeys, s32 id) {
    const auto monkey = monkeys[id];
    switch (monkey.op) {
        case '=': return monkey.lhs;
        case '+': return Solve(monkeys, monkey.lhs) + Solve(monkeys, monkey.rhs);
        case '-': return Solve(monkeys, monkey.lhs) - Solve(monkeys, monkey.rhs);
        case '*': return Solve(monkeys, monkey.lhs) * Solve(monkeys, monkey.rhs);
        case '/': return Solve(monkeys, monkey.lhs) / Solve(monkeys, monkey.rhs);
        default: assert(false); return 0;
    }
}

constexpr auto ROOT_ID = Id("root");
constexpr auto HUMN_ID = Id("humn");

constexpr void Part2() {
    constexpr s64                  range = 1_s64 << 53;
    std::ofstream                  solution{"solution.txt"};
    std::vector<std::future<void>> threads;
    for (s64 i = 0; i < 8; ++i) {
        s64 begin = i * (range / 8);
        s64 end   = (i + 1) * (range / 8);
        threads.emplace_back(std::async(
            std::launch::async,
            [&solution](s64 begin, s64 end) {
                for (s64 i = begin; i < end; ++i) {
                    if ((i & ((1_s64 << 32) - 1)) == 0) {
                        fmt::print("Progress: {}\n", i);
                    }
                    if (hardcoded_monkeys::root2(i)) {
                        fmt::print("Part 2: {}\n", i);
                        solution << i << '\n';
                        break;
                    }
                }
            },
            begin, end));
        threads.emplace_back(std::async(
            std::launch::async,
            [&solution](s64 begin, s64 end) {
                for (s64 i = begin; i > end; --i) {
                    if ((i & ((1_s64 << 32) - 1)) == 0) {
                        fmt::print("Progress: {}\n", i);
                    }
                    if (hardcoded_monkeys::root2(i)) {
                        fmt::print("Part 2: {}\n", i);
                        solution << i << '\n';
                        break;
                    }
                }
            },
            -begin, -end));
    }
    for (auto& thread : threads)
        thread.wait();
}

__global__ Part2_Kernel(s64 offset, s64* result) {
    s64 num = static_cast<s64>(blockDim.x) * static_cast<s64>(blockIdx.x) + static_cast<s64>(threadIdx.x) + offset;
    if (root2(num)) *result = num;
}

constexpr void CUPart2() {
    constexpr s64 range      = 1_s64 << 53;
    constexpr s64 block_size = 1024;
    constexpr s64 grid_size  = 1_s64 << 30;
    constexpr s64 thread_count  = grid_size * block_size;
    s64           result        = 0;
    auto          result_buffer = MakeDeviceBuffer<s64>(1);
    Check(cudaMemcpy(result_buffer.get(), &result, sizeof(result), cudaMemcpyHostToDevice));

    for (s64 i = -range + 1; i < range; i += thread_count) {
        fmt::print("{}\n", i);
        Part2_Kernel<<<grid_size, block_size>>>(i, result_buffer.get());
        Check(cudaMemcpy(&result, result_buffer.get(), sizeof(result), cudaMemcpyDeviceToHost));
        if (result != 0) {
            fmt::print("Part 2: {}\n", result);
        }
    }
}

int main() {
    std::ifstream input_file{"input.txt"};
    std::istream& input = input_file;
    std::string   line_buffer;

    std::vector<Monkey>                    monkey_buffer(Power(26, 4));
    std::span<Monkey, Power<usize>(26, 4)> monkeys{monkey_buffer};

    while (std::getline(input, line_buffer)) {
        std::string_view line{line_buffer};
        Monkey&          monkey = monkeys[Id(line.substr(0, 4))];
        if (isdigit(line[6])) {
            std::from_chars(line.data() + 6, line.data() + line.size(), monkey.lhs);
            monkey.op = '=';
        } else {
            monkey.lhs = Id(line.substr(6, 4));
            monkey.op  = line[11];
            monkey.rhs = Id(line.substr(13, 4));
        }
    }
    fmt::print("Part 1: {}\n", Solve(monkeys, ROOT_ID));
    fmt::print("Part 1: {}\n", hardcoded_monkeys::root(855));

    Part2();
}
