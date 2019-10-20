#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include "pbkr_gpio.h"
#include "pbkr_display.h"


/*******************************************************************************
 *
 *******************************************************************************/
using namespace PBKR;
using namespace std;


namespace
{
static const int DISPLAY_I2C_ADDRESS (0x27);
static const GPIOs::GPIO GPIO_I2C_POW(GPIOs::GPIO_17_PIN11,INPUT,"I2C POWER");
DISPLAY::I2C_Display display( DISPLAY_I2C_ADDRESS, GPIO_I2C_POW);
}
/*******************************************************************************
 *
 *******************************************************************************/
#define STR_EQ(a,b) (strcmp((a),(b)) == 0)
int main (int argc, char**argv)
{
	try {

		if (argc ==1)
		{
			printf("Usage:\n");;
			printf("i    Initialize\n");
			printf("+b   backlight ON (should be set first!)\n");
			printf("-b   backlight OFF\n");
			printf("+c   Cursor ON\n");
			printf("-c   Cursor OFF\n");
			printf("+B   Blink ON\n");
			printf("-B   Blink OFF\n");
			printf("e    Erase screen\n");
			printf("t xx Print text (including newline)\n");
			return 0;
		}
		GPIOs::GPIO::begin();
		int i(1);
		while (i < argc)
		{
			const char* arg (argv[i]);
			const char* param ((i + 1 < argc ? argv[i+1] : ""));
			if (STR_EQ(arg,"i"))
			{
				display.begin();
			}
			else if (STR_EQ(arg,"-b"))
			{
				display.noBacklight();
			}
			else if (STR_EQ(arg,"+b"))
			{
				display.backlight();
			}
			else if (STR_EQ(arg,"-B"))
			{
				display.noBlink();
			}
			else if (STR_EQ(arg,"+B"))
			{
				display.blink();
			}
			else if (STR_EQ(arg,"-c"))
			{
				display.noCursor();
			}
			else if (STR_EQ(arg,"+c"))
			{
				display.cursor();
			}
			else if (STR_EQ(arg,"e"))
			{
				display.clear();
				display.home();
			}
			else if (STR_EQ(arg,"t"))
			{
				i++;
				display.print (param);
				display.print ("\n");
			}
			else
			{
				printf ("Unknown command:%s\n",arg);
				throw;
			}
			i++;
		}
	}
	catch( const std::exception& e ) {
		printf("Exception : %s\n", e.what());
		return 1;
	}
	return 0;
}
