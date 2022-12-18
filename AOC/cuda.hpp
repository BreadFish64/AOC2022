/// common/cuda.hpp
#pragma once

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Basic CUDA utilities
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <memory>
#include <fmt/core.h>

#ifdef __INTELLISENSE__

#define __host__
#define __global__
#define __device__
#define __shared__
#define __constant__
#define __launch_bounds__(...)                                                                                         \
    do {                                                                                                               \
    } while (false)

struct dim3 {
    unsigned x, y, z;
};
struct uint3 {
    unsigned x, y, z;
};
struct float4 {
    float x, y, z, w;
};
struct uchar4 {
    unsigned char x, y, z, w;
};

struct cudaSurfaceObject_t {};
struct cudaArray {};
struct cudaError_t {};
struct cudaSurfaceBoundaryMode {};

uint3 threadIdx, blockIdx;
dim3  blockDim, gridDim;

#define __syncthreads(...)                                                                                             \
    do {                                                                                                               \
    } while (false)
#define __shfl_sync(...)                                                                                               \
    do {                                                                                                               \
    } while (false)

#endif

/// <summary>
/// Takes a cudaError and prints the error string if it is not Success
/// </summary>
/// <param name="error"></param>
inline void Check(cudaError_t error) {
    if (cudaSuccess == error) return;
    fmt::print(stderr, "{}({}): {}\n", cudaGetErrorName(error), error, cudaGetErrorString(error));
}

/// <summary>
/// Functor for cudaFree
/// </summary>
struct CudaDeleter {
    template <typename T>
    void operator()(T* ptr) const {
        Check(cudaFree(ptr));
    }
    void operator()(cudaArray* ptr) const { Check(cudaFreeArray(ptr)); }
};
/// <summary>
/// Alias for unique_ptr to memory allocated through cudaMalloc
/// </summary>
/// <typeparam name="T">Type of buffer</typeparam>
template <typename T>
using UniqueCudaBuffer = std::unique_ptr<T, CudaDeleter>;
/// <summary>
/// Convenience wrapper for cudaMalloc
/// </summary>
/// <typeparam name="T">Type of buffer</typeparam>
/// <param name="count">Number of elements</param>
/// <returns>unique_ptr to buffer</returns>
template <typename T>
inline UniqueCudaBuffer<T> MakeDeviceBuffer(size_t count) {
    T* ptr{};
    Check(cudaMalloc(reinterpret_cast<void**>(&ptr), sizeof(T) * count));
    return UniqueCudaBuffer<T>{ptr};
}
template <typename T>
inline UniqueCudaBuffer<T> MakeUnifiedDeviceBuffer(size_t count) {
    T* ptr{};
    Check(cudaMallocManaged(reinterpret_cast<void**>(&ptr), sizeof(T) * count));
    return UniqueCudaBuffer<T>{ptr};
}


/// <summary>
/// Alias for unique_ptr to cudaArray
/// </summary>
/// <typeparam name="T">Type of buffer</typeparam>
using UniqueCudaArray = std::unique_ptr<cudaArray, CudaDeleter>;
/// <summary>
/// Convenience wrapper for cudaMallocArray
/// </summary>
/// <returns>unique_ptr to array</returns>
inline UniqueCudaArray MakeDeviceArray(const cudaChannelFormatDesc& desc, size_t width, size_t height = 1,
                                       unsigned int flags = 0) {
    cudaArray* arr;
    Check(cudaMallocArray(&arr, &desc, width, height, flags));
    return UniqueCudaArray{arr};
}

template <typename T>
inline __device__ T Load(cudaSurfaceObject_t surface, int x,
                         cudaSurfaceBoundaryMode boundaryMode = cudaBoundaryModeZero) {
    // For some reason the x coordinate is in bytes
    return surf1Dread<T>(surface, x * sizeof(T), boundaryMode);
}

template <typename T>
inline __device__ T Load(cudaSurfaceObject_t surface, int x, int y,
                         cudaSurfaceBoundaryMode boundaryMode = cudaBoundaryModeZero) {
    // For some reason the x coordinate is in bytes
    return surf2Dread<T>(surface, x * sizeof(T), y, boundaryMode);
}

template <typename T>
inline __device__ void Store(cudaSurfaceObject_t surface, int x, int y, T data,
                             cudaSurfaceBoundaryMode boundaryMode = cudaBoundaryModeZero) {
    // For some reason the x coordinate is in bytes
    return surf2Dwrite(data, surface, x * sizeof(T), y, boundaryMode);
}

template <typename T>
inline __device__ void Store(cudaSurfaceObject_t surface, int x, T data,
                             cudaSurfaceBoundaryMode boundaryMode = cudaBoundaryModeZero) {
    // For some reason the x coordinate is in bytes
    return surf1Dwrite(data, surface, x * sizeof(T), boundaryMode);
}
