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
#include "pbkr_osc.h"
#include "pbkr_menu.h"
#include "pbkr_cfg.h"
#include "pbkr_projects.h"
#include "pbkr_console.h"
#include "pbkr_api.h"
#include "pbkr_websrv.h"


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
    printf("Usage: %s [-h] [-i] <pbdevice=hw:1> <clickdevice=hw:0,1>\n", name);
    printf("Options:\n");
    printf(" -h  This help:\n");
    printf(" -v  Show version & exit\n");
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

static GPIOs::Led redLed(GPIOs::GPIO_13_PIN33);
void manageLed(const bool hasSig)
{
    static bool isRedLedOn = false;
    static Fader ledFader(0.05, 0.0, 1.0);
    if (hasSig)
    {
        if (not isRedLedOn)
        {
            redLed.on();
            isRedLedOn = true;
        }
        ledFader.restart();
    }
    else
    {
        ledFader.update();
        if (ledFader.done())
        {
            if (isRedLedOn)
            {
                redLed.off();
                isRedLedOn = false;
            }
        }
    }
}

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
    int dacIdx = 0;
    const char* hifidac = NULL;
    const char *hdmidac = NULL;
    const char *hdphdac = NULL;
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
            if (dacIdx == 0)
            {
                hifidac = cmd; // TODO : autodetect "sndrpihifiberry"
                printf("Using <%s> as HIFIDAC (Playback)\n", hifidac);
            }
            else if (dacIdx == 1)
            {
                hdmidac = cmd; // TODO : autodetect "HDMI"
                printf("Using <%s> as HDMI (Not used)\n", hdmidac);
            }
            else if (dacIdx == 2)
            {
                hdphdac = cmd; // TODO : autodetect "HEADPHONES"
                printf("Using <%s> as HEADPHONES (Clics)\n", hdphdac);
            }
            else
            {
                printf("Unexpected parameter : %s\n", cmd);
                print_help(argv[0]);
                return 0;
            }
            dacIdx++;
        }
    }

    print_version();

    if (hifidac == NULL || hdphdac == NULL)
    {
        print_help(argv[0]);
        return 0;
    }

    static const OSC::OSC_Ctrl_Cfg oscCfg (8000,9000);
    static OSC::OSC_Controller osc(oscCfg);

    static MIDI::MIDI_Controller_Mgr midiMgr;

	GPIOs::GPIO::begin();

	MidiOutMsg midiCmdToWemos; // The midi command read from file and sent to wemos

    // TODO : Add a settings parameter for OUT volume (general)
    const float& mainVolume(PBKR::API::samplesVolume);
    const float& samplesVolume(PBKR::API::samplesVolume);
    const float& clicVolume(PBKR::API::clicVolume);

	signal(SIGINT, intHandler);
	try {
	    DISPLAY::DisplayManager::instance().onEvent(DISPLAY::DisplayManager::evBegin);
	    setDefaultProject();
	    fileManager.startup();

        if (interactive_console) console.start();
		Fader* ledFader(new Fader(0.5,0,0));
		bool  led_on(true);
		//led.set (led_on);

		// l/r is the output sample
		// l2/r2 is the clic outputs
        float l,r, l2, r2;
        int midi;
		float phase = 0;
		const float phasestep=(TWO_PI * 440.0)/ 48000.0; // TODO PHASE!

        SOUND::SoundPlayer playerHifi (hifidac);
        (void)hdmidac;
        // SOUND::SoundPlayer playerClic1 (hdmidac); // TODO : cannot open both at the same time.
        SOUND::SoundPlayer playerClic2 (hdphdac);
        // const bool usingHdmi (false); // TODO => make this dynamic
        SOUND::SoundPlayer* playerClic( &playerClic2);

		setRealTimePriority();

        osc.updateProjectList();

		while (!Thread::isExitting())
		{
		    // if (BTN.pressed()) break;
		    bool playing = fileManager.getSample(l,r, l2, r2 ,midi);
		    if (!playing)
		    {
                playerHifi.pause();
                playerClic->pause();
                while (!playing && !Thread::isExitting())
                {
                    usleep(1000*10);
                    playing = fileManager.getSample(l,r, l2, r2 ,midi);
                }
                // Unpause both first, so that putting samples start exactly
                // at the same time
                refreshLatency();
                playerHifi.unpause();
                playerClic->unpause();
		    }
		    // TODO : manage volume & clicVolume
		    l *=  mainVolume * samplesVolume;
		    r *=  mainVolume * samplesVolume;
		    l2 *= mainVolume * clicVolume;
		    r2 *= mainVolume * clicVolume;
		    if (CURRENT_FREQUENCY != playerHifi.sample_rate())
		    {
	            // Check if freq changed
		        playerHifi.set_sample_rate(CURRENT_FREQUENCY);
		        playerClic->set_sample_rate(CURRENT_FREQUENCY);
		    }
		    if (console.doSine)
		    {

		        l2 = sin (phase) * mainVolume * console.volume();
		        r2 = l2;
		    }

			VirtualTime::elapseSample();
            phase += phasestep;
            if (phase > TWO_PI) phase -=TWO_PI;

            // apply latency
            l = leftLatency.putSample(l);
            r = rightLatency.putSample (r);
            l2 = leftClicLatency.putSample(l2);
            r2 = rightClicLatency.putSample (r2);
            midi = midiLatency.putSample(midi);

            playerHifi.write_sample(l,r);
            playerClic->write_sample(l2,r2);
            ::manageLed(abs(l2)+abs(r2) > 0.05);
			if (midi >=0)
			{
			    // printf("TODO : MIDI byte to send: %02X\n",midi);
			    midiCmdToWemos.push_back(midi);
			}
			else if (not midiCmdToWemos.empty())
			{
			    wemosControl.pushMessage(midiCmdToWemos);
			    midiCmdToWemos.clear();
			}

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

