
#include <stdio.h>
#include <iostream>
#include <iomanip>

#include "pbkr_wav.h"

using namespace PBKR;

static const char* eventTypeName(MidiEventType t) {
    switch (t) {
        case MidiEventType::NoteOff:         return "NoteOff        ";
        case MidiEventType::NoteOn:          return "NoteOn         ";
        case MidiEventType::PolyPressure:    return "PolyPressure   ";
        case MidiEventType::ControlChange:   return "ControlChange  ";
        case MidiEventType::ProgramChange:   return "ProgramChange  ";
        case MidiEventType::ChannelPressure: return "ChannelPressure";
        case MidiEventType::PitchBend:       return "PitchBend      ";
        default:                             return "Unknown        ";
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file.mid> [max_events]\n";
        return 1;
    }

    const char* path       = argv[1];
    const size_t max_print = (argc >= 3) ? (size_t)atoi(argv[2]) : 32;

    // -- Load and parse --------------------------------------------------------
    MidiFile midi(path);
    std::vector<MidiEvent> events = midi.buildEventList();

    // -- File summary ----------------------------------------------------------
    std::cout << "File    : " << path << "\n"
              << "Format  : " << midi.format()     << "\n"
              << "Tracks  : " << midi.numTracks()  << "\n"
              << "Ticks/QN: " << midi.ticksPerQN() << "\n"
              << "Events  : " << events.size()     << "\n"
              << "\n";

    // -- Header ----------------------------------------------------------------
    std::cout << std::left
              << std::setw(6)  << "Idx"
              << std::setw(12) << "Time(s)"
              << std::setw(10) << "Tick"
              << std::setw(17) << "Type"
              << std::setw(5)  << "Ch"
              << std::setw(8)  << "Data1"
              << std::setw(8)  << "Data2"
              << "\n"
              << std::string(66, '-') << "\n";

    // -- Events ----------------------------------------------------------------
    const size_t max_print2 = max_print > 0 ? max_print : events.size();
    const size_t n = (events.size() < max_print2) ? events.size() : max_print2;

    for (size_t i = 0; i < n; ++i) {
        const MidiEvent& ev = events[i];
        std::cout << std::left
                  << std::setw(6)  << i
                  << std::setw(12) << std::fixed << std::setprecision(4) << ev.time_sec
                  << std::setw(10) << ev.tick
                  << std::setw(17) << eventTypeName(ev.type)
                  << std::setw(5)  << (int)ev.channel
                  << std::setw(8)  << (int)ev.data1
                  << std::setw(8)  << (int)ev.data2
                  << "\n";
    }

    if (events.size() > max_print)
        std::cout << "... (" << (events.size() - max_print) << " more events)\n";

    return 0;
}
