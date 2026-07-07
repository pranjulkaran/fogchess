/**
 * @file include/fog_command.h
 * @brief 32-bit packed command payload for the FogEngine.
 */

#ifndef FOG_COMMAND_H
#define FOG_COMMAND_H

#include <cstdint>

namespace fog {

class Command {
public:
    // Default constructor
    Command() : data_(0) {}

    // Construct directly from a raw 32-bit payload (used by the SDK bridge)
    explicit Command(uint32_t raw) : data_(raw) {}

    // Construct from discrete game parameters
    Command(uint8_t from, uint8_t to, uint8_t piece, uint8_t flag) {
        data_ = (from & 0x3F) |                  // Bits 0-5
                ((to & 0x3F) << 6) |             // Bits 6-11
                ((piece & 0x07) << 12) |         // Bits 12-14
                ((flag & 0x0F) << 15);           // Bits 15-18
    }

    // Fast bitwise extractors
    inline uint8_t get_from_square() const { return data_ & 0x3F; }
    inline uint8_t get_to_square() const   { return (data_ >> 6) & 0x3F; }
    inline uint8_t get_piece_type() const  { return (data_ >> 12) & 0x07; }
    inline uint8_t get_flag() const        { return (data_ >> 15) & 0x0F; }
    
    // Returns the raw payload to pass through the C-ABI
    inline uint32_t get_raw() const        { return data_; }

private:
    uint32_t data_;
};

} // namespace fog

#endif // FOG_COMMAND_H