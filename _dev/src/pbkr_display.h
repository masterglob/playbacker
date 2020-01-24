#pragma once

#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <inttypes.h>
#include <stdint.h>
#include <string>
#include <thread>
#include <mutex>

#include "pbkr_config.h"
#include "pbkr_gpio.h"

namespace PBKR
{
namespace DISPLAY
{
static const int DISPLAY_I2C_ADDRESS (0x27);

static const int DISPLAY_NB_LINES (2);
static const int DISPLAY_WIDTH (16);
/*******************************************************************************
 *
 *******************************************************************************/
class I2C_Display
{
public:
    I2C_Display(const int address);
    virtual ~I2C_Display(void);
    void begin (void);
    void noDisplay();
    void display();
    void noBlink();
    void blink();
    void noCursor();
    void cursor();
    void noBacklight();
    void backlight();

    void clear();
    void home();
    void write(char value);
    void print(const char* txt);
    /** Col & row start at 0 */
    void setCursor(uint8_t col, uint8_t row);
    void line2(void){setCursor(0, 1);}
private:
    void command(uint8_t value);
    void send(uint8_t value, uint8_t mode);
    void write4bits(uint8_t value);
    void expanderWrite(uint8_t value);
    void pulseEnable(uint8_t _data);
    int _file;
    unsigned char _address;
    uint8_t _displayfunction;
    uint8_t _displaycontrol;
    uint8_t _displaymode;
    uint8_t _backlightval;
    uint8_t _currLine;
    uint8_t _nb_lines;
    // GPIO actually not used, but referenced to monitor used GPIOs
    GPIOs::GPIO _sda;
    GPIOs::GPIO _scl;
}; // class I2C_Display


}  // namespace DISPLAY
}
