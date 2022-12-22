#include "pch.hpp"
#include "cuda.hpp"

#include "monkey_math.inl"


void Part2() {
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

__global__ void Part2_Kernel(s64 offset, s64* result) {
    s64 num = static_cast<s64>(blockDim.x) * static_cast<s64>(blockIdx.x) + static_cast<s64>(threadIdx.x) + offset;
    if (hardcoded_monkeys::root2(num)) *result = num;
}

void CUPart3() {
    // checked -range to -8957721231491072
    constexpr s64 range      = 1_s64 << 53;
    constexpr s64 block_size = 1024;
    constexpr s64 grid_size  = 1_s64 << 11;
    constexpr s64 thread_count  = grid_size * block_size;
    constexpr s64 start         = 3412619952128;
    s64           result        = 0;
    auto          result_buffer = MakeDeviceBuffer<s64>(1);
    Check(cudaMemcpy(result_buffer.get(), &result, sizeof(result), cudaMemcpyHostToDevice));

    for (s64 i = start; i < range; i += thread_count) {
        fmt::print("{}\n", i);
        std::fflush(stdout);
        Part2_Kernel<<<grid_size, block_size>>>(i, result_buffer.get());
        Check(cudaMemcpy(&result, result_buffer.get(), sizeof(result), cudaMemcpyDeviceToHost));
        if (result != 0) {
            fmt::print("Part 2: {}\n", result);
            std::fflush(stdout);
            break;
        }
    }
}

void CUPart2() {
    // checked -range to -8957721231491072
    constexpr s64 range         = 1_s64 << 53;
    constexpr s64 block_size    = 1024;
    constexpr s64 grid_size     = 1_s64 << 30;
    constexpr s64 thread_count  = grid_size * block_size;
    s64           result        = 0;
    auto          result_buffer = MakeDeviceBuffer<s64>(1);
    Check(cudaMemcpy(result_buffer.get(), &result, sizeof(result), cudaMemcpyHostToDevice));

    for (s64 i = -thread_count; i >= -range; i -= thread_count) {
        fmt::print("{}\n", i);
        std::fflush(stdout);
        Part2_Kernel<<<grid_size, block_size>>>(i, result_buffer.get());
        Check(cudaMemcpy(&result, result_buffer.get(), sizeof(result), cudaMemcpyDeviceToHost));
        if (result != 0) {
            fmt::print("Part 2: {}\n", result);
            std::fflush(stdout);
            break;
        }
    }
}

int main() {
    fmt::print("Part 1: {}\n", hardcoded_monkeys::root(855));
    fmt::print("Part 2: {}\n", hardcoded_monkeys::root2(3412650897408_s64));
    //CUPart3();

    for (s64 i = 3412649312256_s64 ; i < (1_s64 << 53); ++i) {
        if (hardcoded_monkeys::root2(i)) {
            fmt::print("Part 2: {}\n", i);
            std::fflush(stdout);
            break;
        }
    }
}
