
#include "pbkr_api.h"
#include "pbkr_console.h"
#include "pbkr_display_mgr.h"
#include "pbkr_midi.h"
#include "pbkr_osc.h"
#include "pbkr_cfg.h"
#include "pbkr_midi.h"
#include "pbkr_menu.h"
#include "pbkr_version.h"

#include <stdlib.h>

#define DEBUG_MIDI 0

#define MIDI_NAME_TINYPAD "TINYPAD MIDI 1"
namespace
{
using namespace std;
using namespace PBKR;
float globalVolume = 1.0f;
float clicVolume = 1.0f;
float samplesVolume = 1.0f;
static const float MIN_VOLUME(0.05f);
static const float MAX_VOLUME(0.995f);
inline float normalizeVolume(const float& v)
{
    if (v < MIN_VOLUME) return MIN_VOLUME;
    if (v > MAX_VOLUME) return MAX_VOLUME;
    return v;
}

string getNextWord(string& str)
{
    const char* cStr(str.c_str());
    size_t pos1 = 0;
    // remove leading spaces
    for (;(*cStr) == ' ' && (*cStr) != 0 && pos1++; cStr++);
    if (pos1 > 0)
    {
        str.erase(0,pos1);
    }
    string res;
    size_t wPos(str.find(" "));
    res = str.substr(0, wPos);
    str.erase(0,wPos);
    return res;
}

/*******************************************************************************/
// Replace NOTE ON with 00 Velocity by a NOTE OFF
static uint8_t midiActualCmd(const MIDI::MIDI_Msg& msg)
{
    uint8_t cmd(msg.m_msg[0] & 0xF0);
    if (cmd == 0x90 && msg.m_len> 2 && msg.m_msg[2] == 0)
        cmd ^= 0x10; // 0x90 => 0x80
    return cmd;
}

static void midiDebug(const string& name, const MIDI::MIDI_Msg& msg)
{
#if DEBUG_MIDI
    for (size_t i(0); i< msg.m_len;i++)
        printf("%02X ",msg.m_msg[i]);
    const uint8_t b0(msg.m_msg[0]);
    printf(" => Recv MIDI event from '%s' [", name.c_str());
    const uint8_t b1(msg.m_len> 1 ? msg.m_msg[1] : 0);
    const uint8_t b2(msg.m_len> 2 ? msg.m_msg[2] : 0);
    if ((b0 & 0xF0) == 0xF0)
    {
        printf("<SYSEX>");
    }
    else if (b0 & 0x80)
    {
        const uint8_t cmd((midiActualCmd(msg) & 0x70) >> 4);
        const uint8_t channel(b0 & 0xF);
        // normal message with channel
        const char* names[7] = {"Note Off", "Note On", "Poly Aft.", "CC", "PC", "Chan. Aft.", "Pitch"};
        const bool  has2prms[7] = {true, true, true, true, false, false, true};
        printf("Chan %d, %s(%d", channel, names[cmd], b1);
        if (has2prms[cmd]) printf(", %d", b2);
        printf(")");
    }
    printf("]\n");
#endif
}

string doMidiTest(const string & param)
{
    using namespace MIDI;
    const MIDI_Ctrl_Instance_Vect vect(midiMgrInstance.getControllers());
    for (const MIDI_Ctrl_Instance& inst : vect)
    {
        if (inst.cfg.name == getLastMidiDevicePlugged().name)
        {
            printf("%s, dev=%s In/out = %d/%d\n",
                    inst.cfg.name.c_str(),
                    inst.cfg.device.c_str(),
                    inst.cfg.isInput,
                    inst.cfg.isOutput);
            if (param.empty()) return string("Found ") + inst.cfg.name;

            if (!inst.cfg.isOutput) return inst.cfg.name + " has not output";

            uint8_t msg[32];
            size_t msgLen(0);
            uint8_t val8=0;
            bool firstHalf = true;
            for (const char* buff=param.c_str(); *buff != 0; buff++)
            {
                char c(*buff);
                uint8_t val4=0;
                if (c >= '0' && c <='9')
                {
                    val4 = c - '0';
                }
                else if (c >= 'A' && c <= 'F')
                {
                    val4 = c - 'A' + 10;
                }
                else
                {
                    return "Invalid param";
                }
                if (firstHalf)
                {
                    val8 = val4 * 0x10;
                }
                else
                {
                    if (msgLen > sizeof(msg))
                    {
                        return "Too many data..";
                    }
                    msg[msgLen] = val8 + val4;
                    val8 = 0;
                    msgLen ++;
                }
                firstHalf = ! firstHalf;
            }
            MIDI_Msg midiMsg(msg, msgLen);
            midiDebug(MIDI_NAME_TINYPAD, midiMsg);
            return "TODO";
        }
    }
    return "WIP...";
} // doMidiTest

} // namespace

