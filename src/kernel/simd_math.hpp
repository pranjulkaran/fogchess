/**
 * @file src/kernel/simd_math.hpp
 * @brief Core AVX2 intrinsic wrappers for batched bitboard operations.
 * @details Abstracts the hardware-specific _mm256 instructions. A single 256-bit 
 *          register perfectly holds four 64-bit chess bitboards, allowing us to 
 *          process 4 parallel games in a single CPU clock cycle.
 */

#ifndef FOG_SIMD_MATH_HPP
#define FOG_SIMD_MATH_HPP

#include <immintrin.h>
#include <cstdint>
#include "fog_state.h"

namespace fog {
namespace kernel {

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

} // namespace kernel
} // namespace fog

#endif // FOG_SIMD_MATH_HPP