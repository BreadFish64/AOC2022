#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <execution>
#include <fstream>
#include <future>
#include <iterator>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_set>
#include <utility>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/preprocessor.hpp>

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/ranges.h>

#ifndef __CUDACC__
#include <experimental/mdspan>

#include <immintrin.h>

#include <scn/all.h>

#include <range/v3/all.hpp>
namespace views = ranges::views;
#endif

using namespace std::literals;

using u8   = std::uint8_t;
using u16  = std::uint16_t;
using u32  = std::uint32_t;
using u64  = std::uint64_t;
using u128 = boost::multiprecision::uint128_t;

using s8   = std::int8_t;
using s16  = std::int16_t;
using s32  = std::int32_t;
using s64  = std::int64_t;
using s128 = boost::multiprecision::int128_t;

using f32 = float;
using f64 = double;

using usize = std::size_t;
using ssize = std::make_signed_t<usize>;

constexpr usize operator""_sz(unsigned long long x) {
    return static_cast<usize>(x);
}

template <typename I>
constexpr I AlignUp(I x, I alignment) {
    x = x + alignment - 1;
    return x - (x % alignment);
}

template <typename I>
constexpr I DivCeil(I x, I y) {
    return (x + y - 1) / y;
}

#define VARIADIC_MAP(r, macro, i, elem) BOOST_PP_COMMA_IF(i) macro(elem)
#define __SCANNER_PASTE_VAL(member) val.member
#define SCANNER(T, fmt, ...)                                                                                           \
    template <>                                                                                                        \
    struct scn::scanner<T> : scn::empty_parser {                                                                       \
        template <typename Context>                                                                                    \
        error scan(T& val, Context& ctx) {                                                                             \
            return scn::scan_usertype(                                                                                 \
                ctx, (fmt),                                                                                            \
                BOOST_PP_SEQ_FOR_EACH_I(VARIADIC_MAP, __SCANNER_PASTE_VAL, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)));    \
        }                                                                                                              \
    };
