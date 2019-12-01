
#include <stdio.h>
#include <unistd.h>         //Used for UART
#include <fcntl.h>          //Used for UART
#include <termios.h>        //Used for UART

#include "pbkr_gpio.h"
using namespace PBKR;

int main (int argc, char**argv)
{
    GPIOs::Input btn1 (GPIOs::GPIO_17_PIN11);
    GPIOs::Input btn2 (GPIOs::GPIO_27_PIN13);
    GPIOs::Input btn3 (GPIOs::GPIO_23_PIN16);

    GPIOs::GPIO::begin();

    int p1 (btn1.pressed());
    int p2 (btn2.pressed());
    int p3 (btn3.pressed());
    while (1)
    {
        usleep(100*1000);
        {
            const int n(btn1.pressed());
            int& p(p1);
            if (n != p)
            {
                p = n;
                if (p) printf("p1 pressed\n");
            }
        }
        {
            const int n(btn2.pressed());
            int& p(p2);
            if (n != p)
            {
                p = n;
                if (p) printf("p2 pressed\n");
            }
        }
        {
            const int n(btn3.pressed());
            int& p(p3);
            if (n != p)
            {
                p = n;
                if (p) printf("p3 pressed\n");
            }
        }
    }
	return 0;
}

