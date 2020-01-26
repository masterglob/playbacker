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
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

#include "pbkr_config.h"
#include "pbkr_gpio.h"
#include "pbkr_display_mgr.h"
#include "pbkr_utils.h"
#include "pbkr_snd.h"
#include "pbkr_midi.h"
#include "pbkr_webserv.h"
#include "pbkr_osc.h"
#include "pbkr_menu.h"
#include "pbkr_cfg.h"
#include "pbkr_projects.h"
#include "pbkr_console.h"
#include "pbkr_api.h"


/*******************************************************************************
 *
 *******************************************************************************/
using namespace PBKR;
using namespace std;


/*******************************************************************************
 *
 *******************************************************************************/

namespace
{

void print_help(const char* name)
{
    printf("Usage: %s [-h] [-i] <pbdevice=hw:0> <clickdevice=hw:1>\n", name);
    printf("Options:\n");
    printf(" -h  This help:\n");
    printf(" -v  Show versino & exit\n");
    printf(" -i  Use interactive keyboard console:\n");
}

void print_version(void)
{
    printf("PBKR V" PBKR_VERSION "\n");
}

/*******************************************************************************
 * SIGNAL HANDLERS
 *******************************************************************************/
void intHandler(int dummy) {
    PBKR::DISPLAY::DisplayManager::instance().onEvent(PBKR::DISPLAY::DisplayManager::evEnd);
    Thread::doExit();
}

void alarmHandler(int sig)
{
  signal(SIGALRM, SIG_IGN);          /* ignore this signal       */
  printf("Failed to exit cleany. Force exit 1\n");
  exit (1);
}


static Console console;


/*******************************************************************************
 * PROCESS OSC EVENTS
 *******************************************************************************/
class OSC_Impl : public OSC::OSC_Event
{
public:
    virtual ~OSC_Impl(void){}
    virtual void forceRefresh    (void)
    {
        DISPLAY::DisplayManager::instance().forceRefresh();
        if (OSC::p_osc_instance) OSC::p_osc_instance->updateProjectList();
    }
    virtual void onPlayEvent    (void){fileManager.startReading ();}
    virtual void onStopEvent    (void){fileManager.stopReading ();}
    virtual void onBackward     (void){fileManager.backward ();}
    virtual void onFastForward  (void){fileManager.fastForward ();}
    virtual void onChangeTrack  (const uint32_t idx){fileManager.selectIndex(idx + 1);}
    virtual void setClicVolume  (const float& v){API::setClicVolume(v);}
    virtual std::string onKeyboardCmd  (const std::string& msg){return API::onKeyboardCmd(msg);}
};

} // namsepace

/*******************************************************************************
 *
 *******************************************************************************/
int main (int argc, char**argv)
{

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
        if (strcmp(cmd,"-v") == 0)
        {
            print_version();
            return 0;
        }
        else if (strcmp(cmd,"-i") == 0)
        {
            interactive_console = true;
        }
        else
        {
            hifidac = cmd; // TODO : autodetect "sndrpihifiberry"
            break;
        }
    }

    print_version();

    static const OSC::OSC_Ctrl_Cfg oscCfg (8000,9000);
    static OSC_Impl oscImpl;
    static OSC::OSC_Controller osc(oscCfg, oscImpl);

    static MIDI::MIDI_Controller_Mgr midiMgr;

    WEB::BasicWebSrv srv(fileManager, midiMgr);
	GPIOs::GPIO::begin();

	MidiOutMsg midiCmdToWemos; // The midi command read from file and sent to wemos

	signal(SIGINT, intHandler);
	try {
	    DISPLAY::DisplayManager::instance().onEvent(DISPLAY::DisplayManager::evBegin);
	    setDefaultProject();
	    fileManager.startup();

        if (interactive_console) console.start();
		Fader* ledFader(new Fader(0.5,0,0));
		bool  led_on(true);
		//led.set (led_on);

        float l,r;
        int midi;
		float phase = 0;
        float volume(0.9);
		const float phasestep=(TWO_PI * 440.0)/ 44100.0;
		SOUND::SoundPlayer playerHifi (hifidac);

		setRealTimePriority();

        osc.updateProjectList();

		while (!Thread::isExitting())
		{
			// if (BTN.pressed()) break;

			if (console.doSine)
			{

			    l = sin (phase) * volume * console.volume();
			    r = l;
			}
			else
			{
			    fileManager.getSample(l,r,midi);
                l *=  volume;
                r *=  volume;
			}

			VirtualTime::elapseSample();
            phase += phasestep;
            if (phase > TWO_PI) phase -=TWO_PI;

            l = leftLatency.putSample(l);
            r = rightLatency.putSample (r);

			playerHifi.write_sample(l,r);
			if (midi >=0)
			{
			    midiCmdToWemos.push_back(midi);
			}
			else if (midiCmdToWemos.size() > 0)
			{
			    wemosControl.pushMessage(midiCmdToWemos);
			    midiCmdToWemos.clear();
			}

            // printf("TODO : MIDI byte to send: %02X\n",midi);
			wemosControl.sendByte();

			if (ledFader->update())
			{
			    delete(ledFader);
				ledFader = new Fader(0.5,0,0);
				led_on = not led_on;
				//led.set (led_on);
			}
		}
        PBKR::DISPLAY::DisplayManager::instance().onEvent(PBKR::DISPLAY::DisplayManager::evEnd);
		intHandler(0);
	}
	catch( const std::exception& e ) {
		printf("Exception : %s\n", e.what());
	}

	signal(SIGALRM, alarmHandler);
	alarm(2);
	Thread::join_all();

	return 0;
}

