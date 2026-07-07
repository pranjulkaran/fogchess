/**
 * @file include/fog_state.h
 * @brief Structure-of-Arrays (SoA) layout for batched SIMD execution.
 */

#ifndef FOG_STATE_H
#define FOG_STATE_H

#include <cstdint>

namespace fog {

/**
 * @struct StateBatch4
 * @brief Aligned memory block holding 4 independent game states for AVX2 processing.
 */
struct alignas(64) StateBatch4 {
    // pieces[color (2)][piece_type (6)][lane (4)]
    uint64_t pieces[2][6][4];
    
    // Master occupancy bitboards mapped per lane
    uint64_t white_occ[4];
    uint64_t black_occ[4];
    uint64_t total_occ[4];
    
    // The active player color for each of the 4 games (0 = White, 1 = Black)
    uint32_t active_turn[4]; 
};

} // namespace fog

#endif // FOG_STATE_H