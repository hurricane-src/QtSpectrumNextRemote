#pragma once
#include <cstdint>

enum class RemoteCommand : uint8_t
{
    Ping = 0,       //
    GetBanks = 1,   // page[0..8]
    Set8KBank = 2,  // page, bank
    WriteAt = 3,    // offset, len10, bytes
    JumpTo = 4,     // offset
    Echo = 5,
    Zero = 6,       // offset, len10
};
