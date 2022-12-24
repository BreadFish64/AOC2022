#include "pch.hpp"

#include "cuda.hpp"

#include <gsl/gsl>

std::istringstream test{R"(..............
..............
.......#......
.....###.#....
...#...#.#....
....#...##....
...#.###......
...##.#.##....
....#..#......
..............
..............
..............
)"};

using Coord = Eigen::Vector2i;

enum Direction : s8 {
    NORTH = 8,
    SOUTH,
    WEST,
    EAST,
    END,
    NO_MOVE = 127,
    NONE    = 0,
};

__device__ constexpr s32 GRID_SIZE   = 156;
__device__ constexpr s32 GRID_CENTER = 63;

__device__ constexpr s32 BLOCK_SIZE    = 32;
__device__ constexpr s32 BOUNDARY_SIZE = 3;
__device__ constexpr s32 TILE_SIZE     = BLOCK_SIZE - (2 * BOUNDARY_SIZE);

__global__ void ElfMoveKernel(cudaSurfaceObject_t elf_surface, s8 proposed_direction, s64* dirty) {
    const std::array<Coord, 4> DIR_LUT{{
        {0, -1},
        {0, 1},
        {-1, 0},
        {1, 0},
    }};

    const Eigen::Vector2i block_size{BLOCK_SIZE, BLOCK_SIZE};
    const Eigen::Vector2i tile_size{TILE_SIZE, TILE_SIZE};
    const Eigen::Vector2i boundary_size{BOUNDARY_SIZE, BOUNDARY_SIZE};

    const Eigen::Vector2i tile_idx{blockIdx.x, blockIdx.y};
    const Eigen::Vector2i thread_idx{threadIdx.x, threadIdx.y};
    const Eigen::Vector2i tile_offset = thread_idx - boundary_size;
    const Eigen::Vector2i global_idx  = tile_idx.cwiseProduct(tile_size) + tile_offset;

    __shared__ s8 elves[BLOCK_SIZE][BLOCK_SIZE];
    const auto    GetElf = [&](Coord idx) -> s8& {
        return elves[idx[1]][idx[0]];
    };
    __shared__ bool proposed_moves[BLOCK_SIZE][BLOCK_SIZE][2];
    proposed_moves[thread_idx[1]][thread_idx[0]][0] = false;
    proposed_moves[thread_idx[1]][thread_idx[0]][1] = false;

    auto& cc = GetElf(thread_idx);
    cc       = Load<s8>(elf_surface, global_idx);

    __syncthreads();

    bool dead_thread =
        thread_idx[0] == 0 || thread_idx[0] == BLOCK_SIZE - 1 || thread_idx[1] == 0 || thread_idx[1] == BLOCK_SIZE - 1;

    auto proposed_coord = thread_idx;
    if (!dead_thread && cc != NONE) {
        u8 empty_spots = 0;
        empty_spots |= GetElf(thread_idx + Coord{-1, -1}) == NONE ? 0x01 : 0;
        empty_spots |= GetElf(thread_idx + Coord{0, -1}) == NONE ? 0x02 : 0;
        empty_spots |= GetElf(thread_idx + Coord{1, -1}) == NONE ? 0x04 : 0;
        empty_spots |= GetElf(thread_idx + Coord{-1, 0}) == NONE ? 0x08 : 0;
        empty_spots |= GetElf(thread_idx + Coord{1, 0}) == NONE ? 0x10 : 0;
        empty_spots |= GetElf(thread_idx + Coord{-1, 1}) == NONE ? 0x20 : 0;
        empty_spots |= GetElf(thread_idx + Coord{0, 1}) == NONE ? 0x40 : 0;
        empty_spots |= GetElf(thread_idx + Coord{1, 1}) == NONE ? 0x80 : 0;

        u8 valid_directions = 0;
        valid_directions |= (empty_spots & 0b00000111) == 0b00000111 ? 0x01 : 0;
        valid_directions |= (empty_spots & 0b11100000) == 0b11100000 ? 0x02 : 0;
        valid_directions |= (empty_spots & 0b00101001) == 0b00101001 ? 0x04 : 0;
        valid_directions |= (empty_spots & 0b10010100) == 0b10010100 ? 0x08 : 0;

        cc = NO_MOVE;
        if (valid_directions != 0 && valid_directions != 0xF) {
            while (true) {
                if (valid_directions & (1 << (proposed_direction - NORTH))) {
                    cc = proposed_direction;
                    proposed_coord += DIR_LUT[proposed_direction - NORTH];
                    proposed_moves[proposed_coord[1]][proposed_coord[0]][proposed_direction & 1] = true;
                    break;
                }
                if (++proposed_direction == END) proposed_direction = NORTH;
            }
        }
    }

    auto& is_proposed = proposed_moves[proposed_coord[1]][proposed_coord[0]];

    __syncthreads();

    if (!dead_thread && (cc >= NORTH && cc < END)) {
        if (is_proposed[proposed_direction & 1] && !is_proposed[(proposed_direction & 1) ^ 1]) {
            *dirty                 = true;
            cc                     = NONE;
            GetElf(proposed_coord) = NO_MOVE;
        }
    }

    __syncthreads();

    dead_thread |=
        tile_offset[0] < 0 || tile_offset[0] >= TILE_SIZE || tile_offset[1] < 0 || tile_offset[1] >= TILE_SIZE;
    if (dead_thread) return;

    Store(elf_surface, global_idx, cc);
}

