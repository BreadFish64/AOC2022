cmake_minimum_required(VERSION 3.20)

set(CMAKE_CUDA_ARCHITECTURES "75;86")
set(CMAKE_CUDA_SEPARABLE_COMPILATION ON)

project("AOC"
	CXX
	CUDA
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CUDA_STANDARD 23)

add_subdirectory("scnlib")
add_subdirectory("mdspan")

add_subdirectory("AOC")
