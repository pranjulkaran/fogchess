/**
 * @file src/kernel/move_gen_full.cpp
 * @brief Extracts pseudo-legal moves for a specific lane without dynamic allocations.
 */

#include "fog_state.h"
#include "fog_command.h"

namespace fog {
namespace kernel {

extern uint64_t pawn_attacks[2][64];
extern uint64_t knight_attacks[64];
extern uint64_t king_attacks[64];
extern uint64_t get_bishop_vision_scalar(int sq, uint64_t occ);
extern uint64_t get_rook_vision_scalar(int sq, uint64_t occ);
extern uint64_t get_queen_vision_scalar(int sq, uint64_t occ);

constexpr uint64_t RANK_2 = 0x000000000000FF00ULL; // Defined in pomdp64.hpp[cite: 4]
constexpr uint64_t RANK_7 = 0x00FF000000000000ULL; // Defined in pomdp64.hpp[cite: 4]

#ifdef _MSC_VER
#include <intrin.h>
inline int fast_ctzll(uint64_t bb) { unsigned long idx; _BitScanForward64(&idx, bb); return idx; }
#else
inline int fast_ctzll(uint64_t bb) { return __builtin_ctzll(bb); }
#endif

/**
 * @brief Traverses bitboards for a given lane and populates the out_commands buffer[cite: 3].
 * @return uint32_t Total number of moves generated.
 */
uint32_t generate_pseudo_moves_lane(const StateBatch4* batch, int lane, uint32_t* out_commands, uint32_t max_moves) {
    uint32_t count = 0;
    int color = batch->active_turn[lane];
    uint64_t friendly = (color == 0) ? batch->white_occ[lane] : batch->black_occ[lane];
    uint64_t enemy    = (color == 0) ? batch->black_occ[lane] : batch->white_occ[lane];
    uint64_t total    = batch->total_occ[lane];

    // 1. Process Knights through Kings[cite: 3]
    for (int piece = 1; piece <= 5; ++piece) {
        uint64_t bb = batch->pieces[color][piece][lane];
        while (bb) {
            int from = fast_ctzll(bb);
            uint64_t attacks = 0ULL;

            switch (piece) {
                case 1: attacks = knight_attacks[from]; break;
                case 2: attacks = get_bishop_vision_scalar(from, total); break;
                case 3: attacks = get_rook_vision_scalar(from, total); break;
                case 4: attacks = get_queen_vision_scalar(from, total); break;
                case 5: attacks = king_attacks[from]; break; // Standard king moves[cite: 3]
            }

            attacks &= ~friendly; // Prevent friendly fire[cite: 3]

            while (attacks && count < max_moves) {
                int to = fast_ctzll(attacks);
                uint8_t flag = (enemy & (1ULL << to)) ? MOVE_CAPTURE : MOVE_QUIET; // Verify captures[cite: 3, 4]
                out_commands[count++] = Command(from, to, piece, flag).get_raw();
                attacks &= attacks - 1;
            }
            bb &= bb - 1;
        }
    }

    // 2. Process Pawns (Highly directional)[cite: 3]
    uint64_t pawns = batch->pieces[color][0][lane];
    while (pawns && count < max_moves) {
        int from = fast_ctzll(pawns);
        uint64_t moves = 0ULL;

        if (color == 0) { // White Pawns[cite: 3]
            int push = from + 8;
            if (push < 64 && !(total & (1ULL << push))) moves |= (1ULL << push);
            if (((1ULL << from) & RANK_2) && !(total & (1ULL << push)) && !(total & (1ULL << (from + 16)))) {
                moves |= (1ULL << (from + 16));
            }
            moves |= pawn_attacks[0][from] & enemy;
        } else { // Black Pawns[cite: 3]
            int push = from - 8;
            if (push >= 0 && !(total & (1ULL << push))) moves |= (1ULL << push);
            if (((1ULL << from) & RANK_7) && !(total & (1ULL << push)) && !(total & (1ULL << (from - 16)))) {
                moves |= (1ULL << (from - 16));
            }
            moves |= pawn_attacks[1][from] & enemy;
        }

        while (moves && count < max_moves) {
            int to = fast_ctzll(moves);
            uint8_t flag = (enemy & (1ULL << to)) ? MOVE_CAPTURE : MOVE_QUIET;

            // Promotion Forks[cite: 3, 4]
            if ((color == 0 && to >= 56) || (color == 1 && to <= 7)) {
                if (count + 3 < max_moves) {
                    out_commands[count++] = Command(from, to, 0, MOVE_PROMOTE_KNIGHT).get_raw();
                    out_commands[count++] = Command(from, to, 0, MOVE_PROMOTE_BISHOP).get_raw();
                    out_commands[count++] = Command(from, to, 0, MOVE_PROMOTE_ROOK).get_raw();
                    out_commands[count++] = Command(from, to, 0, MOVE_PROMOTE_QUEEN).get_raw();
                }
            } else {
                if (color == 0 && (to - from == 16))      flag = MOVE_DOUBLE_PAWN_PUSH;
                else if (color == 1 && (from - to == 16)) flag = MOVE_DOUBLE_PAWN_PUSH;
                out_commands[count++] = Command(from, to, 0, flag).get_raw();
            }
            moves &= moves - 1;
        }
        pawns &= pawns - 1;
    }

    return count;
}

} // namespace kernel
} // namespace fog