#include "pbkr_utils.h"
#include "pbkr_display.h"
#include "pbkr_wav.h"
#include "pbkr_osc.h"

#include <stdio.h>

#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <dirent.h>
#include <iostream>
#include <fstream>
#include <sched.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include <sys/types.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>

#define DEBUG_MIDI 0

/*******************************************************************************
 * LOCAL FUNCTIONS
 *******************************************************************************/
namespace
{
using namespace PBKR;
static DISPLAY::DisplayManager& display (DISPLAY::DisplayManager::instance());
static bool strEqual (const xmlChar* a, const char* b)
{
    return strcmp((const char*)a, b) == 0;
}

static std::string getStrAttr(xmlNode * n,const char* attrName)
{
    xmlAttr* attribute = n->properties;
    while(attribute && attribute->name && attribute->children)
    {
        xmlChar* value = xmlNodeListGetString(n->doc, attribute->children, 1);
        if (::strEqual (attribute->name,attrName))
        {
            std::string res(std::string((const char*)value));
            xmlFree(value);
            return res;
        }
        xmlFree(value);
        attribute = attribute->next;
    }
    throw std::range_error("Attribute not found");
}

} // namespace


#define PRELOAD_RAM 0
#define DEBUG (void)

/*******************************************************************************
 * EXTERNAL FUNCTIONS
 *******************************************************************************/
namespace PBKR
{
FileManager fileManager (MOUNT_POINT);

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

void setLowPriority(void)
{
    sched_param sch;
    sch.__sched_priority = 0;
    if(pthread_setschedparam(pthread_self(), SCHED_OTHER, &sch))
    {
        throw EXCEPTION("Failed to set Thread scheduling : setLowPriority");
    }
}

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
 * CONFIGURATION (XML)
 *******************************************************************************/

XMLConfig::XMLConfig(const char* filename)
{
    trackVect.clear();
    LIBXML_TEST_VERSION

    xmlDocPtr doc; /* the resulting document tree */
    xmlNode *root_element = NULL;

    doc = xmlReadFile( filename, NULL, 0);
    if (doc == NULL) {
        fprintf(stderr, "Failed to parse document\n");
        return;
    }

    root_element = xmlDocGetRootElement(doc);

    // Look for "<PBKR>"

    xmlNode *pbkrNode = NULL;

    for (pbkrNode = root_element; pbkrNode; pbkrNode = pbkrNode->next) {
        if (pbkrNode->type == XML_ELEMENT_NODE && strEqual (pbkrNode->name,"PBKR") )
            break;
    }
    if (pbkrNode)
    {
        m_title = ::getStrAttr(pbkrNode, "title");

        xmlNode *trackNode = NULL;
        for (trackNode = pbkrNode->children; trackNode; trackNode = trackNode->next) {
            if (trackNode->type == XML_ELEMENT_NODE && strEqual (trackNode->name,"Track") )
            {
                // process node!
                TrackNode t (trackNode);
                trackVect.push_back(t);
                printf("Track %d (%s :%s)\n",t.id,t.title.c_str(),t.filename.c_str());
                display.setTrackName(t.filename,t.id - 1);
            }
        }
    }
    else
    {
        printf("No element PBKR found...\n");
    }


    printf("%s open successfull\n",filename);
    xmlFreeDoc(doc);
    xmlCleanupParser();
}
XMLConfig::TrackNode::TrackNode ( xmlNode *trackNode):
        _n(trackNode),
        id(getIntAttr("id")),
        title(::getStrAttr(_n,"title")),
        filename(::getStrAttr(_n,"file"))
{

} //XMLConfig::TrackNode::TrackNode

XMLConfig::TrackNode::~TrackNode (void)
{
}

const int XMLConfig::TrackNode::getIntAttr(const char* attrName)
{
    xmlAttr* attribute = _n->properties;
    while(attribute && attribute->name && attribute->children)
    {
        xmlChar* value = xmlNodeListGetString(_n->doc, attribute->children, 1);
        if (strEqual (attribute->name,attrName))
        {
            const std::string res (strdup((const char*)value));
            try
            {
                int i = std::stoi(res);
                xmlFree(value);
                return i;
            }
            catch (...)
            {
                printf("Invalid INT value:%s\n",res.c_str());
                return 0;
            }
        }
        xmlFree(value);
        attribute = attribute->next;
    }
    return 0;
} // XMLConfig::TrackNode::getIntAttr

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
 * MIDI Decoder
 *******************************************************************************/
MIDI_Decoder::MIDI_Decoder(void):m_maxLevel(0.0),m_hasBreak(false),m_oof(true),m_skip(100){}

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
#if DEBUG_MIDI
        DEBUG_MIDI("MIDI break: %d (%f)\n",val, m_maxLevel);
#endif
    }
    else if (m_hasBreak)
    {
        m_hasBreak = false;
        m_oof = (val == 0);
#if DEBUG_MIDI
        DEBUG_MIDI("MIDI OOF=%d (%d)\n",m_oof, val);
#endif
    }

    if (m_oof) return -1;

    const int b ((MAX_BYTE * val) / m_maxLevel);
