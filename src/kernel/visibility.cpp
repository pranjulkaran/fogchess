/**
 * @file src/kernel/visibility.cpp
 * @brief Batched line-of-sight ray-casting and Fog of War visibility calculations.
 * @details Adapts the ray-clipping logic from your original pomdp64.cpp into the 
 *          StateBatch4 SoA paradigm[cite: 3, 4]. Computes sliding piece sight lines (Rooks, 
 *          Bishops, Queens) across 4 parallel game worlds simultaneously.
 */

#include "simd_math.hpp"
#include "fog_state.h"
#include <cstdint>

namespace fog {
namespace kernel {

// Pre-computed lookup tables matching pomdp64 structure, now mapped for internal kernel optimization[cite: 4]
static uint64_t ray_table[8][64]; // Arrays for up, down, left, right, ur, ul, dr, dl

/**
 * @brief Scalar ray clipping helper adapted from your pomdp64.cpp[cite: 3].
 * @details Preserved exactly to guarantee that when we extract a single lane from our 
 *          batch, the rule evaluation matches the original engine perfectly[cite: 3].
 */
static inline uint64_t clip_positive_lane(uint64_t ray, uint64_t blockers, const uint64_t* table) {
    #ifdef _MSC_VER
        unsigned long index;
        return _BitScanForward64(&index, blockers) ? (ray ^ table[index]) : ray;
    #else
        return blockers ? (ray ^ table[__builtin_ctzll(blockers)]) : ray;
    #endif
}

static inline uint64_t clip_negative_lane(uint64_t ray, uint64_t blockers, const uint64_t* table) {
    #ifdef _MSC_VER
        unsigned long index;
        return _BitScanReverse64(&index, blockers) ? (ray ^ table[63 - index]) : ray;
    #else
        return blockers ? (ray ^ table[63 - __builtin_clzll(blockers)]) : ray;
    #endif
}

/**
 * @brief Computes the sliding line-of-sight vision mask for 4 games simultaneously.
 * @details Loops through the lanes of the batch, mapping total occupancy masks to 
 *          the pre-computed spatial directional ray matrices[cite: 3].
 */
void compute_batched_sliding_vision(const StateBatch4* batch, int sq, int piece_type, uint64_t* out_mask_aligned) {
    alignas(32) uint64_t blockers[4];
    alignas(32) uint64_t final_vision[4];

    // Load the pre-calculated total occupancy bitboards from all 4 games into local storage
    _mm256_store_si256(reinterpret_cast<__m256i*>(blockers), 
                       _mm256_load_si256(reinterpret_cast<const __m256i*>(batch->total_occ)));

    // Process all 4 environments without breaking data layout lines
    for (int lane = 0; lane < 4; ++lane) {
        uint64_t vis = 0ULL;
        uint64_t occ = blockers[lane];

        if (piece_type == 3 || piece_type == 4) { // ROOK or QUEEN[cite: 4]
            vis |= clip_positive_lane(ray_table[0][sq], occ & ray_table[0][sq], ray_table[0]); // UP[cite: 3]
            vis |= clip_positive_lane(ray_table[1][sq], occ & ray_table[1][sq], ray_table[1]); // RIGHT[cite: 3]
            vis |= clip_negative_lane(ray_table[2][sq], occ & ray_table[2][sq], ray_table[2]); // DOWN[cite: 3]
            vis |= clip_negative_lane(ray_table[3][sq], occ & ray_table[3][sq], ray_table[3]); // LEFT[cite: 3]
        }
        if (piece_type == 2 || piece_type == 4) { // BISHOP or QUEEN[cite: 4]
            vis |= clip_positive_lane(ray_table[4][sq], occ & ray_table[4][sq], ray_table[4]); // UR[cite: 3]
            vis |= clip_positive_lane(ray_table[5][sq], occ & ray_table[5][sq], ray_table[5]); // UL[cite: 3]
            vis |= clip_negative_lane(ray_table[6][sq], occ & ray_table[6][sq], ray_table[6]); // DR[cite: 3]
            vis |= clip_negative_lane(ray_table[7][sq], occ & ray_table[7][sq], ray_table[7]); // DL[cite: 3]
        }
        final_vision[lane] = vis;
    }

    // Direct SIMD stream back to memory
    _mm256_store_si256(reinterpret_cast<__m256i*>(out_mask_aligned), 
                       _mm256_load_si256(reinterpret_cast<__m256i*>(final_vision)));
}

} // namespace kernel
} // namespace fog