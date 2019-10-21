#include "pbkr_utils.h"
#include "pbkr_display.h"
#include "pbkr_wav.h"
#include <stdio.h>

#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <dirent.h>
#include <iostream>
#include <fstream>

/*******************************************************************************
 * LOCAL FUNCTIONS
 *******************************************************************************/
namespace
{
static const char* PROJECT_TITLE ("pbkr.title");


} // namespace


#define PRELOAD_RAM 0
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

bool Fader::update(void)
{
	if (_finalTime < VirtualTime::now())
		_done = true;
	return _done;
}
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
 * FILE MANAGER
 *******************************************************************************/
FileManager::FileManager (const char* path):
        Thread(),
        _path(path),
        _indexPlaying(-1),
        _nbFiles(0),
        _file(NULL),
        _reading(false),
        _lastL(0.0),
        _lastR(0.0)
{
    start();
}
FileManager::~FileManager (void)
{
    if (_file) delete (_file);
}

void FileManager::body(void)
{
    printf("FileManager::body(%s)!\n",_path.c_str());
    DISPLAY::display.noBlink();
    DISPLAY::display.noCursor();
    on_disconnect();

    while (1)
    {
        do
        {
            usleep(10 * 1000);
        } while (not is_connected());
        on_connect();
        do
        {
            usleep(10 * 1000);
        } while (is_connected());
        on_disconnect();
    }
}

bool FileManager::is_connected(void)
{
    DIR *dir;
    if ((dir = opendir (_path.c_str())) != NULL)
    {
        closedir (dir);
        return true;
    }
    return false;
}


void FileManager::on_disconnect(void)
{
    DISPLAY::display.clear();
    DISPLAY::display.print("No USB key...\n");
    printf("USB disconnected!\n");
    _nbFiles = 0;
    _title = "NO USB KEY";
}

void FileManager::on_connect(void)
{
    DISPLAY::display.clear();
    DISPLAY::display.print("USB connected!\n");
    printf("USB connected!\n");

    // search for compatible files
    DIR *dir;
    struct dirent *ent;
    _nbFiles = 0;
    if ((dir = opendir (_path.c_str())) != NULL)
    {
        // using  std::regex is reaaly too long for compile time!

        while ((ent = readdir (dir)) != NULL)
        {
            const size_t l(strlen(ent->d_name));
            if (l < 4) continue;
            const char* end(&ent->d_name[l-4]);
            if (strcmp (end,".WAV") == 0 ||
                    strcmp (end,".wav") == 0)
            {
                printf ("%s\n", ent->d_name);
                _files[_nbFiles] =ent->d_name;
                _nbFiles++;
            }
        }
        closedir (dir);
    }
    char txt[40];
    snprintf(txt,sizeof(txt),"%d files",_nbFiles);
    DISPLAY::display.print(txt);

    // search for TITLE
    _title = "No Title";
    try
    {
        std::ifstream f;
        f.open((_path + PROJECT_TITLE).c_str());
        if (f.is_open())
        {
            getline (f,_title);
        }
    }
    catch (...) {
    }
    sleep(1);

    DISPLAY::display.clear();
    DISPLAY::display.print(_title.c_str());
    DISPLAY::display.line2();
    DISPLAY::display.print(txt);

} // FileManager::on_connect


void FileManager::startReading(void)
{
    if (_file)
    {
        _file->reset();
        _reading = true;
        printf("Start reading...\n");
    }
    else
    {
        printf("No selected file!\n");
    }
}

void FileManager::stopReading(void)
{
    printf("Stop reading\n");
    _reading = false;

} // FileManager::stopReading

void FileManager::getSample(float& l, float & r, float& l2, float &r2)
{
    if (_reading && _file)
    {
        uint16_t midi(0);
        if (not _file->getNextSample(l ,r, midi))
        {
            stopReading();
        }

        // TODO fill l2 & r2 with MIDI
        l2 = 0.0;
        r2 = 0.0;

        return;
    }

    l = _lastL;
    r = _lastR;
    l2 = 0.0;
    r2 = 0.0;
}

void FileManager::selectIndex(const size_t i)
{
    if (i > _nbFiles) return;
    DISPLAY::display.clear();
    DISPLAY::display.print(_title.c_str());
    DISPLAY::display.line2();
    stopReading();
    if (i == 0)
    {
        char txt[40];
        snprintf(txt,sizeof(txt),"%d files",_nbFiles);
        DISPLAY::display.print(txt);
        stopReading();
        return;
    }
    _indexPlaying = i-1;
    if (_file) delete (_file);
    try
    {
        _file = new WavFileLRC ( std::string (_path) , _files[_indexPlaying]);
    }
    catch (...) {
        printf("Open cancelled, file badly formatted\n");
        _file = NULL;
        return;
    }
    if (_file->is_open())
    {
        printf("Opened %s\n",_file->_filename.c_str());
        DISPLAY::display.print(_files[_indexPlaying].c_str());
    }
    else
    {
        printf("Failed to open <%s>\n",_files[_indexPlaying].c_str());
    }

} // FileManager::playIndex

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
