/**
 * @file src/kernel/rules.cpp
 * @brief Executes physical rules (captures, promotions, castling) across the SIMD batches.
 */

#include "fog_state.h"
#include "fog_command.h"
#include "simd_math.hpp"

namespace fog {
namespace kernel {

extern void update_batched_occupancy(StateBatch4* batch); // Link to move_gen.cpp

/**
 * @brief Applies 4 commands to 4 parallel game states, mapping exactly to pomdp64 logic[cite: 3].
 */
void apply_batched_commands(StateBatch4* batch, const uint32_t* commands) {
    for (int lane = 0; lane < 4; ++lane) {
        Command move(commands[lane]);
        
        int color = batch->active_turn[lane];
        int enemy = color ^ 1;
        
        int from = move.get_from_square();
        int to = move.get_to_square();
        int piece = move.get_piece_type();
        int flag = move.get_flag();

        // --- CASTLING OPERATIONS[cite: 3] ---
        if (flag == MOVE_KING_CASTLE) {
            if (color == 0) { // White
                batch->pieces[0][5][lane] &= ~(1ULL << 4);  batch->pieces[0][5][lane] |= (1ULL << 6);
                batch->pieces[0][3][lane] &= ~(1ULL << 7);  batch->pieces[0][3][lane] |= (1ULL << 5);
            } else { // Black
                batch->pieces[1][5][lane] &= ~(1ULL << 60); batch->pieces[1][5][lane] |= (1ULL << 62);
                batch->pieces[1][3][lane] &= ~(1ULL << 63); batch->pieces[1][3][lane] |= (1ULL << 61);
            }
            batch->metadata[lane] &= (color == 0) ? ~(3ULL << 1) : ~(3ULL << 3); // Clear castling[cite: 4]
            batch->active_turn[lane] ^= 1; // Flip turn
            continue;
        }

        if (flag == MOVE_QUEEN_CASTLE) {
            if (color == 0) { // White
                batch->pieces[0][5][lane] &= ~(1ULL << 4);  batch->pieces[0][5][lane] |= (1ULL << 2);
                batch->pieces[0][3][lane] &= ~(1ULL << 0);  batch->pieces[0][3][lane] |= (1ULL << 3);
            } else { // Black
                batch->pieces[1][5][lane] &= ~(1ULL << 60); batch->pieces[1][5][lane] |= (1ULL << 58);
                batch->pieces[1][3][lane] &= ~(1ULL << 56); batch->pieces[1][3][lane] |= (1ULL << 59);
            }
            batch->metadata[lane] &= (color == 0) ? ~(3ULL << 1) : ~(3ULL << 3);
            batch->active_turn[lane] ^= 1;
            continue;
        }

        // --- STANDARD REMOVALS & CAPTURES[cite: 3] ---
        batch->pieces[color][piece][lane] &= ~(1ULL << from);
        
        // Blindly clear destination square from all enemy pieces
        for (int p = 0; p < 6; ++p) {
            batch->pieces[enemy][p][lane] &= ~(1ULL << to);
        }

        // --- EN PASSANT GHOST REMOVAL[cite: 3] ---
        if (flag == MOVE_EN_PASSANT) {
            int captured_sq = (color == 0) ? (to - 8) : (to + 8);
            batch->pieces[enemy][0][lane] &= ~(1ULL << captured_sq);
        }

        // --- PROMOTIONS[cite: 3] ---
        if (flag >= MOVE_PROMOTE_KNIGHT) {
            int promo_piece = 4; // Default Queen[cite: 3]
            if (flag == MOVE_PROMOTE_KNIGHT) promo_piece = 1;
            if (flag == MOVE_PROMOTE_BISHOP) promo_piece = 2;
            if (flag == MOVE_PROMOTE_ROOK)   promo_piece = 3;
            batch->pieces[color][promo_piece][lane] |= (1ULL << to);
        } else {
            batch->pieces[color][piece][lane] |= (1ULL << to);
        }

        // Final turn flip
        batch->active_turn[lane] ^= 1;
    }

    // After all 4 lanes are processed, we update the occupancy tables instantly via AVX2
    update_batched_occupancy(batch);
}

} // namespace kernel
} // namespace fog