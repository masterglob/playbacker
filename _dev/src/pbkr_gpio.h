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
	BCM_id getBCM(void)const{return m_bcm;};
	PIN_id getPIN(void)const{return m_pin;};
	static GPIO_id pinToId(const PIN_id pin);
    std::string name(void)const{return m_name;};
private:
	static const BCM_id bcm_map[nb_ios];
	static const PIN_id pin_map[nb_ios];
	static int       io_mode[nb_ios];
protected:
    std::string m_name;
    std::string m_mode;
	GPIO_id m_gpio;
	PIN_id m_pin;
	BCM_id m_bcm;
	static bool m_began;
};


/*******************************************************************************
 * UART PINS
 *******************************************************************************/
static const GPIO_id GPIO_UART0_TX (GPIO::pinToId(8));  // BCM 14, WiringPi 15
static const GPIO_id GPIO_UART0_RX (GPIO::pinToId(10)); // BCM 15, WiringPi 16

/*******************************************************************************
 * SPI PINS
 *******************************************************************************/
static const GPIO_id GPIO_SPI_MOSI (GPIO::pinToId(19)); // BCM 10, WiringPi 12
static const GPIO_id GPIO_SPI_MISO (GPIO::pinToId(21)); // BCM  9, WiringPi 13
static const GPIO_id GPIO_SPI_SCLK (GPIO::pinToId(23)); // BCM 11, WiringPi 14
static const GPIO_id GPIO_SPI_CE0  (GPIO::pinToId(24)); // BCM  8, WiringPi 10
static const GPIO_id GPIO_SPI_CE1  (GPIO::pinToId(26)); // BCM  7, WiringPi 11

/*******************************************************************************
 * I2C PINS
 *******************************************************************************/
#define GPIO_I2C0_SDA  (PBKR::GPIOs::GPIO::pinToId(27)) // BCM  0, WiringPi 30
#define GPIO_I2C0_SCL  (PBKR::GPIOs::GPIO::pinToId(28)) // BCM  1, WiringPi 31
#define GPIO_I2C1_SDA  (PBKR::GPIOs::GPIO::pinToId(3))  // BCM  2, WiringPi 8
#define GPIO_I2C1_SCL  (PBKR::GPIOs::GPIO::pinToId(5))  // BCM  3, WiringPi 9

/*******************************************************************************
 * I2S PINS
 *******************************************************************************/
static const GPIO_id GPIO_I2S_BCK  (PBKR::GPIOs::GPIO::pinToId(12)); // BCM 18, WiringPi 1
static const GPIO_id GPIO_I2S_LRCK (PBKR::GPIOs::GPIO::pinToId(35)); // BCM 19, WiringPi 24
static const GPIO_id GPIO_I2S_DATA (PBKR::GPIOs::GPIO::pinToId(40)); // BCM 21, WiringPi 29

/*******************************************************************************
 * GENERAL PURPOSE PINS
 *******************************************************************************/
static const GPIO_id GPIO_17_PIN11 (GPIO::pinToId(11)); // BCM 17, WiringPi 0
static const GPIO_id GPIO_27_PIN13 (GPIO::pinToId(13)); // BCM 27, WiringPi 2
static const GPIO_id GPIO_22_PIN15 (GPIO::pinToId(15)); // BCM 22, WiringPi 3
static const GPIO_id GPIO_23_PIN16 (GPIO::pinToId(16)); // BCM 23, WiringPi 4
static const GPIO_id GPIO_24_PIN18 (GPIO::pinToId(18)); // BCM 12, WiringPi 1
static const GPIO_id GPIO_05_PIN29 (GPIO::pinToId(29)); // BCM  5, WiringPi 21
static const GPIO_id GPIO_06_PIN31 (GPIO::pinToId(31)); // BCM  6, WiringPi 22
static const GPIO_id GPIO_13_PIN33 (GPIO::pinToId(33)); // BCM 13, WiringPi 23
static const GPIO_id GPIO_19_PIN35 (GPIO::pinToId(35)); // BCM 19, WiringPi 24
static const GPIO_id GPIO_26_PIN37 (GPIO::pinToId(37)); // BCM 26, WiringPi 25


/*******************************************************************************
 *
 *******************************************************************************/
class Output: public GPIO
{
public:
	Output (const unsigned int id, const char*name):GPIO(id, OUTPUT,name){};
	void on(void)const{digitalWrite(m_gpio, HIGH);}
	void off(void)const{digitalWrite(m_gpio, LOW);}
	void set(bool isOn)const {if (isOn) on(); else off();}
};
class Led: public Output
{
public:
	Led (const unsigned int id):Output(id, "LED"){};
};


/*******************************************************************************
 *
 *******************************************************************************/
class Input: protected GPIO
{
public:
    Input (const unsigned int id, const char*name="INPUT"):GPIO(id, INPUT,name){};
    bool pressed(void)const{return digitalRead(m_gpio);}
};

}
}
#endif
