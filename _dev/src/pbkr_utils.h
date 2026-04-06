#ifndef _pbkr_utils_h_
#define _pbkr_utils_h_

#include <pthread.h>
#include <stdlib.h>
#include <stdexcept>
#include <exception>
#include <string.h>
#include <deque>
#include <vector>
#include <mutex>
#include <atomic>
#include <array>
#include <thread>

#include "pbkr_config.h"
#include "pbkr_types.h"

namespace PBKR
{

class EXCEPTION : public std::exception
{
public:
    EXCEPTION (const std::string& msg): m_msg(msg){}
    const char * what () const throw ()
    {
        return m_msg.c_str();
    }
private:
    std::string m_msg;
};

class WavFileLRC;
class ByteQueue;
/*******************************************************************************
 * GLOBAL CONSTANTS
 *******************************************************************************/

#define PI 3.14159265
#define TWO_PI (2.0*PI)
#define DEFAULT_FREQUENCY_HZ (44100u)

/*******************************************************************************
 * THREADS
 *******************************************************************************/

void setLowPriority(void);
void setRealTimePriority(int prio=80);

class Thread
{
public:
	Thread(const std::string& name);
	virtual ~Thread(void);
	void start(bool needJoin=true);
	virtual void body(void) =0;
    void stop(void){m_stop=true;}
    bool isDone(void){return m_done;}
    static void join_all(void);
    static void doExit(void);
    static bool isExitting(void);
protected:
private:
    struct ThrInfo
    {
        pthread_t pid;
        std::string name;
        const std::string toStr(void)const
        {return std::string("THREAD ") + name+"(" + std::to_string(pid) + ")";}
    };
    typedef std::vector<ThrInfo,std::allocator<ThrInfo>> ThreadVect;
    static ThreadVect mVect;
	static void* real_start(void* param);
    static bool m_isExitting;
    bool m_stop;
    bool m_done;
	static std::mutex m_mutex;
	pthread_t _thread;
	pthread_attr_t* _attr;
	const std::string m_name;
};

/*******************************************************************************
 * SAMPLE_RATE
 *******************************************************************************/
class SampleRate
{
public:
    typedef unsigned long Frequency;
    SampleRate(void);
    /**
     * Set curent sample rate
     * @return true if successfull. false if unknown sample rate
     */
    bool set(const Frequency rate);
    bool supported (const Frequency rate);
    inline Frequency get(void)const{return mCurrent;}
private:
    std::vector<Frequency> mSupportedRates;
    Frequency mCurrent;
};
extern SampleRate actualSampleRate;

#define CURRENT_FREQUENCY (actualSampleRate.get())

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
	static float toS(const Time& s);
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
	bool update(void); // Return true if fader is done
    void restart(void);
    void debug(void);
	inline bool done (void)const{return _done;}
private:
	float m_dur_s;
	VirtualTime::Time _initTime;
	VirtualTime::Time _finalTime;
	float _initVal;
	float _finalVal;
	const float _duration;
	const float _factor;
	bool _done;
};

#if USE_MIDI_AS_TRACK
/*******************************************************************************
 * MIDI Event
 *******************************************************************************/
class MIDI_Event
{
public:
    MIDI_Event(uint16_t event);
    bool isProgChange(void);
    void getProgChange(uint8_t& prg, bool& firstChannel);
    bool isSoundEvent(void);
    void getSoundEvent(uint8_t& sndId);
    bool isCtrlChange(void);
    void getCtrlChange(uint8_t& ctrl, uint8_t& val, bool& firstChannel);
private:
    uint8_t _type;
    uint8_t _val;
};
#endif // USE_MIDI_AS_TRACK

class Project;
typedef vector<Project*,allocator<Project*>> ProjectVect;

namespace MIDI_FILE
{

/*******************************************************************************
 * MidiFile
 *******************************************************************************/
enum class MidiEventType : uint8_t {
    NoteOff         = 0x80,
    NoteOn          = 0x90,
    PolyPressure    = 0xA0,
    ControlChange   = 0xB0,
    ProgramChange   = 0xC0,
    ChannelPressure = 0xD0,
    PitchBend       = 0xE0,
    SysEx           = 0xF0,
    Meta            = 0xFF,
};

// Channel events only — Meta and SysEx are used internally during parsing
// (tempo map) but are not exposed. This keeps MidiEvent a trivial flat struct
// with no heap allocation, safe for vector reallocation on old GCC ABI.
struct MidiEvent {
    uint32_t      tick     = 0;    // absolute position in MIDI ticks
    float         time_sec = 0.0;  // position in seconds (after tempo resolution)
    MidiEventType type     = MidiEventType::NoteOff;
    uint8_t       channel  = 0;    // 0-15
    uint8_t       data1    = 0;    // note / CC number / etc.
    uint8_t       data2    = 0;    // velocity / CC value / etc.
    void pushToQueue(ByteQueue&) const;
};
using MidiEventVect = std::vector<MidiEvent>;
}

/*******************************************************************************
 * FILE MANAGER
 *******************************************************************************/
