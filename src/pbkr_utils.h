#ifndef _pbkr_utils_h_
#define _pbkr_utils_h_

#include <pthread.h>
#include <stdlib.h>
#include <stdexcept>
#include <exception>

namespace PBKR
{

/*******************************************************************************
 * GLOBAL CONSTANTS
 *******************************************************************************/

#define PI 3.14159265
#define TWO_PI (2.0*PI)
#define FREQUENCY_HZ (44100u)

/*******************************************************************************
 * THREADS
 *******************************************************************************/

class Thread
{
public:
	Thread(void);
	virtual ~Thread(void);
	void start(void);
	void* join(void);
	virtual void body(void) =0;
private:
	static void* real_start(void* param);
	pthread_t _thread;
	pthread_attr_t* _attr;
};

/*******************************************************************************
 * VIRTUAL TIME
 *******************************************************************************/
class VirtualTime
{
public:
	struct Time
	{
		unsigned long samples;
		unsigned long nbSec;
		bool operator< ( Time const&r)const;
		bool operator<= ( Time const&r)const;
		Time operator+  (Time const&r)const;
		Time operator- (Time const&r)const;
	};
	static const Time zero_time;
	static void elapseSample(void);
	static inline Time now(void){return _currTime;}
	static Time inS(float s);
	static inline float toS(const Time& s);
	static unsigned long delta(const Time& t0, const Time& t1);
private:
	static Time _currTime;
};


/*******************************************************************************
 * FADER
 *******************************************************************************/

class Fader
{
public:
	Fader(float dur_s, float initVal, float finalVal);
	float position (void);
	inline bool done (void)const{return _done;}
private:
	VirtualTime::Time _initTime;
	VirtualTime::Time _finalTime;
	float _initVal;
	float _finalVal;
	const float _duration;
	const float _factor;
	bool _done;
};


/*******************************************************************************
 * GENERAL PURPOSE FUNCTIONSS
 *******************************************************************************/
const char  getch(void);
}
#endif // _pbkr_utils_h_
