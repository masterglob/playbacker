#ifndef I_pbkr_midi_h_I
#define I_pbkr_midi_h_I

#include "pbkr_config.h"
#include "pbkr_utils.h"
#include <mutex>
#include <inttypes.h>
#include <alsa/asoundlib.h>
#include <sys/soundcard.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include <stdexcept>

namespace PBKR
{
namespace MIDI
{
/**
 * A midi message
 */
struct MIDI_Msg{
    MIDI_Msg(const uint8_t* data, const size_t len);
    virtual ~MIDI_Msg (void);
    const size_t m_len;
    const uint8_t* m_msg;
};

struct MIDI_Ctrl_Cfg
{
    std::string device;
    std::string name;
    bool isInput;
    bool isOutput;
};

typedef std::vector<MIDI_Ctrl_Cfg,std::allocator<MIDI_Ctrl_Cfg>> MIDI_Ctrl_Cfg_Vect;

/***
 * Derive this class to receive MIDI events from MIDI_Controller
 */
struct MIDI_Event
{
    virtual ~MIDI_Event(void){}
    virtual void onMidiEvent (const MIDI_Msg& msg, const std::string& fromDevice) = 0;
    virtual void onDisconnectEvent (const char* device) = 0;
};

/*******************************************************************************
 * MIDI CONTROLLER
 *******************************************************************************/

class MIDI_Controller : private Thread
{
public:
    MIDI_Controller(const MIDI_Ctrl_Cfg& cfg, MIDI_Event& receiver);
    virtual ~MIDI_Controller(void);
private:
    virtual void body(void);
    snd_rawmidi_t* m_midiin;
    snd_rawmidi_t* m_midiout;
    MIDI_Event & m_receiver;
    const MIDI_Ctrl_Cfg m_cfg;
}; // class MIDI_Controller


/*******************************************************************************
 * MIDI CONTROLLER MANAGER
 *******************************************************************************/

class MIDI_Controller_Mgr
{
public:
    MIDI_Controller_Mgr(void);
    void loop(void);
    virtual ~MIDI_Controller_Mgr(void){}
    virtual void onInputConnect (const MIDI_Ctrl_Cfg& cfg) = 0;
    virtual void onInputDisconnect (const MIDI_Ctrl_Cfg& cfg) = 0;
    void onDisconnect (const char* device);
    const MIDI_Ctrl_Cfg_Vect getControllers(void);
private:
    MIDI_Ctrl_Cfg_Vect m_InputControllers;
    std::mutex m_mutex;
};


}  // namespace MIDI
} // namespace PBKR
#endif // I_pbkr_midi_h_I
