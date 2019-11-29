#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdexcept>
#include <iostream>

#include <signal.h>
#include <math.h>
#include <vector>
#include <fstream>
#include <sstream>

#include "pbkr_config.h"
#include "pbkr_gpio.h"
#include "pbkr_display.h"
#include "pbkr_utils.h"
#include "pbkr_snd.h"
#include "pbkr_midi.h"
#include "pbkr_webserv.h"
#include "pbkr_osc.h"


/*******************************************************************************
 *
 *******************************************************************************/
using namespace PBKR;
using namespace std;


namespace
{
static volatile int keepRunning = 1;
const GPIOs::Input BTN (GPIOs::GPIO::pinToId(13));
const GPIOs::Led led(GPIOs::GPIO::pinToId(15));

static FileManager manager (MOUNT_POINT);
static void intHandler(int dummy);
static inline void setClicVolume  (const float& v);
static inline std::string onKeyboardCmd  (const std::string& msg);

class Console:public Thread
{
public:
    Console(void): Thread(),
    exitreq(false),
    doSine(false),
    _volume(0.11),
    _fader(NULL)
{}
    virtual ~Console(void){}
    virtual void body(void)
    {
        printf("Console Ready\n");
        while (not exitreq)
        {
            printf("> ");
            fflush(stdout);
            const char c ( getch());
            printf("%c\n",c);
            switch (c) {
            case 'q':
            case 'Q':
                printf("Exit requested\n");
                PBKR::DISPLAY::DisplayManager::instance().onEvent(PBKR::DISPLAY::DisplayManager::evEnd);
                exitreq = true;
                break;
                // Sine on/off
            case 's':doSine = not doSine;
                break;
            case '+':
                changeVolume (_volume + 0.02);
                break;
            case '-':
                changeVolume (_volume - 0.02);
                break;
            case 'v':
                printf("Current volume = %d%%\n", (int)(_volume *100));
                break;
            case 0x0A:
            case ' ':
                if (manager.reading())
                    manager.stopReading();
                else
                    manager.startReading ();
                break;
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                manager.selectIndex (c - '1');
                break;
            default:
                printf("Unknown command :(0x%02X)\n",c);
                break;
            }
        }
        printf("Console Exiting\n");
        keepRunning = 0;
    }
    bool exitreq;
    bool doSine;
    float volume(void);
    void changeVolume (float v, const float duration = volumeFaderDurationS);
private:
    static const float volumeFaderDurationS;
    float _volume;
    Fader* _fader;
};
const float Console::volumeFaderDurationS (0.1);


void Console::changeVolume (float v, const float duration)
{
    if (v > 1.0) v = 1.0;
    if (v < 0.0) v = 0.0;
    // printf("New volume : %d%%\n",(int)(_volume*100));
    const float v0(volume()); // !! Compute volume before destroying fader!
    if (_fader) delete (_fader);
    _fader =  new Fader(duration,v0, v);
    //printf("New volume = %d%%\n", (int)(_volume *100));
}

float Console::volume(void)
{
    if (_fader)
    {
        const float f(_fader->position());
        if (_fader->done())
        {
            _volume = f;
            // printf("FADER DONE\n");
            delete (_fader);
            _fader=NULL;
        }
        else
            return f;
    }
    return _volume;
}

static Console console;
void intHandler(int dummy) {
    PBKR::DISPLAY::DisplayManager::instance().onEvent(PBKR::DISPLAY::DisplayManager::evEnd);
    keepRunning = 0;
    console.exitreq = true;
    led.off();
}

class MIDI_Input_Mgr;

struct Evt : MIDI::MIDI_Event
{
    Evt(MIDI_Input_Mgr* mgr):m_mgr(mgr){}
    virtual ~Evt(void){}
    virtual void onMidiEvent (const MIDI::MIDI_Msg& msg, const std::string& fromDevice);
    virtual void onDisconnectEvent (const char* device);
private:
    MIDI_Input_Mgr*m_mgr;
};

class MIDI_Input_Mgr : public MIDI::MIDI_Controller_Mgr
{
public:
    MIDI_Input_Mgr (void):MIDI::MIDI_Controller_Mgr (),
    m_evt(this),
    m_Thread(this)
    {
    }
    void start(void){
        m_Thread.start();
    }
private:
    Evt m_evt;
    struct M_Thread : public Thread
    {
        M_Thread(MIDI_Input_Mgr* mgr):m_mgr(mgr){start();}
        virtual void body(void)
        {
            printf("MIDI_Input_Mgr:loop()\n");
            m_mgr->loop();
        };
        MIDI_Input_Mgr* m_mgr;
    };
    M_Thread m_Thread;
    virtual void onInputConnect (const MIDI::MIDI_Ctrl_Cfg& cfg)
    {
        printf("Found: %s (%s)\n", cfg.name.c_str(), cfg.device.c_str());
        MIDI::MIDI_Controller* midi_ctrl(new MIDI::MIDI_Controller (cfg, m_evt));
        DISPLAY::DisplayManager::instance().info(cfg.name + " connected.");
        (void)midi_ctrl;
    }
    virtual void onInputDisconnect (const MIDI::MIDI_Ctrl_Cfg& cfg)
    {
        DISPLAY::DisplayManager::instance().info(cfg.name + " disconnected.");
    }
};
static MIDI_Input_Mgr midiMgr;


void Evt::onMidiEvent (const MIDI::MIDI_Msg& msg, const std::string& fromDevice)
{
    static const uint8_t SYS_EX_START(0xF0);
    static const uint8_t SYS_EX_STOP(0xF7);
    if (msg.m_len == 0) return;
    const uint8_t cmd(msg.m_msg[0]);
    const uint8_t b1(msg.m_len> 1 ? msg.m_msg[1] : 0);
    const uint8_t b2(msg.m_len> 2 ? msg.m_msg[2] : 0);
    const uint8_t lst(msg.m_msg[msg.m_len-1]);

    if (fromDevice == std::string("TINYPAD MIDI 1"))
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
                        manager.selectIndex (i);
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
                    manager.startReading ();
                    return;
                case PAD_STOP:
                    manager.stopReading();
                    return;
                case PAD_PREV:
                    manager.prevTrack();
                    return;
                case PAD_NEXT:
                    manager.nextTrack();
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
                setClicVolume(msg.m_msg[6] / 128.0);
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
    printf("Recv MIDI event from device <%s>:[",fromDevice.c_str());
    for (size_t i(0); i< msg.m_len;i++)
        printf("%02X ",msg.m_msg[i]);
    printf("]\n");

}
void Evt::onDisconnectEvent (const char* device)
{
    printf("Disconnect %s\n",device);
    m_mgr->onDisconnect(device);
}

void print_help(const char* name)
{
    printf("Usage: %s [-h] [-i] <pbdevice=hw:0> <clickdevice=hw:1>\n", name);
    printf("Options:\n");
    printf(" -h  This help:\n");
    printf(" -i  Use interactive keyboard console:\n");
}

void setClicVolume  (const float& v)
{
    console.changeVolume(v,0.01);
    DISPLAY::DisplayManager::instance().info(
            std::string("Clic Vol:") + std::to_string((int)(100*v)) + "%");
    if (OSC::p_osc_instance)
        OSC::p_osc_instance->setClicVolume(v);
}

std::string onKeyboardCmd  (const std::string& msg)
{
    static const std::string badCmd ("Unknown Command :");
    // static bool logged (false);
    if (msg == "/R")
    {
        keepRunning = 0;
        return "Reboot command OK";
    }
    return badCmd + msg;
}

class OSC_Event : public OSC::OSC_Event
{
public:
    virtual ~OSC_Event(void){}
    virtual void forceRefresh    (void){DISPLAY::DisplayManager::instance().forceRefresh();}
    virtual void onPlayEvent    (void){manager.startReading ();}
    virtual void onStopEvent    (void){manager.stopReading ();}
    virtual void onChangeTrack  (const uint32_t idx){manager.selectIndex(idx);}
    virtual void setClicVolume  (const float& v){::setClicVolume(v);}
    virtual std::string onKeyboardCmd  (const std::string& msg){return ::onKeyboardCmd(msg);}
};
static OSC_Event oscEvent;

static const OSC::OSC_Ctrl_Cfg oscCfg (8000,9000);
static OSC::OSC_Controller osc(oscCfg,oscEvent);

} // namsepace

