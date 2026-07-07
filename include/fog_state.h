/**
 * @file include/fog_state.h
 * @brief Structure-of-Arrays (SoA) layout for batched SIMD execution.
 */

#ifndef FOG_STATE_H
#define FOG_STATE_H

#include <cstdint>

namespace fog {

struct alignas(64) StateBatch4 {
    uint64_t pieces[2][6][4];
    
    uint64_t white_occ[4];
    uint64_t black_occ[4];
    uint64_t total_occ[4];
    
    uint32_t active_turn[4]; 
    
    // Tracks castling rights, turn, and en-passant square per game[cite: 4]
    uint64_t metadata[4]; 
};

} // namespace fog

#endif // FOG_STATE_H