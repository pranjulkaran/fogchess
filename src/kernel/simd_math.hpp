/**
 * @file src/kernel/simd_math.hpp
 * @brief Core AVX2 intrinsic wrappers for batched bitboard operations.
 * @details Abstracts the hardware-specific _mm256 instructions. A single 256-bit 
 *          register perfectly holds four 64-bit chess bitboards, allowing us to 
 *          process 4 parallel games in a single CPU clock cycle.
 */

#ifndef FOG_SIMD_MATH_HPP
#define FOG_SIMD_MATH_HPP

#include <cstdint>
#include <cstring>
#include "fog_state.h"

// AVX2 intrinsics are only available on x86/x64. Apple Silicon (arm64, the
// default "macos-latest" architecture) and other non-x86 targets fall back
// to a portable scalar implementation with an identical interface below.
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#define FOG_SIMD_HAS_AVX2 1
#include <immintrin.h>
#else
#define FOG_SIMD_HAS_AVX2 0
#endif

namespace fog {
namespace kernel {

#if FOG_SIMD_HAS_AVX2

/**
 * @brief Wrapper for a 256-bit AVX2 vector containing 4 x uint64_t.
 */
struct Vector256 {
    __m256i data;

    // Implicit conversion constructor from raw memory array
    inline Vector256(const uint64_t* memory_aligned) {
        data = _mm256_load_si256(reinterpret_cast<const __m256i*>(memory_aligned));
    }
    
    inline Vector256(__m256i raw) : data(raw) {}

    // Store back to contiguous SoA memory
    inline void store(uint64_t* memory_aligned) const {
        _mm256_store_si256(reinterpret_cast<__m256i*>(memory_aligned), data);
    }
};

// --- BATCHED BITWISE OPERATIONS ---

/** @brief Performs bitwise AND across 4 boards simultaneously. */
inline Vector256 operator&(const Vector256& a, const Vector256& b) {
    return _mm256_and_si256(a.data, b.data);
}

/** @brief Performs bitwise OR across 4 boards simultaneously. */
inline Vector256 operator|(const Vector256& a, const Vector256& b) {
    return _mm256_or_si256(a.data, b.data);
}

/** @brief Performs bitwise AND-NOT (~a & b) across 4 boards simultaneously. */
inline Vector256 and_not(const Vector256& mask, const Vector256& target) {
    return _mm256_andnot_si256(mask.data, target.data);
}

#else // !FOG_SIMD_HAS_AVX2

/**
 * @brief Portable (non-AVX2) stand-in for the 4 x uint64_t vector.
 * @details Used on architectures without AVX2 (e.g. arm64/Apple Silicon).
 *          Produces bit-identical results to the AVX2 path, just without
 *          the single-instruction hardware fan-out.
 */
struct Vector256 {
    uint64_t lanes[4];

    inline Vector256(const uint64_t* memory_aligned) {
        std::memcpy(lanes, memory_aligned, sizeof(lanes));
    }

    inline Vector256() = default;

    inline void store(uint64_t* memory_aligned) const {
        std::memcpy(memory_aligned, lanes, sizeof(lanes));
    }
};

// --- BATCHED BITWISE OPERATIONS (scalar fallback) ---

inline Vector256 operator&(const Vector256& a, const Vector256& b) {
    Vector256 out;
    for (int i = 0; i < 4; ++i) out.lanes[i] = a.lanes[i] & b.lanes[i];
    return out;
}

inline Vector256 operator|(const Vector256& a, const Vector256& b) {
    Vector256 out;
    for (int i = 0; i < 4; ++i) out.lanes[i] = a.lanes[i] | b.lanes[i];
    return out;
}

/** @brief Performs bitwise AND-NOT (~a & b) across 4 boards simultaneously. */
inline Vector256 and_not(const Vector256& mask, const Vector256& target) {
    Vector256 out;
    for (int i = 0; i < 4; ++i) out.lanes[i] = ~mask.lanes[i] & target.lanes[i];
    return out;
}

#endif // FOG_SIMD_HAS_AVX2

} // namespace kernel
} // namespace fog

#endif // FOG_SIMD_MATH_HPP