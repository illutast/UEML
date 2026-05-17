#ifndef UEMLDEPEDENCY_HEX
#define UEMLDEPEDENCY_HEX

#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>

class UEMLHexademicalString
{
public:
    uint64_t CursorPos = 0;
    std::string Data;
};

/// @brief Opens/Creates the Hex string and syncs both read/write pointers to 0.
inline void rglInitHexString(UEMLHexademicalString* HStr)
{
    HStr->CursorPos = 0;
}

/// @brief Jumps the cursor to a specific position and syncs hardware pointers.
inline void rglHexStringJumpTo(UEMLHexademicalString* HStr, uint64_t Position)
{
    HStr->CursorPos = Position;
}

/// @brief Moves cursor forward and syncs hardware pointers.
inline void rglHexStringAdvanceForward(UEMLHexademicalString* HStr, uint64_t AdvancePosition)
{
    HStr->CursorPos += AdvancePosition;
}

/// @brief Moves cursor backward (floored at 0) and syncs hardware pointers.
inline void rglHexStringAdvanceBackward(UEMLHexademicalString* HStr, uint64_t AdvancePosition)
{
    if (AdvancePosition > HStr->CursorPos) HStr->CursorPos = 0;
    else HStr->CursorPos -= AdvancePosition;
}

/// @brief Writes content at a specific position without moving the main cursor.
inline void rglHexStringWriteAt(UEMLHexademicalString* HStr, uint64_t Position, const std::vector<uint8_t>& Data)
{
    if (Position + Data.size() > HStr->Data.size())
    {
        HStr->Data.resize(Position + Data.size());
    }
    std::copy(Data.begin(), Data.end(), HStr->Data.begin() + Position);
}

/// @brief Writes content at the current cursor and advances the CursorPos.
inline void rglHexStringWriteAdvance(UEMLHexademicalString* HStr, const std::vector<uint8_t>& Data)
{
    rglHexStringWriteAt(HStr, HStr->CursorPos, Data);
    HStr->CursorPos += Data.size();
}

/// @brief   Reads exactly **Length** bytes at the cursor.
/// @warning This function does not advance cursor.
inline std::vector<uint8_t> rglHexStringReadAtCursor(UEMLHexademicalString* HStr, uint64_t Length)
{
    if (HStr->CursorPos >= HStr->Data.size() || Length == 0) return {};

    uint64_t MaxRead = std::min(Length, (uint64_t)HStr->Data.size() - HStr->CursorPos);
    auto StartIt = HStr->Data.begin() + HStr->CursorPos;
    
    return std::vector<uint8_t>(StartIt, StartIt + MaxRead);
}

/// @brief Reads a range of bytes and returns hardware pointers to CursorPos.
inline std::vector<uint8_t> rglHexStringReadFrom(UEMLHexademicalString* HStr, uint64_t Start, uint64_t End)
{
    if (Start >= HStr->Data.size() || End <= Start) return {};

    uint64_t AEnd = std::min(End, (uint64_t)HStr->Data.size());
    auto StartIt = HStr->Data.begin() + Start;
    
    return std::vector<uint8_t>(StartIt, HStr->Data.begin() + AEnd);
}

/// @brief Clears the HexString
inline void rglClearHexString(UEMLHexademicalString* HStr)
{
    HStr->Data.clear();
    HStr->CursorPos = 0;
}

#endif