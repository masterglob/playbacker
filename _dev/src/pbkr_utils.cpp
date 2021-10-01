#include "pbkr_utils.h"
#include "pbkr_display_mgr.h"
#include "pbkr_wav.h"
#include "pbkr_osc.h"
#include "pbkr_menu.h"
#include "pbkr_projects.h"

#include <math.h>
#include <stdio.h>

#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <dirent.h>
#include <iostream>
#include <fstream>
#include <sched.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <ctime>

// #define DEBUG_MIDI printf
#define DEBUG_MIDI(...)

// #define DEBUG_THREADS printf
#define DEBUG_THREADS(...)

#define DEBUG_WFS printf
// #define DEBUG_WFS(...)

/*******************************************************************************
 * LOCAL FUNCTIONS
 *******************************************************************************/
namespace
{
using namespace PBKR;
static DISPLAY::DisplayManager& display (DISPLAY::DisplayManager::instance());

#define FAST_FORWARD_BACKWARD_S 20
#define PAUSE_BACKWARD_S 5

} // namespace

/*******************************************************************************
 * WemosFileSender
 *******************************************************************************/
namespace PBKR
{

/*******************************************************************************/
WemosFileSender::WemosFileSender(const string& filename):
        Thread("WemosFileSender"),
        mFilename(filename)
{
    DEBUG_WFS("WemosFileSender::WemosFileSender\n");
    start(false);
}

/*******************************************************************************/
WemosFileSender::~WemosFileSender(void)
{

}

/*******************************************************************************/
void
WemosFileSender::body(void)
{
    DEBUG_WFS("WemosFileSender::body\n");

    uint32_t nbSamplesSent(0);
    const int tmax = time(NULL) + 3; /// MAX ~3 sec

    return; // TODO: remove!
    MidiOutMsg msg;
    msg.push_back(0); // Reserved
    try
    {
        WavFile8Mono file (mFilename);
        DEBUG_WFS("FILE format OK\n");
        while (file.is_open() and not (file.eof() or isExitting()))
        {
            for (int i(0); i < 100; i++)
            {
                const uint8_t b (file.readSample());
                if (file.eof()) break;
                msg.push_back(b/2); // On 7 bits!
                nbSamplesSent++;
            }

            if (time(NULL) > tmax)
            {
                DEBUG_WFS("WemosFileSender: timeout after %d samples!\n", nbSamplesSent);
                break;
            }
        }
        printf ("\n");
        wemosControl.pushSysExMessage(WemosControl::SYSEX_COMMAND_PLAY_SAMPLE, msg);
        printf("Send SYS EX msg with size:%d\n", nbSamplesSent);
        // printf("TODO!!\n"); // TODO
    }
    catch (...)
    {
        printf("Reading %s failed after %d samples!...\n", mFilename.c_str(), nbSamplesSent);
    }


}

} // namespace

/*******************************************************************************
 * EXTERNAL FUNCTIONS
 *******************************************************************************/
