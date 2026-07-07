/**
 * @file src/kernel/tables.cpp
 * @brief Pre-computes ray masks and attack steps during engine initialization.
 */

#include <cstdint>

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace fog {
namespace kernel {

uint64_t pawn_attacks[2][64];
uint64_t knight_attacks[64];
uint64_t king_attacks[64];
uint64_t ray_table[8][64]; // UP, RIGHT, DOWN, LEFT, UR, UL, DR, DL

// Inline scalar clipping helpers[cite: 3]
static inline uint64_t clip_positive(uint64_t ray, uint64_t blockers, const uint64_t* table) {
#ifdef _MSC_VER
    unsigned long index;
    return _BitScanForward64(&index, blockers) ? (ray ^ table[index]) : ray;
#else
    return blockers ? (ray ^ table[__builtin_ctzll(blockers)]) : ray;
#endif
}

static inline uint64_t clip_negative(uint64_t ray, uint64_t blockers, const uint64_t* table) {
#ifdef _MSC_VER
    unsigned long index;
    return _BitScanReverse64(&index, blockers) ? (ray ^ table[63 - index]) : ray;
#else
    return blockers ? (ray ^ table[63 - __builtin_clzll(blockers)]) : ray;
#endif
}

uint64_t get_rook_vision_scalar(int sq, uint64_t occ) {
    return clip_positive(ray_table[0][sq], occ & ray_table[0][sq], ray_table[0]) |
           clip_positive(ray_table[1][sq], occ & ray_table[1][sq], ray_table[1]) |
           clip_negative(ray_table[2][sq], occ & ray_table[2][sq], ray_table[2]) |
           clip_negative(ray_table[3][sq], occ & ray_table[3][sq], ray_table[3]);
}

uint64_t get_bishop_vision_scalar(int sq, uint64_t occ) {
    return clip_positive(ray_table[4][sq], occ & ray_table[4][sq], ray_table[4]) |
           clip_positive(ray_table[5][sq], occ & ray_table[5][sq], ray_table[5]) |
           clip_negative(ray_table[6][sq], occ & ray_table[6][sq], ray_table[6]) |
           clip_negative(ray_table[7][sq], occ & ray_table[7][sq], ray_table[7]);
}

uint64_t get_queen_vision_scalar(int sq, uint64_t occ) {
    return get_rook_vision_scalar(sq, occ) | get_bishop_vision_scalar(sq, occ);
}

/**
 * @brief Bootstrapper that populates all 64-square arrays[cite: 3].
 */
void init_engine_tables() {
    constexpr int knight_delta[8][2] = {{2,1}, {2,-1}, {-2,1}, {-2,-1}, {1,2}, {1,-2}, {-1,2}, {-1,-2}};
    constexpr int king_delta[8][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}, {1,1}, {1,-1}, {-1,1}, {-1,-1}};

    for (int s = 0; s < 64; ++s) {
        int f = s & 7, r = s >> 3;

        knight_attacks[s] = 0;
        king_attacks[s] = 0;
        pawn_attacks[0][s] = 0;
        pawn_attacks[1][s] = 0;
        for (int i=0; i<8; ++i) ray_table[i][s] = 0;

        // Process step calculations[cite: 3]
        for (const auto& d : knight_delta) {
            int nf = f + d[0], nr = r + d[1];
            if (nf >= 0 && nf < 8 && nr >= 0 && nr < 8) knight_attacks[s] |= 1ULL << (nr * 8 + nf);
        }
        for (const auto& d : king_delta) {
            int nf = f + d[0], nr = r + d[1];
            if (nf >= 0 && nf < 8 && nr >= 0 && nr < 8) king_attacks[s] |= 1ULL << (nr * 8 + nf);
        }

        if (r < 7) {
            if (f > 0) pawn_attacks[0][s] |= 1ULL << ((r + 1) * 8 + (f - 1));
            if (f < 7) pawn_attacks[0][s] |= 1ULL << ((r + 1) * 8 + (f + 1));
        }
        if (r > 0) {
            if (f > 0) pawn_attacks[1][s] |= 1ULL << ((r - 1) * 8 + (f - 1));
            if (f < 7) pawn_attacks[1][s] |= 1ULL << ((r - 1) * 8 + (f + 1));
        }

        // Process ray calculations[cite: 3]
        for (int rr = r + 1; rr < 8; ++rr)  ray_table[0][s] |= 1ULL << (rr * 8 + f); 
        for (int ff = f + 1; ff < 8; ++ff)  ray_table[1][s] |= 1ULL << (r * 8 + ff); 
        for (int rr = r - 1; rr >= 0; --rr) ray_table[2][s] |= 1ULL << (rr * 8 + f); 
        for (int ff = f - 1; ff >= 0; --ff) ray_table[3][s] |= 1ULL << (r * 8 + ff); 
        for (int rr = r+1, ff = f+1; rr<8 && ff<8; ++rr, ++ff)  ray_table[4][s] |= 1ULL << (rr * 8 + ff); 
        for (int rr = r+1, ff = f-1; rr<8 && ff>=0; ++rr, --ff) ray_table[5][s] |= 1ULL << (rr * 8 + ff); 
        for (int rr = r-1, ff = f+1; rr>=0 && ff<8; --rr, ++ff) ray_table[6][s] |= 1ULL << (rr * 8 + ff); 
        for (int rr = r-1, ff = f-1; rr>=0 && ff>=0; --rr, --ff) ray_table[7][s] |= 1ULL << (rr * 8 + ff); 
    }
}

} // namespace kernel
} // namespace fog