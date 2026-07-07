/**
 * @file src/kernel/observation.cpp
 * @brief Zero-allocation tensor generators extracting data from the SoA batch layout.
 */

#include "fog_state.h"
#include <algorithm>

namespace fog {
namespace kernel {

void generate_layer3_tensor_lane(const StateBatch4* batch, int lane,
                                 uint64_t visible_mask,
                                 const int32_t* scouted_sqs, int scouted_count,
                                 const int32_t* ghost_sqs, int ghost_count,
                                 bool is_scout_phase,
                                 float* out_buffer) {
    
    // Fast zero-out of the entire 14 * 64 float block 
    std::fill(out_buffer, out_buffer + 896, 0.0f);

    // Channels 0-5: White Pieces, Channels 6-11: Black Pieces
    for (int sq = 0; sq < 64; ++sq) {
        if ((visible_mask >> sq) & 1ULL) {
            // Replicating np.flipud alignment: flipped_r = 7 - (sq / 8)
            int flipped_r = 7 - (sq >> 3); 
            int out_idx = flipped_r * 8 + (sq & 7);
            
            for (int p = 0; p < 6; ++p) {
                // Accessing the specific lane from the pieces[Color][PieceType][Lane] array[cite: 13]
                if ((batch->pieces[0][p][lane] >> sq) & 1ULL) out_buffer[(p * 64) + out_idx] = 1.0f; 
                if ((batch->pieces[1][p][lane] >> sq) & 1ULL) out_buffer[((p + 6) * 64) + out_idx] = 1.0f; 
            }
        }
    }

    // Channel 12: Scouted Origins and Ghost Residues
    for (int i = 0; i < scouted_count; ++i) {
        int sq = scouted_sqs[i];
        out_buffer[(12 * 64) + ((7 - (sq >> 3)) * 8 + (sq & 7))] = 1.0f;
    }
    for (int i = 0; i < ghost_count; ++i) {
        int sq = ghost_sqs[i];
        out_buffer[(12 * 64) + ((7 - (sq >> 3)) * 8 + (sq & 7))] = 1.0f;
    }

    // Channel 13: Scout Phase Blanket Indicator
    if (is_scout_phase) {
        std::fill(out_buffer + 13 * 64, out_buffer + 14 * 64, 1.0f);
    }
}

void generate_true_state_matrix_lane(const StateBatch4* batch, int lane, int8_t* out_buffer) {
    // Clear buffer to full-visibility empty squares (0)
    std::fill(out_buffer, out_buffer + 64, 0);

    for (int sq = 0; sq < 64; ++sq) {
        int flipped_r = 7 - (sq >> 3);
        int out_idx = flipped_r * 8 + (sq & 7);
        int8_t piece_token = 0;

        // White pieces (Tokens 1 to 6)
        for (int p = 0; p < 6; ++p) {
            if ((batch->pieces[0][p][lane] >> sq) & 1ULL) {
                piece_token = static_cast<int8_t>(p + 1);
                break;
            }
        }
        
        // Black pieces (Tokens -1 to -6)
        if (piece_token == 0) {
            for (int p = 0; p < 6; ++p) {
                if ((batch->pieces[1][p][lane] >> sq) & 1ULL) {
                    piece_token = static_cast<int8_t>(-(p + 1));
                    break;
                }
            }
        }
        out_buffer[out_idx] = piece_token;
    }
}

} // namespace kernel
} // namespace fog