namespace PBKR
{
FileManager fileManager;

/*******************************************************************************/
Thread::Thread(const std::string& name):
        m_stop(false), m_done(false),_thread(-1),_attr(NULL),m_name(name)

{
}

/*******************************************************************************/
Thread::~Thread(void)
{
}

/*******************************************************************************/
void Thread::start(bool needJoin)
{
    DEBUG_THREADS("Thread::start\n");
    m_mutex.lock();
	pthread_create (&_thread, _attr,Thread::real_start, (void*)this);
	if (needJoin)
	{
	    ThrInfo info;
	    info.name = m_name;
	    info.pid = _thread;
	    mVect.push_back(info);
	}
    m_mutex.unlock();
    DEBUG_THREADS("[THREAD] start %s\n", info.name.c_str());
}

/*******************************************************************************/
void* Thread::real_start(void* param)
{
    DEBUG_THREADS("Thread::real_start\n");
	Thread* thr((Thread*)param);
	if (thr)
	{
		thr -> body ();
	    thr->m_done = true;
	}
	return NULL;
}

bool Thread::m_isExitting(false);
Thread::ThreadVect Thread::mVect;
std::mutex Thread::m_mutex;
void Thread::doExit(void){m_isExitting = true;}
bool Thread::isExitting(void){return m_isExitting;}
void Thread::join_all(void)
{
    m_mutex.lock();
    for (auto it = mVect.begin() ; it != mVect.end(); it++)
    {
        ThrInfo& info (*it);
        void* returnVal;
        DEBUG_THREADS("[THREAD] joining %s...\n", info.toStr().c_str());
        pthread_join(info.pid, &returnVal);
        DEBUG_THREADS("[OK]\n");
    }
    mVect.clear();
    m_mutex.unlock();
    m_isExitting = true;
}

/*******************************************************************************/
void setLowPriority(void)
{
    sched_param sch;
    sch.__sched_priority = 0;
    if(pthread_setschedparam(pthread_self(), SCHED_OTHER, &sch))
    {
        throw EXCEPTION("Failed to set Thread scheduling : setLowPriority");
    }
}

/*******************************************************************************/
void setRealTimePriority(void)
{
    sched_param sch;
    sch.__sched_priority = 80;
    if(pthread_setschedparam(pthread_self(), SCHED_FIFO, &sch))
    {
        throw EXCEPTION("Failed to set Thread scheduling : setRealTimePriority");
    }
}

/*******************************************************************************
 * SAMPLE_RATE
 *******************************************************************************/
SampleRate::SampleRate(void)
{
    mSupportedRates.push_back(44100u);
    mSupportedRates.push_back(48000u);
    mCurrent = mSupportedRates[0];
}

/*******************************************************************************/
bool SampleRate::supported (const Frequency rate)
{
    FOR(it,mSupportedRates)
    {
        const Frequency r(*it);

        if (rate == r)
        {
            // DEBUG_WFS("Supported sample rate:%lu\n",rate);
            return true;
        }
    }
    DEBUG_WFS("Unsupported sample rate:%lu\n",rate);
    return false;
}
/*******************************************************************************/
bool SampleRate::set(const Frequency rate)
{
    if (supported(rate))
    {
        if (mCurrent != rate)
        {
            // DEBUG_WFS ("Request Sample Rate to %lu\n",rate);
        }
        mCurrent =rate;
        return true;
    }
    return false;
}

// Static fields for SampleRate
SampleRate actualSampleRate;

/*******************************************************************************
 * VIRTUAL TIME
 *******************************************************************************/
const VirtualTime::Time VirtualTime::zero_time ({0,0});
VirtualTime::Time VirtualTime::_currTime({0,0});

/*******************************************************************************/
bool VirtualTime::Time::operator< (Time const&r)const
{
	return nbSec < r.nbSec or
			(nbSec == r.nbSec and samples < r.samples);
}

/*******************************************************************************/
bool VirtualTime::Time::operator<= (Time const&r)const
{
	return nbSec < r.nbSec or
			(nbSec == r.nbSec and samples <= r.samples);
}

/*******************************************************************************/
VirtualTime::Time VirtualTime::Time::operator- (VirtualTime::Time const&r)const
{
	if (*this < r) return VirtualTime::zero_time;
	VirtualTime::Time result (*this);
	result.nbSec -= r.nbSec;
	if (result.samples < r.samples)
	{
		result.samples+=CURRENT_FREQUENCY;
		result.nbSec--;
	}
	result.samples -= r.samples;
	return result;
}


/*******************************************************************************/
VirtualTime::Time VirtualTime::Time::operator+ (VirtualTime::Time const&r)const
{
	VirtualTime::Time result (*this);
	result.nbSec += r.nbSec;
	result.samples += r.samples;
	if (result.samples>=CURRENT_FREQUENCY)
	{
		result.samples-=CURRENT_FREQUENCY;
		result.nbSec++;
	}
	return result;
}

/*******************************************************************************/
void VirtualTime::elapseSample(void)
{
	_currTime.samples ++;
	if (_currTime.samples>=CURRENT_FREQUENCY)
	{
		_currTime.samples-=CURRENT_FREQUENCY;
		_currTime.nbSec++;
	}
}

/*******************************************************************************/
unsigned long VirtualTime::delta(const Time& t0, const Time& t1)
{
	unsigned long result (t1.samples - t0.samples);
	result += CURRENT_FREQUENCY * (t1.nbSec -t0.nbSec);
	return result;
}

/*******************************************************************************/
float VirtualTime::toS(const Time& s)
{
	const float res (s.nbSec + ((float)s.samples) /CURRENT_FREQUENCY);
	return res;
}

/*******************************************************************************/
VirtualTime::Time VirtualTime::inS(float s)
{
	Time result (zero_time);
	if (s >0)
	{
		const unsigned long intsec(s);
		result.nbSec = intsec;
		s -= intsec;

		const unsigned long toSamples (s * CURRENT_FREQUENCY);

		result.samples = toSamples;
		if (result.samples>CURRENT_FREQUENCY)
		{
			result.samples-=CURRENT_FREQUENCY;
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
                m_dur_s(dur_s),
				_initTime(VirtualTime::now()),
				_finalTime(_initTime+ VirtualTime::inS(m_dur_s)),
				_initVal(initVal),
				_finalVal(finalVal),
				_duration(VirtualTime::toS(_finalTime-_initTime)),
				_factor((initVal-finalVal)/_duration),
				_done(false)
{}

/*******************************************************************************/
void Fader::debug(void)
{
    printf("Timer (init= %f, end = %f, now = %f) (%d)\n",
            VirtualTime::toS(_initTime),
            VirtualTime::toS(_finalTime),
            VirtualTime::toS(VirtualTime::now()),update());
}

/*******************************************************************************/
void Fader::restart(void)
{
    _initTime = VirtualTime::now();
    _finalTime = _initTime+ VirtualTime::inS(m_dur_s);
    _done = false;
}

/*******************************************************************************/
bool Fader::update(void)
{
	if (_finalTime < VirtualTime::now())
		_done = true;
	return _done;
}
/*******************************************************************************/
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
 * MIDI Decoder
 *******************************************************************************/
MIDI_Decoder::MIDI_Decoder(void):m_maxLevel(0.0),m_hasBreak(false),m_oof(true),m_skip(100){}

/*******************************************************************************/
int MIDI_Decoder::receive (int16_t val)
{
    static const float MAX_BYTE (256.0);
    if (m_skip)
    {
        m_skip--;
        m_oof = true;
        val = 0;
    }
    if (val < 0)
    {
        if (val > -(MAX_BYTE*2))
        {
            printf("Serial encoding error :insufficient level (%f)!\n",m_maxLevel);
        }
        m_maxLevel = -val;
        m_hasBreak = true;
        m_oof = true;
        DEBUG_MIDI("MIDI break: %d (%f)\n",val, m_maxLevel);
    }
    else if (m_hasBreak)
    {
        m_hasBreak = false;
        m_oof = (val == 0);
        DEBUG_MIDI("MIDI OOF=%d (%d)\n",m_oof, val);
    }

    if (m_oof) return -1;

    const float f ((MAX_BYTE * val) / m_maxLevel);
    const int b (round (f));
    /*
    static int maxLog(500);
    if (maxLog-- > 0)
    {

        const int vAccurate ((b * m_maxLevel) /MAX_BYTE);
        const int vInaccurate (((b+1) * m_maxLevel) /MAX_BYTE);
        DEBUG_MIDI("MIDI B=0x%02X (%d), accurate would be %d, err = %d %%)\n",
                b,val, vAccurate, abs(100 * (val - vAccurate)/(vInaccurate - vAccurate)));

    }
     */
    if (b >= MAX_BYTE)
    {
        printf("Serial encoding error, MAX=(%f), b=%d\n",m_maxLevel,b);
        m_skip = 100;
        m_hasBreak = false;
        m_maxLevel = 0.0;
    }
    return b;
} // MIDI_Decoder::receive


/*******************************************************************************
 * FILE MANAGER
 *******************************************************************************/
FileManager::FileManager (void):
        Thread("FileManager"),
        m_indexPlaying(0),
        m_nbFiles(0),
        _file(NULL),
        _reading(false),
        _starting(false),
        mWemosFileSender(NULL),
        _paused(false),
        _lastL(0.0),
        _lastR(0.0),
        _pProject(NULL),
        m_usbMounted(false)
{
}

/*******************************************************************************/
FileManager::~FileManager (void)
{
    if (_file) delete (_file);
    if (mWemosFileSender) delete mWemosFileSender;
}

/*******************************************************************************/
void FileManager::body(void)
{
    for ( ; not isExitting() ; usleep(100 * 1000))
    {
        ProjectVect projects (getAllProjects());
        if (_pProject)
        {
            // check that project still exists!
            if (_pProject->toClose())
            {
                unloadProject();
            }
        }
        // auto-load first project
        for (auto it(projects.begin()); (it != projects.end()) && (not _pProject); it++)
        {
            Project* project (*it);
            if (not project->toClose())
            {
                loadProject(project);
            }
        }

        // Check USB
        struct stat info;
        const bool usbMounted ( stat( USB_MOUNT_POINT , &info ) == 0);
        if (m_usbMounted != usbMounted)
        {
            using namespace DISPLAY;
            m_usbMounted = usbMounted;
            if (m_usbMounted)
            {
                DisplayManager::instance().warning("USB plugged");
            }
            else
            {
                DisplayManager::instance().warning("USB unplugged");
            }
            if (OSC::p_osc_instance) OSC::p_osc_instance->CheckUSB();
        }

        // update timecode
        const string timecode (_file ? _file->getTimeCode() : " ");
        display.setTimeCode(timecode);
    }
}

/*******************************************************************************/
bool
FileManager::loadProject (Project* proj)
{
    if (!proj) return false;
    if (proj->getNbTracks() <= 0) return false;

    stopReading();
    _pProject = proj;
    _pProject->setInuse(true);
    m_indexPlaying = -1;
    m_nbFiles = proj->getNbTracks();
    m_title = proj->m_title;
    display.onEvent(DISPLAY::DisplayManager::evProjectTrackCount, std::to_string (m_nbFiles));
    display.onEvent(DISPLAY::DisplayManager::evProjectTitle, m_title);

    const TrackVect tracks(_pProject->tracks());
    FOR (it, tracks)
    {
        const Track& track =(*it);
        display.setTrackName(track.m_title, track.m_index - 1);
    }
    printf("Loaded project '%s' (%d tracks)\n",m_title.c_str(), m_nbFiles);
    selectIndex (proj->getFirstTrackIndex());

    display.updateProjectList();
    if (OSC::p_osc_instance) OSC::p_osc_instance->updateProjectList();
    return true;
}  // FileManager::loadProject

/*******************************************************************************/
void
FileManager::unloadProject (void)
{
    if (!_pProject) return;
    stopReading();
    m_indexPlaying = -1;
    m_nbFiles = -1;
    _pProject->setInuse(false);
    _pProject = NULL;
    display.onEvent(DISPLAY::DisplayManager::evProjectTitle, "No project...");
    display.onEvent(DISPLAY::DisplayManager::evProjectTrackCount, std::to_string (0));

}  // FileManager::unloadProject

/*******************************************************************************/
void FileManager::startReading(void)
{
    if (!_file)
    {
        selectIndex (m_indexPlaying);
    }
    if (_file)
    {
        if (_reading)
        {
            pauseReading();
        }
        else
        {
            _file->reset();
            _reading = true;
            _starting = true;
            if (mWemosFileSender) delete mWemosFileSender;
            mWemosFileSender = NULL;
            _paused = false;
            printf("Start reading...\n");
            display.onEvent(DISPLAY::DisplayManager::evPlay);
        }
    }
    else
    {
        printf("No selected file!\n");
    }
}

/*******************************************************************************/
void FileManager::pauseReading(void)
{
    if (_file && _reading)
    {
        if (_paused)
        {
            _paused = false;
            _file->fastForward(false, PAUSE_BACKWARD_S);
            display.onEvent(DISPLAY::DisplayManager::evPlay);
        }
        else
        {
            display.onEvent(DISPLAY::DisplayManager::evPause);
            _paused = true;
        }
    }

} // FileManager::stopReading

/*******************************************************************************/
void FileManager::stopReading(void)
{
    if (_file && _reading)
    {
        _starting = false;
        _reading = false;
        printf("Stop reading\n");
        display.onEvent(DISPLAY::DisplayManager::evStop);
        if (_file) delete (_file);
        _file=NULL;
    }

} // FileManager::stopReading

/*******************************************************************************/
void FileManager::fastForward(void)
{
    if (_file && _reading)
    {
        _file->fastForward(true, FAST_FORWARD_BACKWARD_S);
    }
} // FileManager:: fastForward

/*******************************************************************************/
void FileManager::backward(void)
{
    if (_file && _reading)
    {
        _file->fastForward(false, FAST_FORWARD_BACKWARD_S);
    }
} // FileManager:: backward

/*******************************************************************************/
void FileManager::getSample(float& l, float & r, int& midiB)
{
    midiB = -1;
    if (_starting)
    {
        if (mWemosFileSender)
        {
            // Already started... wait for the end
            if (mWemosFileSender->isDone())
            {
                DEBUG_WFS("mWemosFileSender done\n");
                delete mWemosFileSender;
                mWemosFileSender = NULL;
                _starting = false;
            }
        }
        else
        {
            const string wavTitle(fileWavTitle(indexPlaying()));
            if (wavTitle.length() > 0)
            {
                DEBUG_WFS("mWemosFileSender : %s\n", wavTitle.c_str());
                mWemosFileSender = new WemosFileSender(wavTitle);
            }
            else
            {
                // No Wav title file, just start reading...
                _starting = false;
            }
        }
    }
    else if (_reading && _file && (!_paused))
    {
        int16_t midiIn;
        if (not _file->getNextSample(l ,r, midiIn))
        {
            stopReading();
        }

        midiB = _midiDecoder.receive(midiIn);
        return;
    }

    l = _lastL;
    r = _lastR;
} // FileManager::getSample

/*******************************************************************************/
void FileManager::startup(void)
{
    start();
}

/*******************************************************************************/
std::string FileManager::filename(size_t idx)const
{
    if (_pProject == NULL) return "";
    return _pProject->getByTrackId(idx).m_filename;
}

/*******************************************************************************/
std::string FileManager::fileTitle(size_t idx)const
{
    if (_pProject == NULL) return "";
    return _pProject->getByTrackId(idx).m_title;
}

/*******************************************************************************/
std::string FileManager::fileWavTitle(size_t idx)const
{
    if (_pProject == NULL) return "";
    return _pProject->getByTrackId(idx).m_wavTitle;
}

/*******************************************************************************/
void FileManager::nextTrack(void)
{
    printf("nextTrack %d %d \n",m_nbFiles, m_indexPlaying);
    const int prev (m_indexPlaying);
    while (m_nbFiles > 0 && m_indexPlaying < m_nbFiles )
    {
        m_indexPlaying ++;
        if (selectIndex (m_indexPlaying)) return;
    }
    m_indexPlaying=prev;
}

/*******************************************************************************/
void FileManager::prevTrack(void)
{
    printf("prevTrack %d %d \n",m_nbFiles, m_indexPlaying);
    const int prev (m_indexPlaying);
    while (m_nbFiles > 0 && m_indexPlaying > 1)
    {
        m_indexPlaying --;
        if (selectIndex (m_indexPlaying)) return;
    }
    m_indexPlaying=prev;
}

/*******************************************************************************/
bool FileManager::selectIndex(const size_t i)
{
    if (!_pProject) return false;
    if (i == 0) return false;
    const size_t trackId (i);
    const Track & track(_pProject->getByTrackId(trackId));
    if (track.m_title.length() == 0)
    {
        display.warning(string("No track #") + std::to_string(trackId));
        return false;
    }
    stopReading();
    m_indexPlaying = trackId;
    try
    {
        const string path(_pProject->m_source.pPath + "/" + _pProject->m_title);
        _file = new WavFileLRC (path, track.m_filename);
    }
    catch (...) {
        printf("Open cancelled, file badly formatted\n");

        display.warning(string("BAD FILE FORMAT"));
        _file = NULL;
        return false;
    }
    if (_file->is_open())
    {
        printf("Opened %s\n",_file->mFilename.c_str());
        actualSampleRate.set(_file->frequency());
        refreshMidiVolume();
        const string trackIdx(std::to_string(track.m_index));
        display.onEvent(DISPLAY::DisplayManager::evFile, track.m_title);
        display.onEvent(DISPLAY::DisplayManager::evTrack, trackIdx);
        return true;
    }
    else
    {
        printf("Failed to open <%s>\n",_file->mFilename.c_str());
    }
    return false;

} // FileManager::playIndex

/*******************************************************************************
 * GENERAL PURPOSE FUNCTIONSS
 *******************************************************************************/

/*******************************************************************************/
const char getch(void) {
	char buf = 0;
#if 1
	struct termios old = {0};
	if (tcgetattr(0, &old) < 0)
		perror("tcsetattr()");
	old.c_lflag &= ~ICANON;
	old.c_lflag &= ~ECHO;
	old.c_cc[VMIN] = 1;
	old.c_cc[VTIME] = 0;
	if (tcsetattr(0, TCSANOW, &old) < 0)
		perror("tcsetattr ICANON");
#endif
	if (read(0, &buf, 1) < 0)
		perror ("read()");
#if 1
	old.c_lflag |= ICANON;
	old.c_lflag |= ECHO;
	if (tcsetattr(0, TCSADRAIN, &old) < 0)
		perror ("tcsetattr ~ICANON");
#endif
	return (buf);
}


const char* NET_DEV_WIFI ("wlan0");
const char* NET_DEV_ETH ("eth0");
const char* getIPAddr (const char* device)
{
    static char ipaddr[INET_ADDRSTRLEN];
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;

    strcat(ipaddr, "\?\?\?");

    getifaddrs(&ifAddrStruct);
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
           if (!ifa->ifa_addr) {
               continue;
           }
           if (strcmp (ifa->ifa_name, device) == 0)
           {
               if (ifa->ifa_addr->sa_family == AF_INET)
               { // check it is IP4 is a valid IP4 Address
                   tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
                   inet_ntop(AF_INET, tmpAddrPtr, ipaddr, INET_ADDRSTRLEN);
               }
           }

       }
       if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);

    return ipaddr;
}

const char* getIPNetMask (const char* device)
{
    static char ipaddr[INET_ADDRSTRLEN+1];
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;

    strcat(ipaddr, "\?\?\?");

    getifaddrs(&ifAddrStruct);
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
           if (!ifa->ifa_netmask) {
               continue;
           }
           if (strcmp (ifa->ifa_name, device) == 0)
           {
               if (ifa->ifa_netmask->sa_family == AF_INET)
               { // check it is IP4 is a valid IP4 Address
                   tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_netmask)->sin_addr;
                   inet_ntop(AF_INET, tmpAddrPtr, ipaddr, INET_ADDRSTRLEN);
               }
           }

       }
       if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);

    return ipaddr;
}

