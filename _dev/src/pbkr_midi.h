/**
 * This package is only intended to manage input MIDI devices for controlling the playback
 */

#ifndef I_pbkr_midi_h_I
#define I_pbkr_midi_h_I

#include "pbkr_config.h"
#include "pbkr_menu.h"
#include "pbkr_utils.h"
#include <mutex>
#include <inttypes.h>
#include <alsa/asoundlib.h>
#include <sys/soundcard.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include <stdexcept>

#define MIDI_CMD_CC 0xB0
#define MIDI_CHANNEL_16 0x0F
#define MIDI_CC_VOLUME 0x07

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

/*******************************************************************************
 * MIDI CONTROLLER
 *******************************************************************************/
class MIDI_Controller_Mgr;

class MIDI_Controller : private Thread
{
public:
    MIDI_Controller(const MIDI_Ctrl_Cfg& cfg,  MIDI_Controller_Mgr& mgr);
    virtual ~MIDI_Controller(void);
private:
    virtual void body(void);
    snd_rawmidi_t* m_midiin;
    snd_rawmidi_t* m_midiout;
    const MIDI_Ctrl_Cfg m_cfg;
    MIDI_Controller_Mgr& m_mgr;
}; // class MIDI_Controller


/*******************************************************************************
 * MIDI CONTROLLER MANAGER
 *******************************************************************************/
/** Check for new MIDI controllers to be plugged / unplugged
 * and instantiate a MIDI_Controller for each new controller found
 */
class MIDI_Controller_Mgr : private Thread
{
public:
    static MIDI_Controller_Mgr& instance();
    virtual ~MIDI_Controller_Mgr(void){}
    void loop(void);
    const MIDI_Ctrl_Cfg_Vect getControllers(void);
    void onDisconnect (const MIDI_Ctrl_Cfg& cfg);
    inline void doMidiLearn(MainMenu::Key key){mMidiLearn = key;}
    inline void cancelMidiLearn(){mMidiLearn = MainMenu::KEY_NONE;}
    inline MainMenu::Key getMidiLearn(){return mMidiLearn;}
private:
    MIDI_Controller_Mgr(void);
    void onInputConnect (const MIDI_Ctrl_Cfg& cfg);
    virtual void body(void);
    MIDI_Ctrl_Cfg_Vect m_InputControllers;
    std::mutex m_mutex;
    MainMenu::Key mMidiLearn;
};
extern MIDI::MIDI_Controller_Mgr& midiMgrInstance;

}  // namespace MIDI
} // namespace PBKR
#endif // I_pbkr_midi_h_I
