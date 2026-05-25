#pragma once

#include <atomic>
#include <stdint.h>
#include <string>
#include <cstdint>
#include <queue>
#include <mutex>
#include <thread>

#include "lumidi/lum_types.h"

namespace LUMIDI
{

class Program
{
public:
    explicit Program(uint8_t* leds = nullptr);
    ~Program();
    static void feed(const MidiMessage&);

private:
    inline static Program* instance(){return m_instance;}
    static Program* m_instance;
    void threadImpl();

    std::atomic_bool m_running{true};
    std::mutex m_mutex;
    std::thread m_thread;
    uint8_t* m_ledsMap{nullptr};

};

} // namespace LUMIDI
