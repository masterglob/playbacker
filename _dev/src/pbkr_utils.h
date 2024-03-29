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
void setRealTimePriority(void);

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

/*******************************************************************************
 * MIDI Decoder
 *******************************************************************************/

class MIDI_Decoder
{
public:
    MIDI_Decoder(void);
    /* @param f: the MIDI value read from WAV stream
     * @return a byte to send on erial link if >=0.
     */
    int receive (int16_t val);
private:
    float m_maxLevel;
    bool  m_hasBreak;
    bool  m_oof; // out of frame
    int   m_skip;
};

class Project;
typedef vector<Project*,allocator<Project*>> ProjectVect;

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
     * - 3-track files : Clic1/Clic2 are set to 0
     * - 4-track files : MIDI is set to 0
     * @return true is sound is playing
     */
    bool getSample( float& l, float & r, float& l2, float & r2, int& midiB);
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
    size_t m_indexPlaying; // 1 = track 1
    size_t m_nbFiles;
    std::string m_title;
    WavFileLRC* _file;
    bool _reading;
    bool _starting; // When starting, the title may be sent to WeMos
    bool _paused;
    float _lastL;
    float _lastR;
    MIDI_Decoder _midiDecoder;
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
 * SEND MESSAGES TO WEMOS
 *******************************************************************************/
class ByteQueue
{
public:
    inline void push_back(const uint8_t u8)
    {
        m_queue.push_back(u8);
    }
    void push_back(const ByteQueue& queue);
    inline bool empty(void) const
    {
        return m_queue.empty();
    }
    uint8_t pop_front(void);
    inline void clear(void)
    {
        m_queue.clear();
    }
private:
    typedef std::deque<uint8_t, std::allocator<uint8_t>> m_Queue;
    m_Queue m_queue;
};
typedef ByteQueue MidiOutMsg;

class WemosControl
{
public:
    explicit WemosControl(const char* filename);
    virtual ~WemosControl();
    enum Sysex_Command
    {
        SYSEX_COMMAND_KEEP_ALIVE = 0,
        SYSEX_COMMAND_VOLUME = 6,
        SYSEX_COMMAND_PLAY_SAMPLE = 0x7F,
    };
    void pushMessage(const MidiOutMsg& msg);
    void pushSysExMessage(const Sysex_Command cmd, const MidiOutMsg& msg);
    void sendByte(void);
    void check(void);
private:
    ByteQueue m_msgs;
    std::mutex m_mutex;
    int m_handle;
    Fader m_keepAlive;
};
extern WemosControl wemosControl;

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
