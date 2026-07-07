/**
 * @file src/kernel/threats.cpp
 * @brief Threat detection and terminal state evaluations.
 */

#include "fog_state.h"

namespace fog {
namespace kernel {

// Extern lookup arrays (Assuming these are generated at boot exactly like init_attack_masks()[cite: 3])
extern uint64_t pawn_attacks[2][64];
extern uint64_t knight_attacks[64];
extern uint64_t king_attacks[64];
extern uint64_t get_bishop_vision_scalar(int sq, uint64_t occ); // Fallback scalar for ray lookups
extern uint64_t get_rook_vision_scalar(int sq, uint64_t occ);

/**
 * @brief Checks if a specific square in a specific lane is attacked by an enemy[cite: 3].
 */
bool is_square_attacked_lane(const StateBatch4* batch, int lane, int sq, int attacker_color) {
    uint64_t total_occ = batch->total_occ[lane];
    int defender_color = attacker_color ^ 1;

    // Pawn threats[cite: 3]
    if (batch->pieces[attacker_color][0][lane] & pawn_attacks[defender_color][sq]) return true;
    
    // Knight threats[cite: 3]
    if (batch->pieces[attacker_color][1][lane] & knight_attacks[sq]) return true;
    
    // King threats[cite: 3]
    if (batch->pieces[attacker_color][5][lane] & king_attacks[sq]) return true;

    // Bishop & Queen diagonal threats[cite: 3]
    if (get_bishop_vision_scalar(sq, total_occ) & (batch->pieces[attacker_color][2][lane] | batch->pieces[attacker_color][4][lane]))
        return true;

    // Rook & Queen orthogonal threats[cite: 3]
    if (get_rook_vision_scalar(sq, total_occ) & (batch->pieces[attacker_color][3][lane] | batch->pieces[attacker_color][4][lane]))
        return true;

    return false;
}

/**
 * @brief Vectorized check for captured kings across all 4 games simultaneously[cite: 3].
 * @return uint32_t A 4-bit mask where bit `i` is 1 if lane `i` has a captured king.
 */
uint32_t batched_is_king_captured(const StateBatch4* batch, int target_color) {
    uint32_t termination_mask = 0;
    for (int lane = 0; lane < 4; ++lane) {
        if (batch->pieces[target_color][5][lane] == 0ULL) { // King bitboard plane collapsed to zero[cite: 3]
            termination_mask |= (1 << lane);
        }
    }
    return termination_mask;
}

} // namespace kernel
} // namespace fog