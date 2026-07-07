/**
 * @file fog_command.h
 * @brief FogEngine minimal command payload definitions.
 * @details Enforces the strict "Command-Driven" principle. All inputs to the 
 *          engine must be serialized through these fixed-width structures.
 */

#pragma once

#include <cstdint>

namespace fog {

/**
 * @enum CommandFlag
 * @brief 4-bit flags defining special command properties (Castling, Promotion, etc.)
 */
enum CommandFlag : uint8_t {
    CMD_NORMAL          = 0x0,
    CMD_SCOUT           = 0x1,
    CMD_CAPTURE         = 0x2,
    CMD_EP_CAPTURE      = 0x3,
    CMD_DOUBLE_PUSH     = 0x4,
    CMD_CASTLE_KING     = 0x5,
    CMD_CASTLE_QUEEN    = 0x6,
    CMD_PROMOTE_KNIGHT  = 0x7,
    CMD_PROMOTE_BISHOP  = 0x8,
    CMD_PROMOTE_ROOK    = 0x9,
    CMD_PROMOTE_QUEEN   = 0xA,
    CMD_RESIGN          = 0xF // Special system flag
};

/**
 * @struct Command
 * @brief A hyper-optimized, 16-bit encoded action payload.
 * @details Packs the origin square, destination square, and action flag into 
 *          exactly 2 bytes. This guarantees deterministic network transmission
 *          and ultra-low memory footprint for replays (100 turns = 200 bytes).
 * 
 * Bit Layout (16 bits total):
 * [ 15..12: Flag (4b) ] [ 11..6: To Square (6b) ] [ 5..0: From Square (6b) ]
 */
struct Command {
    uint16_t raw_data;

    // Default constructor for zero-initialization in arrays
    constexpr Command() : raw_data(0) {}

    // Parameterized constructor explicitly packing the bits
    constexpr Command(uint8_t from_sq, uint8_t to_sq, CommandFlag flag = CMD_NORMAL) 
        : raw_data( (static_cast<uint16_t>(flag & 0x0F) << 12) | 
                    (static_cast<uint16_t>(to_sq & 0x3F) << 6) | 
                    (static_cast<uint16_t>(from_sq & 0x3F)) ) {}

    /**
     * @brief Extracts the 6-bit origin square (0-63)
     */
    [[nodiscard]] constexpr uint8_t get_from() const {
        return static_cast<uint8_t>(raw_data & 0x3F);
    }

    /**
     * @brief Extracts the 6-bit destination square (0-63)
     */
    [[nodiscard]] constexpr uint8_t get_to() const {
        return static_cast<uint8_t>((raw_data >> 6) & 0x3F);
    }

    /**
     * @brief Extracts the 4-bit action flag
     */
    [[nodiscard]] constexpr CommandFlag get_flag() const {
        return static_cast<CommandFlag>((raw_data >> 12) & 0x0F);
    }

    // Equality operators for deterministic comparisons
    constexpr bool operator==(const Command& other) const { return raw_data == other.raw_data; }
    constexpr bool operator!=(const Command& other) const { return raw_data != other.raw_data; }
};

static_assert(sizeof(Command) == 2, "Command payload must be exactly 2 bytes for network/replay optimization.");

} // namespace fog