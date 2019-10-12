#ifndef _pbkr_gpio_h_
#define _pbkr_gpio_h_

#include <string>
#include <wiringPi.h>
#include <stdexcept>

namespace PBKR
{
namespace GPIOs
{
/*******************************************************************************
 * GPIO definitions
 *******************************************************************************/
static const unsigned int nb_ios(32);

/*******************************************************************************
 *
 *******************************************************************************/
typedef unsigned int GPIO_id;
typedef int PIN_id;
typedef int BCM_id;

#define RESERVED_MODE -2

class GPIO
{
public:
	GPIO(const GPIO_id id, const int mode, const std::string& name);
	static void begin(void);
	BCM_id getBCM(void)const{return _bcm;};
	PIN_id getPIN(void)const{return _pin;};
	static GPIO_id pinToId(const PIN_id pin);
private:
	static const BCM_id bcm_map[nb_ios];
	static const PIN_id pin_map[nb_ios];
	static int       io_mode[nb_ios];
protected:
	std::string _name;
	GPIO_id _gpio;
	PIN_id _pin;
	BCM_id _bcm;
	static bool _began;
};


/*******************************************************************************
 * UART PINS
 *******************************************************************************/
static const GPIO_id GPIO_UART0_TX (GPIO::pinToId(8));
static const GPIO_id GPIO_UART0_RX (GPIO::pinToId(10));

/*******************************************************************************
 * SPI PINS
 *******************************************************************************/
static const GPIO_id GPIO_SPI_MOSI (GPIO::pinToId(19));
static const GPIO_id GPIO_SPI_MISO (GPIO::pinToId(21));
static const GPIO_id GPIO_SPI_SCLK (GPIO::pinToId(23));
static const GPIO_id GPIO_SPI_CE0  (GPIO::pinToId(24));
static const GPIO_id GPIO_SPI_CE1  (GPIO::pinToId(26));

/*******************************************************************************
 * I2C PINS
 *******************************************************************************/
#define GPIO_I2C0_SDA  (PBKR::GPIOs::GPIO::pinToId(27))
#define GPIO_I2C0_SCL  (PBKR::GPIOs::GPIO::pinToId(28))
#define GPIO_I2C1_SDA  (PBKR::GPIOs::GPIO::pinToId(3))
#define GPIO_I2C1_SCL  (PBKR::GPIOs::GPIO::pinToId(5))

/*******************************************************************************
 * I2S PINS
 *******************************************************************************/
static const GPIO_id GPIO_I2S_BCK  ( 1u);
static const GPIO_id GPIO_I2S_LRCK (24u);
static const GPIO_id GPIO_I2S_DATA (29u);

/*******************************************************************************
 * GENERAL PURPOSE PINS
 *******************************************************************************/
static const GPIO_id GPIO_17_PIN11 (GPIO::pinToId(11));
static const GPIO_id GPIO_27_PIN13 (GPIO::pinToId(13));
static const GPIO_id GPIO_22_PIN15 (GPIO::pinToId(15));


/*******************************************************************************
 *
 *******************************************************************************/
class Led: protected GPIO
{
public:
	Led (const unsigned int id):GPIO(id, OUTPUT,"LED"){};
	void on(void)const{digitalWrite(_gpio, HIGH);}
	void off(void)const{digitalWrite(_gpio, LOW);}
	void set(bool isOn)const {if (isOn) on(); else off();}
};


/*******************************************************************************
 *
 *******************************************************************************/
class Button: protected GPIO
{
public:
	Button (const unsigned int id):GPIO(id, INPUT,"BUTTON"){};
	bool pressed(void)const{return digitalRead(_gpio);}
};
}
}
#endif
