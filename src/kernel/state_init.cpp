/**
 * @file src/kernel/state_init.cpp
 * @brief Batched initialization setting up standard 0x88 board states.
 */

#include "fog_state.h"
#include "simd_math.hpp"
#include <cstring>

namespace fog {
namespace kernel {

extern void update_batched_occupancy(StateBatch4* batch); // From move_gen.cpp

/**
 * @brief Resets all 4 lanes in a batch to the standard starting chess array.
 */
void batched_reset_board(StateBatch4* batch) {
    std::memset(batch, 0, sizeof(StateBatch4));

    for (int lane = 0; lane < 4; ++lane) {
        // White Pieces
        batch->pieces[0][0][lane] = 0x000000000000FF00ULL; // PAWN
        batch->pieces[0][1][lane] = 0x0000000000000042ULL; // KNIGHT
        batch->pieces[0][2][lane] = 0x0000000000000024ULL; // BISHOP
        batch->pieces[0][3][lane] = 0x0000000000000081ULL; // ROOK
        batch->pieces[0][4][lane] = 0x0000000000000008ULL; // QUEEN
        batch->pieces[0][5][lane] = 0x0000000000000010ULL; // KING

        // Black Pieces[cite: 3]
        batch->pieces[1][0][lane] = 0x00FF000000000000ULL; // PAWN
        batch->pieces[1][1][lane] = 0x4200000000000000ULL; // KNIGHT
        batch->pieces[1][2][lane] = 0x2400000000000000ULL; // BISHOP
        batch->pieces[1][3][lane] = 0x8100000000000000ULL; // ROOK
        batch->pieces[1][4][lane] = 0x0800000000000000ULL; // QUEEN
        batch->pieces[1][5][lane] = 0x1000000000000000ULL; // KING

        batch->active_turn[lane] = 0; // White starts
        
        // Metadata: Set 0xF (bits 1-4) to enable all four castling rights flags[cite: 3]
        batch->metadata[lane] = (0x0FULL << 1); 
    }

    // Instantly compress all bitboards into the master occupancy arrays using SIMD
    update_batched_occupancy(batch);
}

} // namespace kernel
} // namespace fog