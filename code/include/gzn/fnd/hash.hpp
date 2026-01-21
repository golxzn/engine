/*
 * rapidhash++ - Very fast, high quality, platform-independent hashing
 * algorithm.
 *
 * Based on 'rapidhash', by Wang Yi <ndecarli@meta.com>
 * (https://github.com/Nicoshev)
 *
 * Copyright (C) 2026 Ruslan Golovinskii
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <span>

#include "gzn/fnd/definitions.hpp"
#include "gzn/fnd/internal/str-utils.hpp"

namespace gzn::fnd {

#if defined(_MSC_VER)
#  include <intrin.h>
#  if defined(_M_X64) && !defined(_M_ARM64EC)
#    pragma intrinsic(_umul128)
#  endif
#endif

/*
 *  C++ macros.
 */

/*
 *  Endianness macros.
 */
#ifndef GZN_HASH_LITTLE_ENDIAN
#  if defined(_WIN32) || defined(__LITTLE_ENDIAN__) ||                     \
    (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#    define GZN_HASH_LITTLE_ENDIAN
#  elif defined(__BIG_ENDIAN__) ||                                      \
    (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#    define GZN_HASH_BIG_ENDIAN
#  else
#    warning "could not determine endianness! Falling back to little endian."
#    define GZN_HASH_LITTLE_ENDIAN
#  endif
#endif

/*
 *  Default secret parameters.
 */
inline constexpr std::array<u64, 8> hash_secret{
  0x2D'35'8D'CC'AA'6C'78'A5ull, 0x8B'B8'4B'93'96'2E'AC'C9ull,
  0x4B'33'A6'2E'D4'33'D4'A3ull, 0x4D'5A'2D'A5'1D'E1'AA'47ull,
  0xA0'76'1D'64'78'BD'64'2Full, 0xE7'03'7E'D1'A0'B4'28'DBull,
  0x90'ED'17'65'28'1C'38'8Cull, 0xAA'AA'AA'AA'AA'AA'AA'AAull
};

namespace util {

template<class T>
concept hashing_symbol = character_type<T> || std::is_same_v<T, std::byte>;

} // namespace util

template<util::hashing_symbol Symbol>
using hash_key_span = std::span<Symbol const>;

template<util::hashing_symbol Symbol>
struct hash_params {
  hash_key_span<Symbol> key;
  std::span<u64 const>  secret{ hash_secret };
  u64                   seed{};
};

/*
 *  64*64 -> 128bit multiply function.
 *
 *  @param A  Address of 64-bit number.
 *  @param B  Address of 64-bit number.
 *
 *  Calculates 128-bit C = *A * *B.
 *
 *  Xors and overwrites A contents with C's low 64 bits.
 *  Xors and overwrites B contents with C's high 64 bits.
 */
gzn_inline constexpr void hash_mum(u64 &A, u64 &B) noexcept {
  u64 lo, hi;
#if defined(__SIZEOF_INT128__)
  __uint128_t r{ A };
  r  *= B;
  lo  = static_cast<u64>(r);
  hi  = static_cast<u64>(r >> 64);
#elif defined(_MSC_VER) && (defined(_WIN64) || defined(_M_HYBRID_CHPE_ARM64))
#  if defined(_M_X64)
  lo = _umul128(A, B, &hi);
#  else
  hi = __umulh(A, B);
  lo = A * B;
#  endif
#else
  u64 const ha{ A >> 32 };
  u64 const hb{ B >> 32 };
  u64 const la{ (u32)A };
  u64 const lb{ (u32)B };
  u64 const rm0{ ha * lb };
  u64 const rm1{ hb * la };
  u64 const rl{ la * lb };
  u64 const t{ rl + (rm0 << 32) };
  u64       c{ t < rl };
  lo  = t + (rm1 << 32);
  c  += lo < t;
  hi  = ha * hb + (rm0 >> 32) + (rm1 >> 32) + c;
#endif
  A ^= lo;
  B ^= hi;
}

/*
 *  Multiply and xor mix function.
 *
 *  @param A  64-bit number.
 *  @param B  64-bit number.
 *
 *  Calculates 128-bit C = A * B.
 *  Returns 64-bit xor between high and low 64 bits of C.
 */
gzn_inline constexpr auto hash_mix(u64 A, u64 B) noexcept -> u64 {
  hash_mum(A, B);
  return A ^ B;
}

template<class T, util::hashing_symbol Symbol>
gzn_inline constexpr auto hash_read(hash_key_span<Symbol> p) noexcept -> T {
  if consteval {
    std::array<Symbol, sizeof(T) / sizeof(Symbol)> stash{};
    std::copy_n(std::begin(p), std::size(stash), std::begin(stash));
    return std::bit_cast<std::array<T, 1>>(stash)[0];
  } else {
    if constexpr (sizeof(Symbol) == sizeof(T)) {
      return static_cast<T>(p.front());
    } else {
      T v;
      std::memcpy(&v, std::data(p), sizeof(T));
      return v;
    }
  }
}

/*
 *  Read functions.
 */
template<util::hashing_symbol Symbol>
gzn_inline constexpr auto hash_read64(hash_key_span<Symbol> p) noexcept
  -> u64 {
  auto const v{ hash_read<u64>(p) };
#if defined(GZN_HASH_LITTLE_ENDIAN)
  return v;
#elif defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__clang__)
  return __builtin_bswap64(v);
#elif defined(_MSC_VER)
  return _byteswap_uint64(v);
#else
  return (
    ((v >> 56) & 0xff) | ((v >> 40) & 0xff00) | ((v >> 24) & 0xff0000) |
    ((v >> 8) & 0xff'00'00'00) | ((v << 8) & 0xff'00'00'00'00) |
    ((v << 24) & 0xff'00'00'00'00'00) | ((v << 40) & 0xff'00'00'00'00'00'00) |
    ((v << 56) & 0xff'00'00'00'00'00'00'00)
  );
#endif // defined(GZN_HASH_LITTLE_ENDIAN)
}

template<util::hashing_symbol Symbol>
gzn_inline constexpr auto hash_read32(hash_key_span<Symbol> p) noexcept
  -> u64 {
  auto const v{ hash_read<u32>(p) };
#if defined(GZN_HASH_LITTLE_ENDIAN)
  return static_cast<u64>(v);
#elif defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__clang__)
  return __builtin_bswap32(v);
#elif defined(_MSC_VER)
  return _byteswap_ulong((v);
#else
  return (
    ((v >> 24) & 0xff) | ((v >> 8) & 0xff00) | ((v << 8) & 0xff0000) |
    ((v << 24) & 0xff'00'00'00)
  );
#endif // defined(GZN_HASH_LITTLE_ENDIAN)
}

template<util::hashing_symbol Symbol>
gzn_inline constexpr auto hash_leaf_seed(
  size_t const              bytes_count,
  hash_params<Symbol> const in
) noexcept -> u64 {
  if (bytes_count < 16) { return in.seed; }

  auto constexpr s_in_u64{ sizeof(u64) / sizeof(Symbol) };

  auto const secret2{ in.secret[2] };
  u64        seed{ in.seed };
  seed = hash_mix(
    hash_read64(in.key.subspan(s_in_u64 * 0, s_in_u64)) ^ secret2,
    hash_read64(in.key.subspan(s_in_u64 * 1, s_in_u64)) ^ seed
  );

  if (bytes_count < 32) { return seed; }
  seed = hash_mix(
    hash_read64(in.key.subspan(s_in_u64 * 2, s_in_u64)) ^ secret2,
    hash_read64(in.key.subspan(s_in_u64 * 3, s_in_u64)) ^ seed
  );

  if (bytes_count < 48) { return seed; }
  auto const secret1{ in.secret[1] };
  seed = hash_mix(
    hash_read64(in.key.subspan(s_in_u64 * 4, s_in_u64)) ^ secret1,
    hash_read64(in.key.subspan(s_in_u64 * 5, s_in_u64)) ^ seed
  );

  if (bytes_count < 64) { return seed; }
  seed = hash_mix(
    hash_read64(in.key.subspan(s_in_u64 * 6, s_in_u64)) ^ secret1,
    hash_read64(in.key.subspan(s_in_u64 * 7, s_in_u64)) ^ seed
  );

  if (bytes_count < 80) { return seed; }
  seed = hash_mix(
    hash_read64(in.key.subspan(s_in_u64 * 8, s_in_u64)) ^ secret2,
    hash_read64(in.key.subspan(s_in_u64 * 9, s_in_u64)) ^ seed
  );

  if (bytes_count < 96) { return seed; }
  seed = hash_mix(
    hash_read64(in.key.subspan(s_in_u64 * 10, s_in_u64)) ^ secret1,
    hash_read64(in.key.subspan(s_in_u64 * 11, s_in_u64)) ^ seed
  );

  return seed;
}

/*
 *  hash main function.
 *
 *  @param key     Buffer to be hashed.
 *  @param len     @key length, in bytes.
 *  @param seed    64-bit seed used to alter the hash result predictably.
 *  @param secret  Triplet of 64-bit secrets used to alter hash result
 * predictably.
 *
 *  Returns a 64-bit hash.
 */
template<util::hashing_symbol Symbol>
gzn_inline constexpr auto hash(hash_params<Symbol> const &in) noexcept -> u64 {
  auto constexpr s_in_u64{ sizeof(u64) / sizeof(Symbol) };

  auto const bytes_count{ in.key.size_bytes() };
  auto const secret{ in.secret };
  auto       seed{ in.seed ^ hash_mix(in.seed ^ secret[2], secret[1]) };
  auto       remain_bytes{ bytes_count };
  u64        a{}, b{};
  auto       p{ in.key };

  if (bytes_count > 16) [[unlikely]] {
    auto constexpr wide_step_bytes{ 224uz };
    auto constexpr short_step_bytes{ wide_step_bytes / 2zu };
    if (bytes_count > short_step_bytes) {
      std::array<u64, 7> see{ seed, seed, seed, seed, seed, seed, seed };

      while (remain_bytes > wide_step_bytes) {
#pragma omp simd
        for (size_t i{}, j{}; i < std::size(see); ++i, j += 2) {
          see[i] = hash_mix(
            hash_read64(p.subspan(s_in_u64 * (j + 0), s_in_u64)) ^ secret[i],
            hash_read64(p.subspan(s_in_u64 * (j + 1), s_in_u64)) ^ see[i]
          );
        }

        auto constexpr wide_symbol_step{ wide_step_bytes / sizeof(Symbol) };
        p = p.subspan(wide_symbol_step, std::size(p) - wide_symbol_step);
        remain_bytes -= wide_step_bytes;
      }

      if (remain_bytes > short_step_bytes) {
#pragma omp simd
        for (size_t i{}, j{}; i < std::size(see); ++i, j += 2) {
          see[i] = hash_mix(
            hash_read64(p.subspan(s_in_u64 * (j + 0), s_in_u64)) ^ secret[i],
            hash_read64(p.subspan(s_in_u64 * (j + 1), s_in_u64)) ^ see[i]
          );
        }

        auto constexpr short_symbol_step{ short_step_bytes / sizeof(Symbol) };
        p = p.subspan(short_symbol_step, std::size(p) - short_symbol_step);
        remain_bytes -= short_step_bytes;
      }
      see[0] ^= see[1];
      see[2] ^= see[3];
      see[4] ^= see[5];
      see[0] ^= see[6];
      see[2] ^= see[4];
      see[0] ^= see[2];
      seed    = see[0];
    }

    seed = hash_leaf_seed<Symbol>(
      remain_bytes, { .key = p, .secret = secret, .seed = seed }
    );

    a = hash_read64(p.subspan(std::size(p) - (s_in_u64 * 2))) ^ remain_bytes;
    b = hash_read64(p.subspan(std::size(p) - (s_in_u64 * 1)));
  } else if (bytes_count >= 4) [[likely]] { // 4, 5....
    seed ^= bytes_count;
    if (bytes_count >= 8) { // 8, 9....
      a = hash_read64(p.subspan(0, s_in_u64));
      b = hash_read64(p.subspan(std::size(p) - s_in_u64, s_in_u64));
    } else { // 4, 5, 6, 7
      auto constexpr s_in_u32{ sizeof(u32) / sizeof(Symbol) };
      a = hash_read32(p.subspan(0, s_in_u32));
      a = hash_read32(p.subspan(std::size(p) - s_in_u32, s_in_u32));
    }
  } else { // 1, 2, 3
    a = (static_cast<u64>(p.front()) << 45) | static_cast<u64>(p.back());
    b = static_cast<u64>(p[std::size(p) >> 1]);
  }

  a ^= secret[1];
  b ^= seed;
  hash_mum(a, b);
  return hash_mix(a ^ secret[7], b ^ secret[1] ^ remain_bytes);
}

#undef GZN_HASH_BIG_ENDIAN
#undef GZN_HASH_LITTLE_ENDIAN

} // namespace gzn::fnd