#if DEBUG_MIDI
    static int maxLog(500);
    if (maxLog-- > 0)
    {
        const int dNext((b+1) * m_maxLevel /MAX_BYTE);
        const int dCurr((b) * m_maxLevel /MAX_BYTE);
        DEBUG_MIDI("MIDI B=0x%02X (%d), err= %d%%)\n",
                b,val, (b-dCurr) / dNext);
    }
#endif
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
FileManager::FileManager (const char* path):
        Thread(),
        _path(path),
        m_indexPlaying(0),
        m_nbFiles(0),
        _file(NULL),
        _reading(false),
        _lastL(0.0),
        _lastR(0.0),
        _pConfig(NULL)
{
}

FileManager::~FileManager (void)
{
    if (_file) delete (_file);
    if (_pConfig) delete _pConfig;
}

void FileManager::body(void)
{
    printf("FileManager::body(%s)!\n",_path.c_str());
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

/*******************************************************************************/
void FileManager::on_disconnect(void)
{
    display.onEvent(DISPLAY::DisplayManager::evUsbOut);
    printf("USB disconnected!\n");
    m_nbFiles = 0;
    _title = "NO USB KEY";

    if (_pConfig) delete _pConfig;
    _pConfig = NULL;

    std::string path (MOUNT_POINT);
    path += "pbkr.xml";
    _pConfig = new XMLConfig (path.c_str());
}

/*******************************************************************************/
void FileManager::on_connect(void)
{
    display.onEvent(DISPLAY::DisplayManager::evUsbIn);

    printf("USB connected!\n");

    // search for compatible files
    m_nbFiles = 0;
    m_indexPlaying = 0;
    /*
    DIR *dir;
    struct dirent *ent;
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
                m_files[m_nbFiles] =ent->d_name;
                m_nbFiles++;
            }
        }
        closedir (dir);
    }*/

    // search for TITLE
    if (_pConfig) delete _pConfig;

    std::string path (MOUNT_POINT);
    path += "pbkr.xml";

    _title= "Untitled project";
    try
    {
        _pConfig = new XMLConfig (path.c_str());
        _title = _pConfig->getTitle();
        m_nbFiles = 0;
        for (size_t i(0); i < sizeof(m_files)/sizeof(*m_files); ++i)
        {
            m_files[i] = "";
        }
        for (auto it (_pConfig->trackVect.begin()); it != _pConfig->trackVect.end(); it++)
        {
            const XMLConfig::TrackNode& tn(*it);
            if (tn.id == 0) continue;
            if (tn.id >= m_nbFiles) m_nbFiles = tn.id;
            m_files[tn.id - 1] = tn.filename;
        }
    }
    catch (...) {
        _pConfig = NULL;
    }

    //sleep(1);
    display.onEvent(DISPLAY::DisplayManager::evProjectTrackCount, std::to_string (m_nbFiles));
    display.onEvent(DISPLAY::DisplayManager::evProjectTitle, _title);
    selectIndex(0);

} // FileManager::on_connect


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
            _reading = false;
            printf("Stop reading\n");
            display.onEvent(DISPLAY::DisplayManager::evStop);
            if (_file) delete (_file);
            _file=NULL;

        }
        else
        {
            _file->reset();
            _reading = true;
            printf("Start reading...\n");
            display.onEvent(DISPLAY::DisplayManager::evPlay);
        }
    }
    else
    {
        printf("No selected file!\n");
    }
}