namespace PBKR
{
namespace API
{
using namespace std;
/*******************************************************************************
 * GLOBAL CONSTANTS
 *******************************************************************************/
const float& samplesVolume(::samplesVolume);
const float& clicVolume(::clicVolume);
const float& globalVolume(::globalVolume);

/*******************************************************************************
 *
 *******************************************************************************/

/*******************************************************************************/
void setGlobalVolume  (const float& v)
{
    ::globalVolume = ::normalizeVolume(v);
}

/*******************************************************************************/
void setSamplesVolume  (const float& v)
{
    ::samplesVolume = ::normalizeVolume(v);
}

/*******************************************************************************/
void setClicVolume  (const float& v)
{
    ::clicVolume = ::normalizeVolume(v);
    /*
    Console * console (Console::instance());
    if (console) console->changeVolume(v,0.01);

    DISPLAY::DisplayManager::instance().info(
            std::string("Clic Vol:") + std::to_string((int)(100*v)) + "%");

    OSC::OSC_Controller* osc (OSC::p_osc_instance);
    if (osc) osc->setClicVolume(v);
    */
}

/*******************************************************************************/
std::string onKeyboardCmd  (const std::string& msg)
{
    string text(msg);
    const string cmd(::getNextWord(text));
    const string p1(::getNextWord(text));
    static const std::string badCmd ("Unknown Command. Type 'HELP'");
    // static bool logged (false);
#if 1
    if (cmd == "/R") // explicitely undocumented
    {
        Thread::doExit();
        return "Reboot command OK";
    }
#endif
    if (cmd == "HELP")
    {
        if (OSC::p_osc_instance != nullptr)
        {
            OSC::p_osc_instance->pushKbdFeedBack("!V  !MEM  !CPU");
            OSC::p_osc_instance->pushKbdFeedBack("ML [L|R|U|D|O|C] (Midi learn)");
        }
        return "Commands :";
    }
    if (cmd == "!V")
    {
        return string("PBKR V" PBKR_VERSION) + " B " + to_string(PBKR_BUILD_ID);
    }
    if (cmd == "!MEM")
    {
        system ("free -k|head -2 | tail -1|awk '{printf \"%d%%\\n\",(100*$3)/$2;}' > /tmp/mem.used");
        const string res (Config::instance().loadStr ("/tmp/mem.used","??",true));
        return string("Used memory : ") + res;
    }
    if (cmd == "!CPU")
    {
        system ("top -n 1 |head -n 2|tail -n 1|awk '{printf \"%s %s\\n\", $1, $2}' > /tmp/cpu.used");
        const string res (Config::instance().loadStr ("/tmp/cpu.used","??",true));
        return res;
    }
    // MIDI LEARN
    if (cmd == "ML")
    {
        using namespace MIDI;
        MIDI_Controller_Mgr& midi(MIDI_Controller_Mgr::instance());
        const MainMenu::Key key(MainMenu::stringToKey(p1));
        if (key == MainMenu::KEY_NONE)
        {
            midi.cancelMidiLearn();
            return "MIDI LEARN CANCELED";
        }
        MIDI::midiMgrInstance.doMidiLearn(key);
        string res = "MIDI Learn for key '"
                + MainMenu::keyToString(key) + "'. Waiting for MIDI event...";
        return res;
    }
    // MIDI TEST (temporary)
    if (cmd == "MT")
    {
        return doMidiTest(p1);
    }
    return badCmd;
}


/*******************************************************************************/
void onPlayEvent    (void){fileManager.startReading ();}
void onPauseEvent    (void){fileManager.pauseReading ();}
void onStopEvent    (void){fileManager.stopReading ();}
void onBackward     (void){fileManager.backward ();}
void onFastForward     (void){fileManager.fastForward ();}
void onChangeTrack  (const uint32_t idx){fileManager.selectIndex(idx + 1);}
void forceRefresh    (void)
{
    DISPLAY::DisplayManager::instance().forceRefresh();
    if (OSC::p_osc_instance) OSC::p_osc_instance->updateProjectList();
}

/*******************************************************************************
 * MIDI
 *******************************************************************************/

/*******************************************************************************/
void onMidiEvent(const MIDI::MIDI_Msg& msg, const MIDI::MIDI_Ctrl_Cfg& cfg)
{
    static const uint8_t SYS_EX_START(0xF0);
    static const uint8_t SYS_EX_STOP(0xF7);
    if (msg.m_len == 0) return;
    const uint8_t cmd(midiActualCmd(msg));
    const uint8_t b1(msg.m_len> 1 ? msg.m_msg[1] : 0);
    const uint8_t b2(msg.m_len> 2 ? msg.m_msg[2] : 0);
    const uint8_t lst(msg.m_msg[msg.m_len-1]);

    // In case of Midi learn, do not apply the event
    const MainMenu::Key learnKey = MIDI::midiMgrInstance.getMidiLearn();
    if (learnKey != MainMenu::KEY_NONE)
    {
        MIDI::MIDI_Event_Type event(msg);
        MIDI::midiMgrInstance.doMidiLearn(MainMenu::KEY_NONE);
        if (OSC::p_osc_instance != nullptr)
        {
            static const string prefix("MIDI learn:");
            OSC::p_osc_instance->pushKbdFeedBack(prefix + event.toString());
        }
        return;
    }

    if (cfg.name == MIDI_NAME_TINYPAD)
    {
        /** TInyPad MINI:
         */
        if (msg.m_len == 3)
        {
            static const uint8_t CMD_NOTE(0x99);
            static const uint8_t CMD_CTRL(0xB9);

            if (cmd == CMD_NOTE && b2 == 0) return;
            if (cmd == CMD_NOTE && b2 != 0)
            {
                // Pads
                static const int nbPads(12);
                const uint8_t padId[nbPads]={0x27,0x30,0x2D,0x2B,0x33,0x31,0x24,0x26,0x28,0x2A,0x2C,0x2E};
                for (uint8_t i(0) ; i < nbPads; i++)
                {
                    if (padId[i] == b1)
                    {
                        fileManager.selectIndex (i + 1);
                        return;
                    }
                }

                // Bottom Controls (Notes)
                static const uint8_t PAD_BOTT_LEFT(0x01);
                static const uint8_t PAD_BOTT_RIGHT(0x02);
                if (b1 == PAD_BOTT_LEFT)
                {
                    PBKR::globalMenu.pressKey( PBKR::MainMenu::KEY_OK);
                    return;
                }
                if (b1 == PAD_BOTT_RIGHT)
                {
                    PBKR::globalMenu.pressKey( PBKR::MainMenu::KEY_CANCEL);
                    return;
                }
            }

            // Left controllers
            static const uint8_t PAD_REFR(0x31);
            static const uint8_t PAD_PREV(0x2F);
            static const uint8_t PAD_NEXT(0x30);
            static const uint8_t PAD_RECO(0x2C);
            static const uint8_t PAD_STOP(0x2E);
            static const uint8_t PAD_PLAY(0x2D);

            if (cmd == CMD_CTRL && b2 == 0) return;
            if (cmd == CMD_CTRL && b2 != 0)
            {
                switch (b1) {
                case PAD_REFR:
                    DISPLAY::DisplayManager::instance().info("Not ass.:REFR");
                    return;
                case PAD_RECO:
                    DISPLAY::DisplayManager::instance().info("Not ass.:REC");
                    return;
                case PAD_PLAY:
                    // Play/pause
                    fileManager.startReading ();
                    return;
                case PAD_STOP:
                    fileManager.stopReading();
                    return;
                case PAD_PREV:
                    fileManager.backward();
                    return;
                case PAD_NEXT:
                    fileManager.fastForward();
                    return;
                default:
                    return;
                }
            }
        }
        //SYSEX msg?
        if (cmd == SYS_EX_START && lst == SYS_EX_STOP)
        {/*
            if (msg.m_len ==8 && msg.m_msg[1] == 0x7F &&
                    msg.m_msg[2] == 0x7F && msg.m_msg[3] == 0x04 &&
                    msg.m_msg[4] == 0x01 && msg.m_msg[5] == 0x00)
            {
                API::setClicVolume(msg.m_msg[6] / 128.0);
                return;
            }*/
            if (msg.m_len == 11 && msg.m_msg[1] == 0x42 &&
                    msg.m_msg[2] == 0x40 && msg.m_msg[3] == 0x00 &&
                    msg.m_msg[4] == 0x01 && msg.m_msg[5] == 0x04 &&
                    msg.m_msg[6] == 0x00 && msg.m_msg[7] == 0x5F &&
                    msg.m_msg[8] == 0x4F)
            {
                const uint8_t bankId( msg.m_msg[9]);
                DISPLAY::DisplayManager::instance().info(
                        std::string("Bank:") + std::to_string(bankId));
                return;
            }
        }
    }
    /*
     * printf("Recv MIDI event from device <%s>:[",cfg.name.c_str());
    for (size_t i(0); i< msg.m_len;i++)
        printf("%02X ",msg.m_msg[i]);
    printf("]\n");
    */
} // onMidiEvent


} // namespace API
} // namespace PBKR
