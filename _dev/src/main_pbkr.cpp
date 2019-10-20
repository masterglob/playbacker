#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdexcept>
#include <iostream>

#include <signal.h>
#include <math.h>

#include "pbkr_gpio.h"
#include "pbkr_display.h"
#include "pbkr_utils.h"
#include "pbkr_snd.h"


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

static const char* MOUNT_POINT("/mnt/usb/PBKR/");
static FileManager manager (MOUNT_POINT);
static void intHandler(int dummy);



class Console:public Thread
{
public:
    Console(void): Thread(),
    exitreq(false),
    doSine(false),
    _volume(0.01),
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
            case '0':
                manager.playIndex (0);
                break;
            case '1':
                manager.playIndex (1);
                break;
            case '2':
                manager.playIndex (2);
                break;
            case '3':
                manager.playIndex (3);
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
    if (_fader)
    {
        free (_fader);
    }
    _fader =  new Fader(duration,v0, v);
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
            free (_fader);
            _fader=NULL;
        }
        else
            return f;
    }
    return _volume;
}

static Console console;
void intHandler(int dummy) {
    keepRunning = 0;
    console.exitreq = true;
    led.off();
    DISPLAY::display.noBacklight();
    DISPLAY::display.noDisplay();
    DISPLAY::display.noCursor();
}

} // namsepace

/*******************************************************************************
 *
 *******************************************************************************/
int main (int argc, char**argv)
{
	GPIOs::GPIO::begin();

	signal(SIGINT, intHandler);
	try {
	    DISPLAY::display.begin();
	    DISPLAY::display.backlight();
	    DISPLAY::display.print("BUILD " __TIME__ "\n");
	    DISPLAY::display.print("World!\n");

		printf("Press btn...!\n");
        console.start();
		Fader* ledFader(new Fader(0.5,0,0));
		bool  led_on(true);
		led.set (led_on);

		float l,r;
		float phase = 0;
		float volume(0.2);
		const float phasestep=(TWO_PI * 440.0)/ 44100.0;
		SOUND::SoundPlayer player1 ("hw:0");
		SOUND::SoundPlayer player2 ("hw:1");



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
			    l = 0;
			    r = l;
			}

			VirtualTime::elapseSample();
            phase += phasestep;
            if (phase > TWO_PI) phase -=TWO_PI;

			player1.write_sample(l,r);
			player2.write_sample(l,r);

			if (ledFader->update())
			{
				free(ledFader);
				ledFader = new Fader(0.5,0,0);
				led_on = not led_on;
				led.set (led_on);
			}
		}
		printf("Button pressed!\n");
		intHandler(0);
	}
	catch( const std::exception& e ) {
		printf("Exception : %s\n", e.what());
		return 1;
	}
	return 0;
}

