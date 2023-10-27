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
    inline MIDI_Ctrl_Cfg(const string& rDev, const string& rName, bool rIn, bool rOut):
        device(rDev), name(rName), isInput(rIn), isOutput(rOut){}
    inline MIDI_Ctrl_Cfg(): isInput(false), isOutput(false){}
};

struct MIDI_Event_Type
{
    enum Event_Class
    {
        EC_NOTE_OFF,
        EC_NOTE_ON,
        EC_POLY_A_T,
        EC_CC,
        EC_PC,
        EC_MONO_A_T,
        EC_PITCH,
        EC_UNSUPPORTED,
    };
    Event_Class eventType;
    uint8_t eventId; // will be PC, or CC number or note number...
    MIDI_Event_Type(const MIDI_Msg& msg);
    string toString(void)const;
};

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

struct MIDI_Ctrl_Instance
{
    MIDI_Ctrl_Cfg cfg;
    MIDI_Controller* pCtrl;
    MIDI_Ctrl_Instance(const MIDI_Ctrl_Cfg& cfgRef):
        cfg(cfgRef), pCtrl(nullptr){};
};

using MIDI_Ctrl_Instance_Vect = std::vector<MIDI_Ctrl_Instance>;


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
    MIDI_Ctrl_Instance_Vect getControllers(void);
    void onDisconnect (MIDI_Controller* pCtrl);
    inline void doMidiLearn(MainMenu::Key key){mMidiLearn = key;}
    inline void cancelMidiLearn(){mMidiLearn = MainMenu::KEY_NONE;}
    inline MainMenu::Key getMidiLearn(){return mMidiLearn;}
private:
    MIDI_Controller_Mgr(void);
    void onInputConnect (MIDI_Ctrl_Instance& cfg);
    virtual void body(void);
    MIDI_Ctrl_Instance_Vect m_InputControllers;
    std::mutex m_mutex;
    MainMenu::Key mMidiLearn;
};
extern MIDI::MIDI_Controller_Mgr& midiMgrInstance;
const MIDI_Ctrl_Cfg& getLastMidiDevicePlugged();

}  // namespace MIDI
} // namespace PBKR
#endif // I_pbkr_midi_h_I
