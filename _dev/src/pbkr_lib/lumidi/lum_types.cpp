#include "lumidi/lum_types.h"

#include <stdint.h>
#include <cstdint>
#include <queue>
#include <array>
#include <sstream>
#include <iomanip>

namespace LUMIDI
{
/***********************************************************************/
int MidiMessage::channel() const
{
    if ((status & 0xF0) < 0xF0)
        return (status & 0x0F) + 1;
    return -1;
}

/***********************************************************************/
inline uint8_t MidiMessage::type() const
{
    return status & 0xF0;
}

/***********************************************************************/
std::string MidiMessage::dumpHex() const
{
    std::ostringstream oss;

    oss << std::hex << std::uppercase << std::setfill('0');

    // Status
    oss << "STATUS: 0x"
        << std::setw(2) << int(status)
        << " ";

    // Type + channel (debug utile)
    if ((status & 0xF0) < 0xF0)
    {
        oss << "(type=0x"
            << std::setw(2) << int(status & 0xF0)
            << ", ch=" << (channel()+1)
            << ") ";
    }

    // Data
    oss << "DATA: ";

    for (int i = 0; i < length && i < 2; ++i)
    {
        oss << "0x"
            << std::setw(2)
            << int(data[i])
            << " ";
    }

    return oss.str();
}

/***********************************************************************/
void MidiParser::feed(uint8_t byte)
{
    // Realtime messages (doivent passer partout)
    if (byte >= 0xF8)
    {
        MidiMessage msg;
        msg.status = byte;
        msg.length = 0;
        push(msg);
        return;
    }

    // Status byte
    if (byte & 0x80)
    {
        runningStatus = byte;

        dataIndex = 0;
        expected = getDataLength(byte);

        current.status = byte;
        current.length = expected;

        // messages sans data (rare)
        if (expected == 0)
        {
            push(current);
        }

        return;
    }

    // Data byte
    if (runningStatus == 0)
        return; // ignore junk avant status

    current.status = runningStatus;
    current.length = expected;

    if (dataIndex < 2)
        current.data[dataIndex++] = byte;

    if (dataIndex >= expected)
    {
        push(current);
        dataIndex = 0;
    }
}

/***********************************************************************/
bool MidiParser::pop(MidiMessage& out)
{
    if (fifo.empty())
        return false;

    out = fifo.front();
    fifo.pop();
    return true;
}

/***********************************************************************/
void MidiParser::push(const MidiMessage& msg)
{
    fifo.push(msg);
}

/***********************************************************************/
int MidiParser::getDataLength(uint8_t status)
{
    switch (status & 0xF0)
    {
        case 0xC0:
        case 0xD0:
            return 1;

        case 0x80:
        case 0x90:
        case 0xA0:
        case 0xB0:
        case 0xE0:
            return 2;
    }

    switch (status)
    {
        case 0xF1:
        case 0xF3:
            return 1;
        case 0xF2:
            return 2;
        default:
            return 0;
    }
}

} // namespace LUMIDI
