#include "pch.hpp"

__global__ void Kernel() {
    unsigned x = DivCeil(12, 5);
	return;
}

int main() {
    fmt::print("Hello");

    return 0;
}