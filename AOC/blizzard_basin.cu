#include "pch.hpp"

#include "cuda.hpp"

std::istringstream test{R"(#.######
#>>.<^<#
#.<..<<#
#>v.><>#
#<^v^^>#
######.#
)"};

using Coord = Eigen::Vector2i;

struct Blizzard {
    Eigen::Vector2i coord;
    char            direction;
};

__global__ void ExpandKernel(cudaSurfaceObject_t dstSurface, cudaSurfaceObject_t srcSurface, s32 start_x, s32 start_y,
                             s32 end_x, s32 end_y, s64* round, Blizzard* blizzards, u32 blizzard_count, bool backtrack);

__global__ void Clear(cudaSurfaceObject_t dstSurface) {
    const Coord block_idx{blockIdx.x, blockIdx.y};
    const Coord block_dim{blockDim.x, blockDim.y};
    const Coord thread_idx{threadIdx.x, threadIdx.y};
    const Coord coord = block_idx.cwiseProduct(block_dim) + thread_idx;

    const char c = Load<char>(dstSurface, coord);
    if (c == 'E') Store(dstSurface, coord, '.');
}

__global__ void MoveBlizzards(cudaSurfaceObject_t dstSurface, cudaSurfaceObject_t srcSurface, s32 start_x, s32 start_y,
                              s32 end_x, s32 end_y, s64* round, Blizzard* blizzards, s32 blizzard_count, bool backtrack,
                              dim3 grid_size, dim3 block_size) {
    const auto blizzard_idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (blizzard_idx >= blizzard_count) return;
    auto& blizzard = blizzards[blizzard_idx];
    switch (blizzard.direction) {
        case '>': {
            if (++blizzard.coord[0] > end_x) blizzard.coord[0] = 1;
        } break;
        case 'v': {
            if (++blizzard.coord[1] >= end_y) blizzard.coord[1] = 1;
        } break;
        case '<': {
            if (--blizzard.coord[0] < 1) blizzard.coord[0] = end_x;
        } break;
        case '^': {
            if (--blizzard.coord[1] < 1) blizzard.coord[1] = end_y - 1;
        } break;
        default: break;
    }
    Store(dstSurface, blizzard.coord, blizzard.direction);

    if (blizzard_idx == 0) {
        ExpandKernel<<<grid_size, block_size>>>(srcSurface, dstSurface, start_x, start_y, end_x, end_y, round,
                                                blizzards, blizzard_count, backtrack);
    }
}

__global__ void ExpandKernel(cudaSurfaceObject_t dstSurface, cudaSurfaceObject_t srcSurface, s32 start_x, s32 start_y,
                             s32 end_x, s32 end_y, s64* round, Blizzard* blizzards, u32 blizzard_count,
                             bool backtrack) {
    const Coord block_idx{blockIdx.x, blockIdx.y};
    const Coord block_dim{blockDim.x, blockDim.y};
    const Coord thread_idx{threadIdx.x, threadIdx.y};
    const Coord coord = block_idx.cwiseProduct(block_dim) + thread_idx;
    Coord       start_coord{start_x, start_y};
    Coord       end_coord{end_x, end_y};
    if (backtrack) Swap(start_coord, end_coord);

    char c = Load<char>(srcSurface, coord);
    if (c == 0 || c == '#') return;
    if (coord == start_coord) {
        c = 'E';
    }
    for (const auto& vec : {
             Coord{0, -1},
             Coord{-1, 0},
             Coord{1, 0},
             Coord{0, 1},
         }) {
        if (Load<char>(srcSurface, coord + vec) == 'E') {
            c = 'E';
        }
    }
    Store(dstSurface, coord, c);
    if (coord == end_coord) {
        ++(*round);
        if (c != 'E') {
            MoveBlizzards<<<DivCeil(blizzard_count, 1024u), 1024u>>>(dstSurface, srcSurface, start_x, start_y, end_x,
                                                                     end_y, round, blizzards, blizzard_count, backtrack,
                                                                     gridDim, blockDim);
        } else {
            Clear<<<gridDim, blockDim>>>(dstSurface);
            Clear<<<gridDim, blockDim>>>(srcSurface);
        }
    }
}

struct BlizzardMap {
    s32                   width;
    s32                   stride;
    s32                   height;
    std::vector<Blizzard> h_blizzards;
    std::string           h_map;

    UniqueCudaBuffer<s64>      d_round;
    UniqueCudaBuffer<Blizzard> d_blizzards;
    UniqueCudaArray            d_map_src;
    UniqueCudaArray            d_map_dst;
    cudaSurfaceObject_t        d_map_src_surface;
    cudaSurfaceObject_t        d_map_dst_surface;

    Coord beginCoord() const { return {1, 0}; }
    Coord endCoord() const { return {width - 2, height - 1}; }

