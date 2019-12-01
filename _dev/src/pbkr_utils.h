#ifndef _pbkr_utils_h_
#define _pbkr_utils_h_

#include <pthread.h>
#include <stdlib.h>
#include <stdexcept>
#include <exception>
#include <string.h>
#include <vector>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include "pbkr_config.h"

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
#define FREQUENCY_HZ (44100u)

/*******************************************************************************
 * THREADS
 *******************************************************************************/

void setLowPriority(void);
void setRealTimePriority(void);

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
 * CONFIGURATION (XML)
 *******************************************************************************/
class XMLConfig
{
public:
    XMLConfig(const char* filename);

    class TrackNode
    {
    public:
        TrackNode ( xmlNode *trackNode);
        ~TrackNode (void);
    private:
        xmlNode *_n;
    public:
        const size_t id;
        const std::string title;
        const std::string filename;
    private:
        const int   getIntAttr(const char* attrName);
    };
    typedef std::vector<TrackNode,std::allocator<TrackNode>> TrackVect;
    TrackVect trackVect;
    const std::string& getTitle(void)const{return m_title;}
private:
    std::string m_title;


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

/*******************************************************************************
 * FILE MANAGER
 *******************************************************************************/
class FileManager: protected Thread
{
public:
	FileManager (const char* path);
	virtual ~FileManager (void);
    void startup(void);
    bool selectIndex(const size_t i); /* @param i = track index, starting at 0 */
    void nextTrack(void);
    void prevTrack(void);
    void stopReading(void);
    void startReading(void);
    void getSample( float& l, float & r, int& midiB);
    bool reading(void)const{return _reading;}
    const std::string title(void)const {return _title;}
    size_t indexPlaying(void)const {return m_indexPlaying;}
    size_t nbFiles(void)const {return m_nbFiles;}
    std::string filename(size_t idx)const;
    std::string fileTitle(size_t idx)const;
protected:
	virtual void body(void);
private:
    bool is_connected(void);
    void on_connect(void);
    void on_disconnect(void);
    void preBuffer(void);
    std::string _path;
    std::string m_files[256];
    size_t m_indexPlaying; // 0 = track 1
    size_t m_nbFiles;
    std::string _title;
    WavFileLRC* _file;
    bool _reading;
    float _lastL;
    float _lastR;
    MIDI_Decoder _midiDecoder;
    XMLConfig* _pConfig;
};

extern FileManager fileManager;

/*******************************************************************************
 * GENERAL PURPOSE FUNCTIONSS
 *******************************************************************************/
const char  getch(void);

#define ZERO(x) memset(&(x),0, sizeof(x))

}
#endif // _pbkr_utils_h_
