cmake_minimum_required(VERSION 3.20)

project("AOC"
	CXX
)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory("scnlib")
add_subdirectory("mdspan")

add_subdirectory("AOC")
