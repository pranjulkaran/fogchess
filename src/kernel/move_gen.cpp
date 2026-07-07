/**
 * @file src/kernel/move_gen.cpp
 * @brief Vectorized SIMD move generation logic executing batched physics rules.
 * @details Replaces the old scalar 'generate_pseudo_moves'[cite: 3]. 
 *          Operates directly on the 'StateBatch4' SoA memory layout, 
 *          processing rules for 4 environments simultaneously without branching.
 */

#include "simd_math.hpp"
#include "fog_state.h"

namespace fog {
namespace kernel {

/**
 * @brief Batched update of Master Occupancy fields across 4 parallel games.
 * @details Reads the individual piece arrays from the StateBatch4 struct and 
 *          aggregates them into the master white_occ and black_occ bitboards 
 *          using 256-bit vector math.
 * @param batch Pointer to a 64-byte aligned SoA batch holding 4 games.
 */
void update_batched_occupancy(StateBatch4* batch) {
    // 1. Initialize accumulator vectors to zero
    Vector256 white_occ = _mm256_setzero_si256();
    Vector256 black_occ = _mm256_setzero_si256();

    // 2. Loop over the 6 piece types (0=PAWN ... 5=KING)
    for (int p = 0; p < 6; ++p) {
        // Load the 4 white bitboards for this piece type
        Vector256 w_pieces(batch->pieces[0][p]);
        white_occ = white_occ | w_pieces;

        // Load the 4 black bitboards for this piece type
        Vector256 b_pieces(batch->pieces[1][p]);
        black_occ = black_occ | b_pieces;
    }

    // 3. Calculate total occupancy for all 4 games
    Vector256 total_occ = white_occ | black_occ;

    // 4. Store the results directly back into the batched state memory
    white_occ.store(batch->white_occ);
    black_occ.store(batch->black_occ);
    total_occ.store(batch->total_occ);
}

/**
 * @brief Filters pseudo-legal attacks across 4 games to prevent friendly-fire.
 * @param attacks The raw 256-bit vector of generated attack squares.
 * @param friendly_occ The 256-bit vector of current friendly piece occupancies.
 * @return Vector256 Validated bitboards with friendly squares masked out.
 */
inline Vector256 filter_friendly_fire(const Vector256& attacks, const Vector256& friendly_occ) {
    // Computes: attacks & (~friendly_occ) for 4 games in one clock cycle
    return and_not(friendly_occ, attacks);
}

} // namespace kernel
} // namespace fog