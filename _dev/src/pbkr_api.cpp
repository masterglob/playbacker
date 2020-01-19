
#include "pbkr_api.h"
#include "pbkr_console.h"
#include "pbkr_display.h"
#include "pbkr_osc.h"

namespace PBKR
{
namespace API
{
using namespace std;
/*******************************************************************************
 * GLOBAL CONSTANTS
 *******************************************************************************/

/*******************************************************************************
 *
 *******************************************************************************/

/*******************************************************************************/
void setClicVolume  (const float& v)
{
    Console * console (Console::instance());
    if (console) console->changeVolume(v,0.01);

    DISPLAY::DisplayManager::instance().info(
            std::string("Clic Vol:") + std::to_string((int)(100*v)) + "%");

    OSC::OSC_Controller* osc (OSC::p_osc_instance);
    if (osc) osc->setClicVolume(v);
}

/*******************************************************************************/
std::string onKeyboardCmd  (const std::string& msg)
{
    static const std::string badCmd ("Unknown Command :");
    // static bool logged (false);
#if 0
    if (msg == "/R")
    {
        Thread::doExit();
        return "Reboot command OK";
    }
#endif
    return badCmd + msg;
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
    const uint8_t cmd(msg.m_msg[0]);
    const uint8_t b1(msg.m_len> 1 ? msg.m_msg[1] : 0);
    const uint8_t b2(msg.m_len> 2 ? msg.m_msg[2] : 0);
    const uint8_t lst(msg.m_msg[msg.m_len-1]);

    if (cfg.name == std::string("TINYPAD MIDI 1"))
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
                    DISPLAY::DisplayManager::instance().info("Not ass.:NBL");
                    return;
                }
                if (b1 == PAD_BOTT_RIGHT)
                {
                    DISPLAY::DisplayManager::instance().info("Not ass.:NBR");
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
                    fileManager.prevTrack();
                    return;
                case PAD_NEXT:
                    fileManager.nextTrack();
                    return;
                default:
                    return;
                }
            }
        }
        //SYSEX msg?
        if (cmd == SYS_EX_START && lst == SYS_EX_STOP)
        {
            if (msg.m_len ==8 && msg.m_msg[1] == 0x7F &&
                    msg.m_msg[2] == 0x7F && msg.m_msg[3] == 0x04 &&
                    msg.m_msg[4] == 0x01 && msg.m_msg[5] == 0x00)
            {
                API::setClicVolume(msg.m_msg[6] / 128.0);
                return;
            }
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
    printf("Recv MIDI event from device <%s>:[",cfg.name.c_str());
    for (size_t i(0); i< msg.m_len;i++)
        printf("%02X ",msg.m_msg[i]);
    printf("]\n");
} // onMidiEvent


} // namespace API
} // namespace PBKR
