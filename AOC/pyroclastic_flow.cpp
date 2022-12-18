#include "pch.hpp"

struct Shape {
    u32 form;
    u8  size;
};
constexpr std::array<Shape, 5> shapes{{
    {
        .form = std::bit_cast<u32>(std::array<u8, 4>{
            0b000111100,
        }),
        .size = 1,
    },
    {
        .form = std::bit_cast<u32>(std::array<u8, 4>{
            0b000010000,
            0b000111000,
            0b000010000,
        }),
        .size = 3,
    },
    {
        .form = std::bit_cast<u32>(std::array<u8, 4>{
            0b000111000,
            0b000001000,
            0b000001000,
        }),
        .size = 3,
    },
    {
        .form = std::bit_cast<u32>(std::array<u8, 4>{
            0b000100000,
            0b000100000,
            0b000100000,
            0b000100000,
        }),
        .size = 4,
    },
    {
        .form = std::bit_cast<u32>(std::array<u8, 4>{
            0b000110000,
            0b000110000,
        }),
        .size = 2,
    },
}};

usize StackHeight(std::string_view gusts, const u64 rounds) {
    usize push_idx   = 0;
    usize next_shape = 0;

    usize           highest        = 1;
    usize           chamber_size   = 1;
    usize           chamber_offset = 0;
    std::vector<u8> chamber        = {0xFF};

    auto start_time = std::chrono::steady_clock::now();
    for (u64 rock = 0; rock < rounds; ++rock) {
        if (rock % (1_u64 << 25) == 0) {
            auto time     = std::chrono::steady_clock::now();
            auto elapsed  = std::chrono::duration<double, std::ratio<3600>>(time - start_time);
            auto fraction = static_cast<double>(rock) / rounds;
            fmt::print("rock {} - {:.2}% - ETA: {}\n", rock, fraction * 100, elapsed / fraction * (1 - fraction));
        }
        auto [shape, shape_size] = shapes[next_shape++];
        if (next_shape >= shapes.size()) next_shape = 0;
        usize shape_pos    = highest + 3;
        s64   insert_count = shape_pos + 4 - chamber_size;
        if (insert_count > 0) {
            insert_count = 64 * 1024;
            chamber_size += insert_count;
            chamber.insert(chamber.end(), insert_count, 0x01);
        }
        shape_pos -= chamber_offset;

        for (int i = 0; i < 4; ++i) {
            const char push = gusts[push_idx++];
            if (push_idx >= gusts.size()) push_idx = 0;

            auto new_shape = std::rotr(shape, (push - '=') & 31);
            if ((new_shape & 0x01010101U) == 0) shape = new_shape;
        }

        shape_pos -= 3;
        u32 chamber_slice = *reinterpret_cast<u32*>(chamber.data() + shape_pos);

        while (true) {
            const u32 new_chamber_slice = chamber_slice << 8 | chamber[shape_pos - 1];
            if (shape & new_chamber_slice) {
                *reinterpret_cast<u32*>(chamber.data() + shape_pos) = chamber_slice | shape;
                highest = std::max(highest, shape_pos + chamber_offset + shape_size);
                break;
            }
            chamber_slice = new_chamber_slice;
            --shape_pos;
            {
                const char push = gusts[push_idx++];
                if (push_idx >= gusts.size()) push_idx = 0;

                auto new_shape = std::rotr(shape, (push - '=') & 31);
                if ((new_shape & chamber_slice) == 0) shape = new_shape;
            }
        }

        if (chamber.size() >= 1024 * 1024) {
            const usize idx = highest - chamber_offset - 4;
            for (auto start = chamber.begin() + idx; start != chamber.begin(); --start) {
                if (std::accumulate(start, start + 3, u8{}, std::bit_or{}) == 0xFF) {
                    chamber_offset += start - chamber.begin();
                    chamber.erase(chamber.begin(), start);
                    break;
                }
            }
        }
    }
    return highest - 1;
}

int main() {
    std::ifstream input_file{"input.txt"};
    std::string   input{std::istreambuf_iterator{input_file}, {}};

    fmt::print("Part 1: {}\n", StackHeight(input, 2022_u64));
    // The ideal solution identifies cycles between the shapes, input, and shape of the reachable pieces
    // I can brute force it in 1.3 hours though... faster than it would take to write the solution
    fmt::print("Part 2: {}\n", StackHeight(input, 1000000000000_u64));
}