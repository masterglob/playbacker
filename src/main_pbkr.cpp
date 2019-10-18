#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdexcept>
#include <iostream>

#include <signal.h>

#include "pbkr_gpio.h"
#include "pbkr_display.h"


/*******************************************************************************
 *
 *******************************************************************************/
using namespace PBKR;
using namespace std;

extern void *signal(int sig, void (*func)(int));


namespace
{
static volatile int keepRunning = 1;
static const int DISPLAY_I2C_ADDRESS (0x27);

DISPLAY::I2C_Display display( DISPLAY_I2C_ADDRESS);
const GPIOs::Button BTN (GPIOs::GPIO::pinToId(16));
const GPIOs::Led led(GPIOs::GPIO::pinToId(11));

void intHandler(int dummy) {
	keepRunning = 0;
	led.off();
	display.noBacklight();
	display.noDisplay();
	display.noCursor();
}
}

/*******************************************************************************
 *
 *******************************************************************************/
int main (int argc, char**argv)
{
	static const int POLL_PERIOD_MS(100);
	static const float LED_PERIOD_MS(1500);
	GPIOs::GPIO::begin();

	signal(SIGINT, intHandler);
	try {
		display.begin();
		display.backlight();
		display.print("BUILD " __TIME__ "\n");
		display.print("World!\n");

		printf("Press btn...!\n");

		float led_time_ms(0);
		bool  led_on(true);
		while (keepRunning)
		{
			if (BTN.pressed()) break;
			delay (POLL_PERIOD_MS);
			led_time_ms -= POLL_PERIOD_MS;
			if (led_time_ms <= 0)
			{
				led_time_ms += LED_PERIOD_MS;
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