/*******************************************************************************
 *
 *******************************************************************************/
int main (int argc, char**argv)
{
    WEB::BasicWebSrv srv(manager, midiMgr);


    using namespace PBKR;
    /*
     * usage $0 <hw:x> <hw:y>
     * with x= I2S DAC
     * with y=ALSA default DAC
     */
    int argi(1);
    bool interactive_console(false);
    const char* hifidac = "hw:0";
    while (argi < argc)
    {
        const char* const cmd(argv[argi++]);
        if (strcmp(cmd,"-h") == 0)
        {
            print_help(argv[0]);
            return 0;
        }
        else if (strcmp(cmd,"-i") == 0)
        {
            interactive_console = true;
        }
        else
        {
            hifidac = cmd;
            break;
        }
    }

	GPIOs::GPIO::begin();

	signal(SIGINT, intHandler);
	try {
	    midiMgr.start();
	    DISPLAY::DisplayManager::instance().onEvent(DISPLAY::DisplayManager::evBegin);
	    manager.startup();
		printf("Press btn...!\n");
        if (interactive_console) console.start();
		Fader* ledFader(new Fader(0.5,0,0));
		bool  led_on(true);
		led.set (led_on);

        float l,r;
        float l2,r2;
		float phase = 0;
        float volume(0.9);
        float volume2(0.90);
		const float phasestep=(TWO_PI * 440.0)/ 44100.0;
		SOUND::SoundPlayer playerHifi (hifidac);

		setRealTimePriority();

		while (keepRunning)
		{
			if (BTN.pressed()) break;

			if (console.doSine)
			{

			    l = sin (phase) * volume * console.volume();
			    r = l;
			}
			else
			{
			    manager.getSample(l,r,l2,r2);
                l *=  volume;
                r *=  volume;
                l2 *= volume2 * (console.volume() + 0.1);
                r2 *= volume2 * (console.volume() + 0.1);
			}

			VirtualTime::elapseSample();
            phase += phasestep;
            if (phase > TWO_PI) phase -=TWO_PI;

			playerHifi.write_sample(l,r);
			// TODO : send serail to WEMOS!

			if (ledFader->update())
			{
			    delete(ledFader);
				ledFader = new Fader(0.5,0,0);
				led_on = not led_on;
				led.set (led_on);
			}
		}
		printf("Button pressed!\n");
        PBKR::DISPLAY::DisplayManager::instance().onEvent(PBKR::DISPLAY::DisplayManager::evEnd);
		intHandler(0);
	}
	catch( const std::exception& e ) {
		printf("Exception : %s\n", e.what());
		return 1;
	}
	return 0;
}

