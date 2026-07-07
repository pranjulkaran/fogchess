/**
 * @file fog_state.h
 * @brief Structure of Arrays (SoA) game state memory layouts for batched SIMD execution.
 * @details Redesigns the standard single-board format into AVX2-friendly chunks,
 *          allowing the engine to process multiple games in a single CPU cycle.
 */

#pragma once

#include <cstdint>

namespace fog {

constexpr int COLOR_WHITE = 0;
constexpr int COLOR_BLACK = 1;

constexpr int PIECE_PAWN   = 0;
constexpr int PIECE_KNIGHT = 1;
constexpr int PIECE_BISHOP = 2;
constexpr int PIECE_ROOK   = 3;
constexpr int PIECE_QUEEN  = 4;
constexpr int PIECE_KING   = 5;

// SIMD Lane Width: 4 games parallelizes perfectly into 256-bit AVX2 registers.
constexpr int BATCH_SIZE = 4; 

/**
 * @struct StateBatch4
 * @brief Holds 4 complete games interleaved for SIMD instructions (Structure of Arrays).
 * @details Instead of evaluating `board.pawns` one game at a time, the compiler can 
 *          load `batch.pieces[WHITE][PAWN]` directly into a 256-bit AVX2 register 
 *          and compute legal moves for 4 different games simultaneously.
 */
struct alignas(64) StateBatch4 {
    // -------------------------------------------------------------------------
    // SoA BITBOARDS (Data aligned for SIMD)
    // Dimension 0: Color (White/Black)
    // Dimension 1: Piece Type (Pawn -> King)
    // Dimension 2: The 4 parallel games
    // -------------------------------------------------------------------------
    uint64_t pieces[2][6][BATCH_SIZE]; 

    // Aggregate masks useful for vision and move generation (computed on the fly)
    uint64_t white_occ[BATCH_SIZE];
    uint64_t black_occ[BATCH_SIZE];
    uint64_t total_occ[BATCH_SIZE];

    // -------------------------------------------------------------------------
    // METADATA
    // Bit 0: Turn (0=White, 1=Black)
    // Bits 1-4: Castling Rights
    // Bits 5-10: En Passant Target Square
    // -------------------------------------------------------------------------
    uint64_t metadata[BATCH_SIZE];

    /**
     * @brief Zeroes out the memory for all 4 parallel games to prevent UB.
     */
    constexpr void clear() {
        for (int c = 0; c < 2; ++c) {
            for (int p = 0; p < 6; ++p) {
                for (int i = 0; i < BATCH_SIZE; ++i) {
                    pieces[c][p][i] = 0ULL;
                }
            }
        }
        for (int i = 0; i < BATCH_SIZE; ++i) {
            white_occ[i] = 0ULL;
            black_occ[i] = 0ULL;
            total_occ[i] = 0ULL;
            metadata[i] = 0ULL;
        }
    }
};

// Ensure cache line alignment to prevent false-sharing across thread pool workers
static_assert(sizeof(StateBatch4) % 64 == 0, "StateBatch4 must map neatly to 64-byte L1 cache lines.");

} // namespace fog