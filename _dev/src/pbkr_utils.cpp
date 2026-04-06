#include "pbkr_api.h"
#include "pbkr_utils.h"
#include "pbkr_display_mgr.h"
#include "pbkr_wav.h"
#include "pbkr_osc.h"
#include "pbkr_menu.h"
#include "pbkr_projects.h"

#include <math.h>
#include <stdio.h>
#include <thread>

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
 * EXTERNAL FUNCTIONS
 *******************************************************************************/
namespace PBKR
{
FileManager fileManager;

/*******************************************************************************/
Thread::Thread(const std::string& name):
        m_stop(false), m_done(false),_thread(-1),_attr(nullptr),m_name(name)

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
	return nullptr;
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
void setRealTimePriority(int prio)
{
    sched_param sch;
    sch.__sched_priority = prio;
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
 * MidiEvent
 *******************************************************************************/
void MIDI_FILE::MidiEvent::pushToQueue(ByteQueue& queue) const
{
    // Sanity guard — Meta (0xFF) and SysEx (0xF0) are never in the event list
    const uint8_t t = static_cast<uint8_t>(type) & 0xF0;
    if (t == 0xF0) return;

    // Status byte = event type (high nibble) | channel (low nibble)
    queue.push(static_cast<uint8_t>(type) | (channel & 0x0F));
    queue.push(data1);

    // ProgramChange (0xC0) and ChannelPressure (0xD0) have only 1 data byte
    if (t != 0xC0 && t != 0xD0)
        queue.push(data2);
}

/*******************************************************************************
 * FILE MANAGER
 *******************************************************************************/
FileManager::FileManager (void):
        Thread("FileManager"),
        m_indexPlaying(0),
        m_nbFiles(0),
        _wavFile(nullptr),
        _reading(false),
        _starting(false),
        _paused(false),
        _lastL(0.0),
        _lastR(0.0),
        _pProject(nullptr),
        m_usbMounted(false)
{
}

/*******************************************************************************/
FileManager::~FileManager (void)
{
    if (_wavFile) delete (_wavFile);
}

/*******************************************************************************/
void FileManager::body(void)
{
    for ( ; not isExitting() ; usleep(1000 * 1000))
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
                DisplayManager::instance().info("USB plugged");
            }
            else
            {
                DisplayManager::instance().info("USB unplugged");
            }
            if (OSC::p_osc_instance) OSC::p_osc_instance->CheckUSB();
        }

        // update timecode
        const string timecode (_wavFile ? _wavFile->getTimeCode() : " ");
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

    const TrackVect& tracks(_pProject->tracks());
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
    _pProject = nullptr;
    display.onEvent(DISPLAY::DisplayManager::evProjectTitle, "No project...");
    display.onEvent(DISPLAY::DisplayManager::evProjectTrackCount, std::to_string (0));

}  // FileManager::unloadProject

