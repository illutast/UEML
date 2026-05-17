#ifndef UEMLDEPENDENCY_FHEX
#define UEMLDEPENDENCY_FHEX

#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

class UEMLHexademicalFile
{
public:
    uint64_t CursorPos = 0;
    std::fstream Stream;
};

/// @brief Opens/Creates the Hex File and syncs both read/write pointers to 0.
inline bool rglOpenHexFile(const std::string& FilePath, UEMLHexademicalFile* HFile)
{
    HFile->Stream.open(FilePath, std::ios::binary | std::ios::in | std::ios::out);

    if (!HFile->Stream.is_open())
    {
        HFile->Stream.clear();
        HFile->Stream.open(FilePath, std::ios::binary | std::ios::out);
        HFile->Stream.close();
        HFile->Stream.open(FilePath, std::ios::binary | std::ios::in | std::ios::out);
    }
    if (HFile->Stream.is_open())
    {
        HFile->CursorPos = 0;
        HFile->Stream.seekp(0, std::ios::beg);
        HFile->Stream.seekg(0, std::ios::beg);
        return true;
    }
    return false;
}

/// @brief Jumps the cursor to a specific position and syncs hardware pointers.
inline void rglHexFileJumpTo(UEMLHexademicalFile* HFile, uint64_t Position)
{
    HFile->CursorPos = Position;
    HFile->Stream.seekp(HFile->CursorPos, std::ios::beg);
    HFile->Stream.seekg(HFile->CursorPos, std::ios::beg);
}

/// @brief Moves cursor forward and syncs hardware pointers.
inline void rglHexFileAdvanceForward(UEMLHexademicalFile* HFile, uint64_t AdvancePosition)
{
    HFile->CursorPos += AdvancePosition;
    HFile->Stream.seekp(HFile->CursorPos, std::ios::beg);
    HFile->Stream.seekg(HFile->CursorPos, std::ios::beg);
}

/// @brief Moves cursor backward (floored at 0) and syncs hardware pointers.
inline void rglHexFileAdvanceBackward(UEMLHexademicalFile* HFile, uint64_t AdvancePosition)
{
    if (AdvancePosition > HFile->CursorPos) HFile->CursorPos = 0;
    else HFile->CursorPos -= AdvancePosition;

    HFile->Stream.seekp(HFile->CursorPos, std::ios::beg);
    HFile->Stream.seekg(HFile->CursorPos, std::ios::beg);
}

/// @brief Writes content at a specific position without moving the main cursor.
inline void rglHexFileWriteAt(UEMLHexademicalFile* HFile, uint64_t Position, const std::vector<uint8_t> Data)
{
    if (HFile->Stream.is_open())
    {
        HFile->Stream.seekp(Position, std::ios::beg);
        HFile->Stream.write(reinterpret_cast<const char*>(Data.data()), Data.size());
        
        HFile->Stream.seekp(HFile->CursorPos, std::ios::beg);
        HFile->Stream.seekg(HFile->CursorPos, std::ios::beg);
        HFile->Stream.flush(); 
    }
}

/// @brief Writes content at the current cursor and advances the CursorPos.
inline void rglHexFileWriteAdvance(UEMLHexademicalFile* HFile, const std::vector<uint8_t> Data)
{
    if (HFile->Stream.is_open())
    {
        HFile->Stream.write(reinterpret_cast<const char*>(Data.data()), Data.size());
        HFile->CursorPos += Data.size();

        HFile->Stream.seekp(HFile->CursorPos, std::ios::beg);
        HFile->Stream.seekg(HFile->CursorPos, std::ios::beg);
        HFile->Stream.flush(); 
    }
}

/// @brief   Reads exactly **Length** bytes at the cursor.
/// @warning This function does not advance cursor.
inline std::vector<uint8_t> rglHexFileReadAtCursor(UEMLHexademicalFile* HFile, uint64_t Length)
{
    if (!HFile->Stream.is_open() || Length == 0) return {};

    std::vector<uint8_t> Buffer(Length);
    
    HFile->Stream.read(reinterpret_cast<char*>(Buffer.data()), Length);
    HFile->Stream.seekg(HFile->CursorPos, std::ios::beg);
    HFile->Stream.seekp(HFile->CursorPos, std::ios::beg);

    return Buffer;
}

/// @brief Reads a range of bytes and returns hardware pointers to CursorPos.
inline std::vector<uint8_t> rglHexFileReadFrom(UEMLHexademicalFile* HFile, uint64_t Start, uint64_t End)
{
    if (!HFile->Stream.is_open() || End <= Start) return {};

    uint64_t Len = End - Start;
    std::vector<uint8_t> Buffer(Len);

    HFile->Stream.seekg(Start, std::ios::beg);
    HFile->Stream.read(reinterpret_cast<char*>(Buffer.data()), Len);

    HFile->Stream.seekg(HFile->CursorPos, std::ios::beg);
    HFile->Stream.seekp(HFile->CursorPos, std::ios::beg);

    return Buffer;
}

/// @brief Closes the stream.
inline void rglCloseHexFile(UEMLHexademicalFile* HFile)
{
    if (HFile->Stream.is_open())
    {
        HFile->Stream.close();
    }
}

#endif