#ifndef _pbkr_gpio_h_
#define _pbkr_gpio_h_

#include <string>
#include <wiringPi.h>
#include <stdexcept>

#include "pbkr_utils.h"

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
static const GPIO_id GPIO_23_PIN16 (GPIO::pinToId(16));
static const GPIO_id GPIO_24_PIN18 (GPIO::pinToId(18));


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

/*******************************************************************************
 *
 *******************************************************************************/
class Button: protected Input
{
public:
    Button (const unsigned int id, const float & maxWait, const char*name="BUTTON"):
        Input(id, name),m_pressed(false),
        m_must_release(false),
        m_t0 (VirtualTime::now()),
        m_maxWait(maxWait){};
    ~Button(void){}
    bool   pressed(float& duration)const;
private :
    mutable bool m_pressed;
    mutable bool m_must_release;
    mutable VirtualTime::Time m_t0;
    const float m_maxWait;
};
}
}
#endif
