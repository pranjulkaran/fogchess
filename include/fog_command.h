/**
 * @file include/fog_command.h
 * @brief 32-bit packed command payload for the FogEngine.
 */

#ifndef FOG_COMMAND_H
#define FOG_COMMAND_H

#include <cstdint>

namespace fog {

// Extracted from pomdp64.hpp definitions[cite: 4]
constexpr uint8_t MOVE_QUIET = 0;
constexpr uint8_t MOVE_CAPTURE = 1;
constexpr uint8_t MOVE_DOUBLE_PAWN_PUSH = 2;
constexpr uint8_t MOVE_EN_PASSANT = 3;
constexpr uint8_t MOVE_KING_CASTLE = 4;
constexpr uint8_t MOVE_QUEEN_CASTLE = 5;
constexpr uint8_t MOVE_PROMOTE_KNIGHT = 6;
constexpr uint8_t MOVE_PROMOTE_BISHOP = 7;
constexpr uint8_t MOVE_PROMOTE_ROOK = 8;
constexpr uint8_t MOVE_PROMOTE_QUEEN = 9;

class Command {
public:
    Command() : data_(0) {}
    explicit Command(uint32_t raw) : data_(raw) {}
    Command(uint8_t from, uint8_t to, uint8_t piece, uint8_t flag) {
        data_ = (from & 0x3F) | ((to & 0x3F) << 6) | ((piece & 0x07) << 12) | ((flag & 0x0F) << 15);
    }

    inline uint8_t get_from_square() const { return data_ & 0x3F; }
    inline uint8_t get_to_square() const   { return (data_ >> 6) & 0x3F; }
    inline uint8_t get_piece_type() const  { return (data_ >> 12) & 0x07; }
    inline uint8_t get_flag() const        { return (data_ >> 15) & 0x0F; }
    inline uint32_t get_raw() const        { return data_; }

private:
    uint32_t data_;
};

} // namespace fog

#endif // FOG_COMMAND_H