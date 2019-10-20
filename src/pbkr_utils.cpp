#include "pbkr_utils.h"
#include <stdio.h>

#include <unistd.h>
#include <termios.h>

/*******************************************************************************
 * LOCAL FUNCTIONS
 *******************************************************************************/
namespace
{
} // namespace


#define DEBUG (void)

/*******************************************************************************
 * EXTERNAL FUNCTIONS
 *******************************************************************************/
namespace PBKR
{

Thread::Thread(void): _thread(-1),_attr(NULL)
{
}

Thread::~Thread(void)
{
}

void Thread::start()
{
	DEBUG("Thread::start\n");
	pthread_create (&_thread, _attr,Thread::real_start, (void*)this);
}

void* Thread::join()
{
	void* returnVal;
	pthread_join(_thread, &returnVal);
	return returnVal;
}

void* Thread::real_start(void* param)
{
	DEBUG("Thread::real_start\n");
	Thread* thr((Thread*)param);
	if (thr)
	{
		thr -> body ();
	}
	return NULL;
}

/*******************************************************************************
 * VIRTUAL TIME
 *******************************************************************************/
const VirtualTime::Time VirtualTime::zero_time ({0,0});
VirtualTime::Time VirtualTime::_currTime({0,0});

bool VirtualTime::Time::operator< (Time const&r)const
{
	return nbSec < r.nbSec or
			(nbSec == r.nbSec and samples < r.samples);
}

bool VirtualTime::Time::operator<= (Time const&r)const
{
	return nbSec < r.nbSec or
			(nbSec == r.nbSec and samples <= r.samples);
}

VirtualTime::Time VirtualTime::Time::operator- (VirtualTime::Time const&r)const
{
	if (*this < r) return VirtualTime::zero_time;
	VirtualTime::Time result (*this);
	result.nbSec -= r.nbSec;
	if (result.samples < r.samples)
	{
		result.samples+=FREQUENCY_HZ;
		result.nbSec--;
	}
	result.samples -= r.samples;
	return result;
}


VirtualTime::Time VirtualTime::Time::operator+ (VirtualTime::Time const&r)const
{
	VirtualTime::Time result (*this);
	result.nbSec += r.nbSec;
	result.samples += r.samples;
	if (result.samples>=FREQUENCY_HZ)
	{
		result.samples-=FREQUENCY_HZ;
		result.nbSec++;
	}
	return result;
}

void VirtualTime::elapseSample(void)
{
	_currTime.samples ++;
	if (_currTime.samples>=FREQUENCY_HZ)
	{
		_currTime.samples-=FREQUENCY_HZ;
		_currTime.nbSec++;
	}
}

unsigned long VirtualTime::delta(const Time& t0, const Time& t1)
{
	unsigned long result (t1.samples - t0.samples);
	result += FREQUENCY_HZ * (t1.nbSec -t0.nbSec);
	return result;
}

float VirtualTime::toS(const Time& s)
{
	const float res (s.nbSec + ((float)s.samples) /FREQUENCY_HZ);
	return res;
}

VirtualTime::Time VirtualTime::inS(float s)
{
	Time result (zero_time);
	if (s >0)
	{
		const unsigned long intsec(s);
		result.nbSec = intsec;
		s -= intsec;

		const unsigned long toSamples (s * FREQUENCY_HZ);

		result.samples = toSamples;
		if (result.samples>FREQUENCY_HZ)
		{
			result.samples-=FREQUENCY_HZ;
			result.nbSec++;
		}
	}
	// printf("inS(%.04f) =(%lu,%lu)\n",s, result.nbSec, result.samples);
	return result;
}


/*******************************************************************************
 * FADER
 *******************************************************************************/

Fader::Fader(float dur_s, float initVal, float finalVal):
				_initTime(VirtualTime::now()),
				_finalTime(_initTime+ VirtualTime::inS(dur_s)),
				_initVal(initVal),
				_finalVal(finalVal),
				_duration(VirtualTime::toS(_finalTime-_initTime)),
				_factor((initVal-finalVal)/_duration),
				_done(false)
{}

float Fader::position (void)
{
	const VirtualTime::Time now (VirtualTime::now());
	if (now < _initTime)
	{
		printf("VIrtualTime error(%f)\n", VirtualTime::toS(now));
		return _finalVal;
	}
	if (_finalTime < now)
	{
		_done = true;
		return _finalVal;
	}
	const float delta(VirtualTime::toS(_finalTime - now));
	const float result (_finalVal + _factor * delta);
	return result;
}


/*******************************************************************************
 * GENERAL PURPOSE FUNCTIONSS
 *******************************************************************************/

const char getch(void) {
	char buf = 0;
	struct termios old = {0};
	if (tcgetattr(0, &old) < 0)
		perror("tcsetattr()");
	old.c_lflag &= ~ICANON;
	old.c_lflag &= ~ECHO;
	old.c_cc[VMIN] = 1;
	old.c_cc[VTIME] = 0;
	if (tcsetattr(0, TCSANOW, &old) < 0)
		perror("tcsetattr ICANON");
	if (read(0, &buf, 1) < 0)
		perror ("read()");
	old.c_lflag |= ICANON;
	old.c_lflag |= ECHO;
	if (tcsetattr(0, TCSADRAIN, &old) < 0)
		perror ("tcsetattr ~ICANON");
	return (buf);
}

} // namespace PBKR