    BlizzardMap(std::istream& is) {
        h_map  = std::string{std::istreambuf_iterator<char>{is}, {}};
        width  = h_map.find('\n');
        stride = width + 1;
        height = h_map.size() / stride;

        for (s32 y = 1; y < height - 1; ++y) {
            for (s32 x = 1; x < width - 1; ++x) {
                auto& cell = h_map[y * stride + x];
                if (cell == '>' || cell == 'v' || cell == '<' || cell == '^') {
                    h_blizzards.emplace_back(Blizzard{
                        Coord{x, y},
                        cell,
                    });
                    cell = '.';
                }
            }
        }

        d_round = MakeDeviceBuffer<s64>(1);
        Check(cudaMemset(d_round.get(), 0, sizeof(s64)));
        d_blizzards = MakeDeviceBuffer<Blizzard>(h_blizzards.size());
        Check(cudaMemcpy(d_blizzards.get(), h_blizzards.data(), h_blizzards.size() * sizeof(Blizzard),
                         cudaMemcpyHostToDevice));
        const auto d_map_format = cudaCreateChannelDesc(8, 0, 0, 0, cudaChannelFormatKindSigned);
        d_map_src               = MakeDeviceArray(d_map_format, width, height, cudaArraySurfaceLoadStore);
        d_map_dst               = MakeDeviceArray(d_map_format, width, height, cudaArraySurfaceLoadStore);

        Check(cudaMemcpy2DToArray(d_map_src.get(), 0, 0, h_map.data(), stride, width, height, cudaMemcpyHostToDevice));
        Check(cudaMemcpy2DToArray(d_map_dst.get(), 0, 0, h_map.data(), stride, width, height, cudaMemcpyHostToDevice));

        cudaResourceDesc descriptor{};
        descriptor.resType = cudaResourceTypeArray;

        descriptor.res.array.array = d_map_src.get();
        Check(cudaCreateSurfaceObject(&d_map_src_surface, &descriptor));
        descriptor.res.array.array = d_map_dst.get();
        Check(cudaCreateSurfaceObject(&d_map_dst_surface, &descriptor));
    };

    char& Cell(std::string& map, const Coord& coord) {
        assert(coord[0] >= 0 && coord[0] < width && coord[1] >= 0 && coord[1] < height);
        return map[coord[1] * stride + coord[0]];
    }

    ~BlizzardMap() {
        Check(cudaDestroySurfaceObject(d_map_src_surface));
        Check(cudaDestroySurfaceObject(d_map_dst_surface));
    }

    bool advance(bool backtrack) {
        auto new_map = h_map;
        for (s32 y = 1; y < height - 1; ++y) {
            for (s32 x = 1; x < width - 1; ++x) {
                Coord coord{x, y};
                for (const auto& vec : {
                         Coord{0, 0},
                         Coord{0, -1},
                         Coord{-1, 0},
                         Coord{1, 0},
                         Coord{0, 1},
                     }) {
                    if (Cell(h_map, coord + vec) == 'E') {
                        Cell(new_map, coord) = 'E';
                    }
                }
            }
        }
        if (!backtrack || Cell(h_map, {1, 1}) == 'E') {
            Cell(new_map, beginCoord()) = 'E';
        }
        if (backtrack || Cell(h_map, {width - 2, height - 2}) == 'E') {
            Cell(new_map, endCoord()) = 'E';
        }
        h_map = std::move(new_map);

        for (Blizzard& blizzard : h_blizzards) {
            switch (blizzard.direction) {
                case '>': {
                    if (++blizzard.coord[0] >= width - 1) blizzard.coord[0] = 1;
                } break;
                case 'v': {
                    if (++blizzard.coord[1] >= height - 1) blizzard.coord[1] = 1;
                } break;
                case '<': {
                    if (--blizzard.coord[0] < 1) blizzard.coord[0] = width - 2;
                } break;
                case '^': {
                    if (--blizzard.coord[1] < 1) blizzard.coord[1] = height - 2;
                } break;
                default: assert(false); break;
            }
            Cell(h_map, blizzard.coord) = blizzard.direction;
        }
        // fmt::print("{}\n\n", map);
        if (backtrack) {
            return Cell(h_map, beginCoord()) == 'E';
        } else {
            return Cell(h_map, endCoord()) == 'E';
        }
    }

    void clear() {
        for (auto& c : h_map) {
            if (c == 'E') c = '.';
        }
    }

    s64 runCU(bool backtrack) {
        auto start_x = 1;
        auto start_y = 0;
        auto end_x   = width - 2;
        auto end_y   = height - 1;
        dim3 block_dim{
            32,
            32,
        };
        dim3 grid_dim{
            DivCeil<u32>(width, block_dim.x),
            DivCeil<u32>(height, block_dim.y),
        };
        ExpandKernel<<<grid_dim, block_dim>>>(d_map_src_surface, d_map_dst_surface, start_x, start_y, end_x, end_y,
                                              d_round.get(), d_blizzards.get(), h_blizzards.size(), backtrack);
        Check(cudaPeekAtLastError());
        s64 round = 0;
        Check(cudaMemcpy(&round, d_round.get(), sizeof(round), cudaMemcpyDeviceToHost));
        return round;
    }
};

void Run(BlizzardMap& blizzard_map, bool gpu) {
    auto start_time = std::chrono::steady_clock::now();
    s32  part1      = 0;
    s32  part15     = 0;
    s32  part2      = 0;
    if (!gpu) {
        s32 rounds = 0;
        while (!blizzard_map.advance(false))
            ++rounds;
        ++rounds;
        part1 = rounds;
        blizzard_map.clear();
        while (!blizzard_map.advance(true))
            ++rounds;
        ++rounds;
        part15 = rounds;
        blizzard_map.clear();
        while (!blizzard_map.advance(false))
            ++rounds;
        ++rounds;
        part2 = rounds;
    } else {
        part1  = blizzard_map.runCU(false);
        part15 = blizzard_map.runCU(true) - 1;
        part2  = blizzard_map.runCU(false) - 2;
    }
    auto end_time = std::chrono::steady_clock::now();

    fmt::print("\n---- {} ----\n", gpu ? "GPU" : "CPU");
    fmt::print("Part   1: {} rounds\n", part1);
    fmt::print("Part 1.5: {} rounds\n", part15);
    fmt::print("Part   2: {} rounds\n", part2);
    fmt::print("Simulation Time: {}\n", end_time - start_time);
}

int main() {
    std::ifstream input_file{"input.txt"};
    BlizzardMap   blizzard_map{input_file};
    Run(blizzard_map, false);
    Run(blizzard_map, true);
}