/*******************************************************************************
 * SEND MESSAGES TO WEMOS
 *******************************************************************************/
WemosControl::WemosControl(const char* filename):
    m_current(0),
    m_handle (file_open_write (filename)),
    m_keepAlive(2, 0.0, 0.0)
{
    if (m_handle < 0) {
        throw EXCEPTION("Failed to open serial port\n");
    }
}

WemosControl::~WemosControl(void)
{
    close(m_handle);
    if (m_current) delete m_current;
}

void WemosControl::pushMessage(const MidiOutMsg& msg)
{
    m_mutex.lock();
    m_msgs.push_back(msg);
    m_mutex.unlock();
}
void WemosControl::pushSysExMessage(const Sysex_Command cmd, const MidiOutMsg& msg)
{
    MidiOutMsg sysMsg;
    m_mutex.lock();
    sysMsg.push_back(0xF0);
    sysMsg.push_back(0x43);
    sysMsg.push_back(0x4D);
    sysMsg.push_back(0x4D);
    sysMsg.push_back((uint8_t) cmd);
    for (auto it = msg.begin(); it != msg.end(); it++ )
    {
        const uint8_t& byte(*it);
        sysMsg.push_back(byte);
    }
    sysMsg.push_back(0xF7);
    m_msgs.push_back(sysMsg);
    m_mutex.unlock();
}

void WemosControl::sendByte(void)
{
    if (m_current)
    {
        if (m_current->size() > 0)
        {
            const uint8_t byte (m_current->front());
            m_current->pop_front();
            write(m_handle, &byte, 1);
        }
        else
        {
            delete m_current;
            m_current = 0;

            m_keepAlive.restart();
        }
    }
    else
    {
        m_mutex.lock();
        if (not m_msgs.empty())
        {
            m_current = new MidiOutMsg (m_msgs.front());
            m_msgs.pop_front();
        }
        m_mutex.unlock();

        if (m_keepAlive.update())
        {
            m_keepAlive.restart();
            MidiOutMsg msg;
            pushSysExMessage(WemosControl::SYSEX_COMMAND_VOLUME, msg);
        }
    }
}

/*******************************************************************************
 * LATENCY MANAGER
 *******************************************************************************/

Latency<int> midiLatency;
Latency<float> leftLatency;
Latency<float> rightLatency;

WemosControl wemosControl("/dev/ttyAMA0");
} // namespace PBKR
