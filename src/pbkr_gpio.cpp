#include "pbkr_gpio.h"

namespace
{
const char* mode_to_string(const int mode)
{
	switch (mode) {
	case INPUT: return "INPUT";
	case OUTPUT: return "OUTPUT";
	case GPIO_CLOCK: return "GPIO_CLOCK";
	case PWM_OUTPUT: return "PWM_OUTPUT";
	case SOFT_PWM_OUTPUT: return "SOFT_PWM_OUTPUT";
	case SOFT_TONE_OUTPUT: return "SOFT_TONE_OUTPUT";
	case PWM_TONE_OUTPUT: return "PWM_TONE_OUTPUT";
		default:return "<UNKNOWN>";
	}
}
}
namespace PBKR
{

namespace GPIOs
{
const BCM_id GPIO::bcm_map[nb_ios]=
{17, 18, 27, 22, 23, 24, 25, 4, // 0-7
		2,  3,  8,  7,  10, 9,  11, 14, // 8-15
		15, -1, -1, -1, -1, 5,  6,  13, // 16-23
		19, 26, 12, 16, 20, 21, 0,  1}; //24 - 31
const PIN_id GPIO::pin_map[nb_ios]=
{11, 12, 13, 15, 16, 18, 22, 7, // 0-7
		3,  5,  24, 26, 19, 21, 23, 8, // 8-15
		10, -1, -1, -1, -1, 29, 31, 33, // 16-23
		35, 37, 32, 36, 38, 40, 27, 28}; //24 - 31
int      GPIO::io_mode[nb_ios]=
{-1, -1, -1, -1, -1, -1, -1, -1, // 0-7
		-1, -1, -1, -1, -1, -1, -1, -1, // 8-15
		-1, -1, -1, -1, -1, -1, -1, -1, // 16-23
		-1, -1, -1, -1, -1, -1, -1, -1}; //24 - 31
GPIO::GPIO(const GPIO_id id, const int mode, const std::string& name):
		_name(name),
		_gpio(id)
{
	if (_gpio >= nb_ios)
	{
		throw std::range_error(std::string("Bad GPIO:")+std::to_string(_gpio));
	}
	_pin = pin_map[_gpio];
	if (_pin < 0)
	{
		throw std::range_error(std::string("GPIO ")+std::to_string(_gpio)+" is not mapped on J8");
	}
	_bcm = bcm_map[_gpio];
	if (_bcm < 0)
	{
		throw std::range_error(std::string("GPIO ")+std::to_string(_gpio)+" is not mapped on J8");
	}
	if (io_mode[_gpio] == -1)
	{
		io_mode[_gpio] = mode;
		pinMode(_gpio, mode);
	}
	else
	{
		throw std::range_error(std::string("GPIO ")+std::to_string(_gpio)+" is already assigned");
	}
	printf("GPIO %d(pin %d) is set to mode %s for %s\n",
			_gpio,_pin,
			mode_to_string (mode), _name.c_str());
}
GPIO_id GPIO::pinToId(const PIN_id pin)
{
	for (GPIO_id i(0) ; i< nb_ios ; ++i)
	{
		if (pin_map[i] == pin) return i;
	}
	throw std::range_error(std::string("GPIO:no GPIO on pin ")+std::to_string(pin));
}


}
}