int main() {
    std::ifstream input_file{"input.txt"};
    std::string   input{std::istreambuf_iterator<char>{input_file}, {}};

    auto       d_elf_dirty_buffer = MakeDeviceBuffer<s64>(1);
    const auto elf_format         = cudaCreateChannelDesc(8, 0, 0, 0, cudaChannelFormatKindSigned);
    auto       d_elf_buffer       = MakeDeviceArray(elf_format, GRID_SIZE, GRID_SIZE, cudaArraySurfaceLoadStore);

    auto      start_time = std::chrono::steady_clock::now();
    const s32 width      = input.find('\n');
    const s32 stride     = width + 1;
    const s32 height     = input.size() / stride;

    std::vector<std::array<s8, GRID_SIZE>> elves(GRID_SIZE);
    for (auto& row : elves)
        std::fill(row.begin(), row.end(), NONE);

    for (s32 y = 0; y < height; ++y) {
        for (s32 x = 0; x < width; ++x) {
            if (input[y * stride + x] == '#')
                elves[y + GRID_CENTER - (height / 2)][x + GRID_CENTER - (width / 2)] = NO_MOVE;
        }
    }

    Check(cudaMemcpy2DToArray(d_elf_buffer.get(), 0, 0, elves.data(), GRID_SIZE, GRID_SIZE, GRID_SIZE,
                              cudaMemcpyHostToDevice));

    const auto PrintElves = [&elves, &d_elf_buffer] {
        for (const auto& row : elves) {
            for (auto e : row) {
                fmt::print("{}", e == NONE ? '.' : '#');
            }
            fmt::print("\n");
        }
        fmt::print("\n");
    };

    cudaResourceDesc descriptor{};
    descriptor.resType         = cudaResourceTypeArray;
    descriptor.res.array.array = d_elf_buffer.get();

    cudaSurfaceObject_t elf_surface{};
    Check(cudaCreateSurfaceObject(&elf_surface, &descriptor));
    auto cleanup_surfaces = gsl::finally([&] { Check(cudaDestroySurfaceObject(elf_surface)); });

    s64        elves_moved     = true;
    s8         round_direction = NORTH;
    s32        round_count     = 0;
    const auto DoRound         = [&] {
        ++round_count;
        elves_moved = false;

        Check(cudaMemcpy(d_elf_dirty_buffer.get(), &elves_moved, sizeof(elves_moved), cudaMemcpyHostToDevice));
        ElfMoveKernel<<<dim3{
                            DivCeil(GRID_SIZE, TILE_SIZE),
                            DivCeil(GRID_SIZE, TILE_SIZE),
                        },
                        dim3{
                            BLOCK_SIZE,
                            BLOCK_SIZE,
                        }>>>(elf_surface, round_direction, d_elf_dirty_buffer.get());
        Check(cudaPeekAtLastError());
        Check(cudaMemcpy(&elves_moved, d_elf_dirty_buffer.get(), sizeof(elves_moved), cudaMemcpyDeviceToHost));

        if (++round_direction == END) round_direction = NORTH;
    };
    while (round_count < 10)
        DoRound();
    Check(cudaMemcpy2DFromArray(elves.data(), GRID_SIZE, d_elf_buffer.get(), 0, 0, GRID_SIZE, GRID_SIZE,
                                cudaMemcpyDeviceToHost));

    s32 elf_count = 0;
    s32 min_x = GRID_SIZE, min_y = GRID_SIZE, max_x = 0, max_y = 0;
    for (s32 y = 0; y < GRID_SIZE; ++y) {
        for (s32 x = 0; x < GRID_SIZE; ++x) {
            if (elves[y][x] != NONE) {
                ++elf_count;
                min_x = std::min(min_x, x);
                min_y = std::min(min_y, y);
                max_x = std::max(max_x, x);
                max_y = std::max(max_y, y);
            }
        }
    }
    const s32 empty_cells = (max_y - min_y + 1) * (max_x - min_x + 1) - elf_count;

    while (elves_moved)
        DoRound();

    auto end_time = std::chrono::steady_clock::now();

    fmt::print("\nBounding Box: y = [{}, {}], x = [{}, {}]\n", min_y, max_y, min_x, max_x);
    fmt::print("Elves: {}\nEmpty Cells: {}\n", elf_count, empty_cells);

    fmt::print("Elves stopped after {} rounds\n", round_count);
    fmt::print("Time: {}\n", end_time - start_time);
}