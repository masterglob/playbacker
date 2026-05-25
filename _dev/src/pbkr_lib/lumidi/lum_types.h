#pragma once

#include <stdint.h>
#include <string>
#include <cstdint>
#include <queue>
#include <array>

namespace LUMIDI
{
struct MidiMessage
{
    uint8_t status = 0;
    std::array<uint8_t, 2> data{};
    uint8_t length = 0;

    int channel() const;

    uint8_t type() const;

    std::string dumpHex() const;
};

class MidiParser
{
public:
    void feed(uint8_t byte);

    bool pop(MidiMessage& out);

    inline bool hasMessage() const
    {
        return !fifo.empty();
    }

private:
    uint8_t runningStatus = 0;

    MidiMessage current{};
    uint8_t dataIndex = 0;
    uint8_t expected = 0;

    std::queue<MidiMessage> fifo;

private:
    void push(const MidiMessage& msg);

    int getDataLength(uint8_t status);
};

} // namespace LUMIDI