/*******************************************************************************/
void FileManager::startReading(void)
{
    if (!_wavFile)
    {
        selectIndex (m_indexPlaying);
    }
    if (_wavFile)
    {
        if (_reading)
        {
            pauseReading();
        }
        else
        {
            _wavFile->reset();
            _reading = true;
            _starting = true;
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
    if (_wavFile && _reading)
    {
        if (_paused)
        {
            _paused = false;
            _wavFile->fastForward(false, PAUSE_BACKWARD_S);
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
    if (_wavFile && _reading)
    {
        _starting = false;
        _reading = false;
        printf("Stop reading\n");
        display.onEvent(DISPLAY::DisplayManager::evStop);
        if (_wavFile) delete (_wavFile);
        _wavFile = nullptr;
        _midiEvents.clear();
    }

} // FileManager::stopReading

/*******************************************************************************/
void FileManager::fastForward(void)
{
    if (_wavFile && _reading)
    {
        const double fromTime = _wavFile->getTimePos();
        _wavFile->fastForward(true, FAST_FORWARD_BACKWARD_S);
        resynchMidi(fromTime);
    }
} // FileManager:: fastForward

/*******************************************************************************/
void FileManager::backward(void)
{
    if (_wavFile && _reading)
    {
        const float fromTime = _wavFile->getTimePos();
        _wavFile->fastForward(false, FAST_FORWARD_BACKWARD_S);
        resynchMidi(fromTime);
    }
} // FileManager:: backward

/*******************************************************************************/
void FileManager::resynchMidi(float fromTime)
{
    float now = _wavFile->getTimePos();

    if (now < fromTime)
    {
        // (Backward)
        while (_nextMidiIdx > 0 && _midiEvents[_nextMidiIdx].time_sec > now)
        {
            _nextMidiIdx--;
        }
    }
    else
    {
        // (Forwward)
        while (_nextMidiIdx + 1 < _midiEvents.size() &&
               _midiEvents[_nextMidiIdx].time_sec <= now)
        {
            _nextMidiIdx++;
        }

    }
    DEBUG_MIDI("resynchMidi (t=%f)=>%u\n", fromTime, _nextMidiIdx);
} // FileManager:: resynchMidi

/*******************************************************************************/
bool FileManager::getSample( float& l, float & r, float& l2, float & r2, ByteQueue& midiOut)
{
    if (_starting)
    {
        API::setSamplesVolume(fileGetVolumeSamples(m_indexPlaying));
        API::setClicVolume(fileGetVolumeClic(m_indexPlaying));
        // just start reading... (No more used be could be used to delay while a
        // task is preparing before reading)
        _starting = false;
    }
    else if (_reading && _wavFile && (!_paused))
    {
        float timePos{-1.0f};
        if (not _wavFile->getNextSample(l ,r, l2, r2, timePos))
        {
            stopReading();
        }
        else if (timePos >= 0.0)
        {
            // Look for MIDI events at this time slot
            while (_nextMidiIdx < _midiEvents.size())
            {
                const MIDI_FILE::MidiEvent& evt{_midiEvents[_nextMidiIdx]};
                if (evt.time_sec > timePos) break;
                evt.pushToQueue(midiOut);
                // printf("Evt:N=%d, t=%f nt=%f \n", _nextMidiIdx, timePos,  evt.time_sec);
                _nextMidiIdx++;
            }
        }
        return true;
    }

    l = _lastL;
    r = _lastR;
    l2 = 0;
    r2 = 0;
    return false;
} // FileManager::getSample

/*******************************************************************************/
void FileManager::startup(void)
{
    start();
}

/*******************************************************************************/
std::string FileManager::filename(size_t idx)const
{
    if (_pProject == nullptr) return "";
    return _pProject->getByTrackId(idx).m_filename;
}

/*******************************************************************************/
std::string FileManager::fileTitle(size_t idx)const
{
    if (_pProject == nullptr) return "";
    return _pProject->getByTrackId(idx).m_title;
}

/*******************************************************************************/
float FileManager::fileGetVolumeSamples(size_t idx)const
{
    if (_pProject == nullptr) return -1.0;
    return _pProject->getByTrackId(idx).m_volumeSamples;
}

/*******************************************************************************/
float FileManager::fileGetVolumeClic(size_t idx)const
{
    if (_pProject == nullptr) return -1.0;
    return _pProject->getByTrackId(idx).m_volumeClic;
}

/*******************************************************************************/
void FileManager::fileSetVolumeSamples(size_t idx, float value)const
{
    if (_pProject == nullptr || idx == 0) return;
    _pProject->getByTrackId(idx).setVolumeSamples(value);
    API::setSamplesVolume(value);
}

/*******************************************************************************/
void FileManager::fileSetVolumeClic(size_t idx, float value)const
{
    if (_pProject == nullptr || idx == 0) return;
    _pProject->getByTrackId(idx).setVolumeClic(value);
    API::setClicVolume(value);
}

/*******************************************************************************/
bool FileManager::fileAreParamsModified(size_t idx)const
{
    if (_pProject == nullptr || idx == 0) return false;
    const Track& track(_pProject->getByTrackId(idx));
    return track.isVolumeClicModified() ||
            track.isVolumeSamplesModified();
}

/*******************************************************************************/
void FileManager::fileSaveParamsModification(size_t idx)const
{
    if (_pProject == nullptr || idx == 0) return;
    _pProject->applyModifications(idx);
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
    const string path(_pProject->m_source.pPath + "/" + _pProject->m_title);

    if(_wavFile) delete (_wavFile);
    _midiEvents.clear();

    try
    {
        _wavFile = new WavFileLRC (path, track.m_filename);
    }
    catch (...)
    {
        printf("Open cancelled, file badly formatted\n");

        display.warning(string("BAD FILE FORMAT"));
        _wavFile = nullptr;
        return false;
    }
    if (_wavFile->is_open())
    {
        try
        {
            MidiFile midiFile(path + "/" + track.m_midiFilename);
            _midiEvents = midiFile.buildEventList();
        }
        catch(const std::exception&e)
        {
            printf("Failed to open file %s:%s\n", track.m_midiFilename.c_str(), e.what());
        }

        printf("Opened %s (%u midi events)\n",_wavFile->mFilename.c_str(), _midiEvents.size());
        actualSampleRate.set(_wavFile->frequency());
        refreshMidiVolume();
        globalMenu.refresh();
        const string trackIdx(std::to_string(track.m_index));
        display.onEvent(DISPLAY::DisplayManager::evFile, track.m_title);
        display.onEvent(DISPLAY::DisplayManager::evTrack, trackIdx);
        return true;
    }
    else
    {
        printf("Failed to open <%s>\n",_wavFile->mFilename.c_str());
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
    struct ifaddrs * ifAddrStruct=nullptr;
    struct ifaddrs * ifa=nullptr;
    void * tmpAddrPtr=nullptr;

    strcat(ipaddr, "\?\?\?");

    getifaddrs(&ifAddrStruct);
    for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next) {
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
    struct ifaddrs * ifAddrStruct=nullptr;
    struct ifaddrs * ifa=nullptr;
    void * tmpAddrPtr=nullptr;

    strcat(ipaddr, "\?\?\?");

    getifaddrs(&ifAddrStruct);
    for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next) {
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
 * SERIAL LINE (MIDI)
 *******************************************************************************/
MidiOutSerial::MidiOutSerial(const char* filename):
    m_handle (file_open_write (filename))
{
    m_thread = std::thread(&MidiOutSerial::midiThread, this);
}

MidiOutSerial::~MidiOutSerial()
{
    m_running = false;
    if (m_thread.joinable())
        m_thread.join();
    if (m_handle >= 0)
        close(m_handle);
}

void MidiOutSerial::midiThread()
{
    setRealTimePriority(85);

    static constexpr size_t BUFFER_SIZE = 64;
    uint8_t buffer[BUFFER_SIZE];

    while (m_running) {
        size_t count = 0;

        while (count < BUFFER_SIZE && pop(buffer[count])) {
            count++;
        }

        if (count > 0) {
            write(m_handle, buffer, count);
            // Debug print
            DEBUG_MIDI("MIDI OUT [%zu]: ", count);
            for (size_t i = 0; i < count; ++i) {
                DEBUG_MIDI("%02X ", buffer[i]);
            }
            DEBUG_MIDI("\n");
        } else {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
}

/*******************************************************************************
 * LATENCY MANAGER
 *******************************************************************************/

Latency<int> midiLatency;
Latency<float> leftLatency;
Latency<float> rightLatency;
Latency<float> leftClicLatency;
Latency<float> rightClicLatency;

} // namespace PBKR
