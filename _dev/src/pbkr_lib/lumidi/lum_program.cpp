#include "lumidi/lum_program.h"

#include <stdint.h>
#include <string>
#include <exception>
#include <cstdint>
#include <queue>
#include <iostream>
#include <chrono>
#include <thread>

using namespace std;

namespace LUMIDI
{

Program* Program::m_instance{nullptr};

/***********************************************************************/
Program::Program(uint8_t* leds) : m_ledsMap(leds)
{
    if (m_instance)
        throw runtime_error("Only one isntance of LUMIDI::Program() is possible");
    m_thread = thread([this](){threadImpl();});
    m_instance = this;
}

/***********************************************************************/
Program::~Program()
{
    m_instance = nullptr;
    m_running = false;
    if (m_thread.joinable())
        m_thread.join();
}

/***********************************************************************/
void Program::feed(const MidiMessage& msg)
{
    if (!m_instance) return;
    unique_lock<mutex> lock(m_instance->m_mutex);

    // TOOD : apply rules to m_ledsMap!
}

/***********************************************************************/
void Program::threadImpl()
{
    cout << "Started 'Program' thread\n";
    while (m_running)
    {
        try
        {
            this_thread::sleep_for(chrono::milliseconds(10));

        }
        catch (const std::exception& e) {
            cerr << "Program::thread exception: " << e.what() << endl;

            this_thread::sleep_for(chrono::milliseconds(100));
        }

    }
    cout << "Terminated 'Program' thread\n";
}

} // namespace LUMIDI