class FileManager: protected Thread
{
public:
	FileManager (void);
	virtual ~FileManager (void);
    void startup(void);
    bool selectIndex(const size_t i); /* @param i = track index, starting at 1 */
    void nextTrack(void);
    void prevTrack(void);
    void pauseReading(void);
    void stopReading(void);
    void startReading(void);
    void fastForward(void);
    void backward(void);
    /**
     * Automatically retreive L/R and clic1/clic2 + MIDI informations
     * - Stereo track : Clic1/Clic2 & MIDI are set to 0
     * - 4-track files : MIDI is set to 0
     * @return true is sound is playing
     */
    bool getSample( float& l, float & r, float& l2, float & r2, ByteQueue& midiOut);
    bool reading(void)const{return _reading;}
    const std::string title(void)const {return m_title;}
    size_t indexPlaying(void)const {return m_indexPlaying;}
    size_t nbFiles(void)const {return m_nbFiles;}
    std::string filename(size_t idx)const;
    std::string fileTitle(size_t idx)const;
    float fileGetVolumeSamples(size_t idx)const;
    float fileGetVolumeClic(size_t idx)const;
    void fileSetVolumeSamples(size_t idx, float value)const;
    void fileSetVolumeClic(size_t idx, float value)const;
    bool fileAreParamsModified(size_t idx)const;
    void fileSaveParamsModification(size_t idx)const;
    bool loadProject (Project* proj);
    void unloadProject (void);
protected:
	virtual void body(void);
private:
    void preBuffer(void);
    void resynchMidi(float fromTime);
    size_t m_indexPlaying; // 1 = track 1
    size_t m_nbFiles;
    std::string m_title;
    WavFileLRC* _wavFile;
    MIDI_FILE::MidiEventVect _midiEvents{};
    size_t _nextMidiIdx{0}; // NExt unread Midi event index (in _midiEvents)
    bool _reading;
    bool _starting; // When starting, the title may be sent to WeMos
    bool _paused;
    float _lastL;
    float _lastR;
    Project* _pProject;
    ProjectVect m_allProjects;
    bool m_usbMounted;
};

extern FileManager fileManager;

/*******************************************************************************
 * LATENCY MANAGER
 *******************************************************************************/
template<typename T>
class Latency
{
public:
    Latency(void);
    ~Latency(void);
    void setMs(int latency);
    T putSample(T newSample); // return a latency over newSample
private:
    size_t m_pos;
    size_t m_size;
    T* m_buffer;
};
extern Latency<int> midiLatency;
extern Latency<float> leftLatency;
extern Latency<float> rightLatency;
extern Latency<float> leftClicLatency;
extern Latency<float> rightClicLatency;

/*******************************************************************************
 * SEND MESSAGES TO MIDI
 *******************************************************************************/

class ByteQueue {
public:
    static constexpr size_t SIZE = 1024 * 16;

    inline bool push(uint8_t b) {
        size_t next = (write_idx + 1) % SIZE;
        if (next == read_idx.load(std::memory_order_acquire))
            return false; // full

        buffer[write_idx] = b;
        write_idx = next;
        return true;
    }

    inline bool pop(uint8_t& b) {
        size_t r = read_idx.load(std::memory_order_relaxed);
        if (r == write_idx)
            return false; // empty

        b = buffer[r];
        read_idx.store((r + 1) % SIZE, std::memory_order_release);
        return true;
    }

private:
    std::array<uint8_t, SIZE> buffer;
    std::atomic<size_t> read_idx{0};
    size_t write_idx{0}; // single producer
};


/*******************************************************************************
 * SERIAL LINE (MIDI)
 *******************************************************************************/
class MidiOutSerial : public ByteQueue
{
public:
    explicit MidiOutSerial(const char* filename);
    MidiOutSerial() = delete;
    ~MidiOutSerial();

private:
    int m_handle;
    std::thread m_thread;
    std::atomic<bool> m_running{true};

    void midiThread();
};

/*******************************************************************************
 * GENERAL PURPOSE FUNCTIONS
 *******************************************************************************/
const char  getch(void);

#define ZERO(x) memset(&(x),0, sizeof(x))

extern const char* NET_DEV_WIFI;
extern const char* NET_DEV_ETH;
/** Return the IP address*/
const char* getIPAddr (const char* device);
const char* getIPNetMask (const char* device);




/*******************************************************************************
 * TEMPLATE DEFINITIONS
 *******************************************************************************/

/*******************************************************************************/
template <typename T>
Latency<T>::Latency(void):m_pos(0),m_size(0),m_buffer(NULL){}
/*******************************************************************************/
template <typename T>
Latency<T>::~Latency(void)
{
    if (m_buffer) delete m_buffer;
}

/*******************************************************************************/
template <typename T>
void Latency<T>::setMs(int latency)
{
    // how many samples? 1000 ms => 48000u
    if (latency < 0)
    {
        latency = 0;
    }

    const size_t newSize ((480*latency) / 10);
    if (newSize == m_size)
    {
        printf("latency unchanged to %d ms = %d samples\n", latency,m_size);
        return;
    }
    m_pos = 0;
    if (m_size == 0 && m_buffer)
    {
        delete m_buffer;
        m_buffer = NULL;
    }

    if (newSize > m_size)
    {
        if (m_buffer) delete m_buffer;
        m_buffer = (T*) malloc (newSize * sizeof(T));
        if (m_buffer)
            memset (m_buffer, 0, newSize * sizeof(T));
    }
    m_size = newSize;
    printf("Changed latency to %d ms = %d samples\n", latency,m_size);
}

/*******************************************************************************/
template <typename T>
T Latency<T>::putSample(T newSample)
{
    if (!m_buffer) return newSample;

    T& ref (m_buffer[m_pos]);
    const T res (ref);
    ref = newSample;
    m_pos ++;
    if (m_pos >= m_size) m_pos = 0;
    return res;
}


}
#endif // _pbkr_utils_h_