void FileManager::stopReading(void)
{
    if (_file && _reading) startReading();
} // FileManager::stopReading

void FileManager::getSample(float& l, float & r, int& midiB)
{
    midiB = -1;
    if (_reading && _file)
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

void FileManager::startup(void)
{
    start();
}

std::string FileManager::filename(size_t idx)const
{
    if (idx > sizeof(m_files)/sizeof(*m_files)) return "";
    return m_files[idx];
}

std::string FileManager::fileTitle(size_t idx)const
{
    if (idx > sizeof(m_files)/sizeof(*m_files)) return "";
    if (_pConfig == NULL) return "";
    const XMLConfig::TrackNode & tn (_pConfig->trackVect[idx]);
    return tn.title;
}

void FileManager::nextTrack(void)
{
    printf("nextTrack %d %d \n",m_nbFiles, m_indexPlaying);
    const int prev (m_indexPlaying);
    while (m_nbFiles > 0 && m_indexPlaying + 1 < m_nbFiles )
    {
        m_indexPlaying ++;
        if (selectIndex (m_indexPlaying)) return;
    }
    m_indexPlaying=prev;
}

void FileManager::prevTrack(void)
{
    printf("prevTrack %d %d \n",m_nbFiles, m_indexPlaying);
    const int prev (m_indexPlaying);
    while (m_nbFiles > 0 && m_indexPlaying > 0)
    {
        m_indexPlaying --;
        if (selectIndex (m_indexPlaying)) return;
    }
    m_indexPlaying=prev;
}

bool FileManager::selectIndex(const size_t i)
{
    if (i >= m_nbFiles || m_files[i] == "")
    {
        display.warning(std::string("No track #") + std::to_string(i+1));
        return false;
    }
    stopReading();
    m_indexPlaying = i;
    try
    {
        _file = new WavFileLRC ( std::string (_path) , m_files[m_indexPlaying]);
    }
    catch (...) {
        printf("Open cancelled, file badly formatted\n");

        display.warning(std::string("BAD FILE FORMAT"));
        _file = NULL;
        return false;
    }
    if (_file->is_open())
    {
        printf("Opened %s\n",_file->_filename.c_str());

        std::string title(m_files[m_indexPlaying]);
        std::string trackIdx(std::to_string(m_indexPlaying+1));
        if (_pConfig && m_indexPlaying < _pConfig->trackVect.size())
        {
            const XMLConfig::TrackNode & tn (_pConfig->trackVect[m_indexPlaying]);
            title = tn.title;
        }
        display.onEvent(DISPLAY::DisplayManager::evFile, title);
        display.onEvent(DISPLAY::DisplayManager::evTrack, trackIdx);
        return true;
    }
    else
    {
        printf("Failed to open <%s>\n",m_files[m_indexPlaying].c_str());
    }
    return false;

} // FileManager::playIndex

/*******************************************************************************
 * GENERAL PURPOSE FUNCTIONSS
 *******************************************************************************/

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
           if (strcmp (ifa->ifa_name, device) != 0)
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
    static char ipaddr[INET_ADDRSTRLEN];
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;

    strcat(ipaddr, "\?\?\?");

    getifaddrs(&ifAddrStruct);
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
           if (!ifa->ifa_netmask) {
               continue;
           }
           if (strcmp (ifa->ifa_name, device) != 0)
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
    m_handle (open (filename, O_WRONLY))
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
        }
    }
    else
    {
        if (not m_msgs.empty())
        {
            m_mutex.lock();
            m_current = new MidiOutMsg (m_msgs.front());
            m_msgs.pop_front();
            m_mutex.unlock();
        }
    }
}

WemosControl wemosControl("/dev/ttyAMA0");
} // namespace PBKR
