#include "pch.hpp"

constexpr s64 FromSnafuDigit(char digit) {
    switch (digit) {
        case '=': return -2;
        case '-': return -1;
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        default: assert(false); return 0;
    }
}

constexpr char ToSnafuDigit(s64 digit) {
    switch (digit) {
        case -2: return '=';
        case -1: return '-';
        case 0: return '0';
        case 1: return '1';
        case 2: return '2';
        default: assert(false); return 0;
    }
}

constexpr s64 SnafuToInt(std::string_view snafu) {
    s64 accumulator = 0;
    for (char digit : snafu) {
        accumulator *= 5;
        accumulator += FromSnafuDigit(digit);
    }
    return accumulator;
}

constexpr auto POWERS_OF_5 = [] {
    std::array<s64, 20> powers_init{};
    powers_init[0] = 1;
    for (usize i = 1; i < powers_init.size(); ++i) {
        powers_init[i] = powers_init[i - 1] * 5;
    }
    return powers_init;
}();

constexpr auto MAX_VARIANCES = [] {
    std::string         twos = "2";
    std::array<s64, 20> max_variances{};
    for (s64& mv : max_variances) {
        mv = SnafuToInt(twos);
        twos += '2';
    }
    return max_variances;
}();

std::optional<std::string> IntToSnafuStep(s64 target, s64 current, s64 power) {
    if (power == -1) {
        if (target == current) {
            return "";
        } else {
            return std::nullopt;
        }
    }
    if (std::abs(target - current) > MAX_VARIANCES[power]) {
        return std::nullopt;
    }
    for (s64 digit = -2; digit <= 2; ++digit) {
        auto opt = IntToSnafuStep(target, current + digit * POWERS_OF_5[power], power - 1);
        if (opt) return ToSnafuDigit(digit) + *opt;
    }
    return std::nullopt;
}

std::string IntToSnafu(s64 num) {
    s64 log5 = std::log2(std::abs(num)) / std::log2(5.0);
    for (s64 digit = -2; digit <= 2; ++digit) {
        auto opt = IntToSnafuStep(num, digit * POWERS_OF_5[log5], log5 - 1);
        if (opt) return ToSnafuDigit(digit) + *opt;
    }
    return "Borked";
}

int main() {
    std::ifstream input_file{"input.txt"};
    std::istream& is = input_file;

    s64         sum = 0;
    std::string line;
    while (std::getline(is, line)) {
        sum += SnafuToInt(line);
    }
    fmt::print("Sum:          {}\n", sum);
    auto snafu_sum = IntToSnafu(sum);
    fmt::print("SNAFU:        {}\n", snafu_sum);
    fmt::print("Sanity Check: {}\n", SnafuToInt(snafu_sum));
}