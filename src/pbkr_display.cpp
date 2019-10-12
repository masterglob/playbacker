#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdexcept>

#include "pbkr_display.h"

// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

// flags for backlight control
#define LCD_BACKLIGHT 0x08
#define LCD_NOBACKLIGHT 0x00

#define En 0x04  // Enable bit
#define Rw 0x02  // Read/Write bit
#define Rs 0x01  // Register select bit

namespace PBKR
{
namespace DISPLAY
{
/*******************************************************************************
 *
 *******************************************************************************/

I2C_Display::I2C_Display (const int address, const int nb_lines):_file(-1),
		_address(address),
		_displayfunction(LCD_4BITMODE | LCD_5x8DOTS),
		_displaycontrol(LCD_DISPLAYON | LCD_CURSORON | LCD_BLINKON),
		_displaymode(LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT),
		_backlightval(LCD_NOBACKLIGHT),
		_currLine(0),
		_nb_lines(nb_lines),
		_sda (GPIOs::GPIO(GPIO_I2C1_SDA,RESERVED_MODE,"I2C SDA")),
		_scl (GPIOs::GPIO(GPIO_I2C1_SCL,RESERVED_MODE,"I2C SCL"))
{
	if (_nb_lines > 1) _displayfunction |= LCD_2LINE;

	const int adapter_nr = 1;
	char filename[20];

	snprintf (filename, 19, "/dev/i2c-%d", adapter_nr);
	printf("Opening '%s' (addr=%x)...\n",filename, _address);
	_file = wiringPiI2CSetupInterface (filename, _address);
	if (_file < 0) {
		/* ERROR HANDLING; you can check errno to see what went wrong */
		throw std::range_error(std::string("Could not find I2C:")+filename);
	}
	printf("'%s' opened!\n",filename);

	if (ioctl(_file, I2C_SLAVE, _address) < 0) {
		throw std::range_error(std::string("I2C init failed"));
		exit(1);
	}
}

void I2C_Display::begin (void)
{

	delay(50);
	expanderWrite(0);	// reset expander
	delay(1000);

	write4bits(0x03 << 4); // Set to 4 bits
	delayMicroseconds(4500); // wait min 4.1ms
	write4bits(0x03 << 4); // Set to 4 bits
	delayMicroseconds(150); // wait min 100ums
	write4bits(0x03 << 4); // Set to 4 bits

	write4bits(0x02 << 4);

	// set # lines, font size, etc.
	command(LCD_FUNCTIONSET | _displayfunction);

	// turn the display on with no cursor or blinking default
	command(LCD_DISPLAYCONTROL | _displaycontrol);

	// set the entry mode
	command(LCD_ENTRYMODESET | _displaymode);

	// clear it off
	clear();

	// Initialize to default text direction (for roman languages)
	home();
}

/********** high level commands, for the user! */
void I2C_Display::clear(){
	command(LCD_CLEARDISPLAY);// clear display, set cursor position to zero
	delayMicroseconds(1600);  // this command takes a long time!
}

void I2C_Display::home(){
	command(LCD_RETURNHOME);  // set cursor position to zero
	delayMicroseconds(1600);  // this command takes a long time!
}
void I2C_Display::noBacklight()
{
	_backlightval=LCD_NOBACKLIGHT;
	command(0);
}
void I2C_Display::backlight()
{
	_backlightval=LCD_BACKLIGHT;
	command(0);
}

// Turn the display on/off (quickly)
void I2C_Display::noDisplay() {
	_displaycontrol &= ~LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void I2C_Display::display() {
	_displaycontrol |= LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void I2C_Display::noCursor() {
	_displaycontrol &= ~LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void I2C_Display::cursor() {
	_displaycontrol |= LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void I2C_Display::noBlink() {
	_displaycontrol &= ~LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void I2C_Display::blink() {
	_displaycontrol |= LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void I2C_Display::write(char value) {
	send(value, uint8_t(Rs));
}

void I2C_Display::setCursor(uint8_t col, uint8_t row){
	static const int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
	_currLine = row;
	_currLine %= _nb_lines;
	command(LCD_SETDDRAMADDR | (col + row_offsets[_currLine]));
}

void I2C_Display::print(const char* txt)
{
	while (1)
	{
		const char c(*(txt++));
		if (c == 0) return;
		if (c == '\n')
		{
			// Change line
			setCursor (0,_currLine +1);
		}
		else
		{
			send(c, uint8_t(Rs));
		}
	}
}

inline void I2C_Display::command(uint8_t value) {
	send(value, 0);
}

// write either command or data
void I2C_Display::send(uint8_t value, uint8_t mode) {
	const uint8_t realMode(mode| _backlightval);
	uint8_t highnib=value&0xf0;
	uint8_t lownib=(value<<4)&0xf0;
	write4bits((highnib)|realMode);
	write4bits((lownib)|realMode);
}

void I2C_Display::write4bits(uint8_t value) {
	expanderWrite(value);
	pulseEnable(value);
}
inline void I2C_Display::expanderWrite(uint8_t value) {
	//const int res = i2c_smbus_write_byte_data(_file,  0,value);
	const int res = wiringPiI2CWrite(_file, value);
	if (res != 0)
	{
		printf("i2c_smbus_write_byte_data returned %d\n",res);
	}
}
void I2C_Display::pulseEnable(uint8_t _data){
	expanderWrite(uint8_t (_data | En));	// En high
	delayMicroseconds(1);		// enable pulse must be >450ns

	expanderWrite(uint8_t (_data & ~En));	// En low
	delayMicroseconds(50);		// commands need > 37us to settle
}



}
}
