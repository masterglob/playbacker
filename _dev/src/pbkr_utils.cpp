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

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

/*******************************************************************************
 * LOCAL FUNCTIONS
 *******************************************************************************/
namespace
{

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
 * CONFIGURATION (XML)
 *******************************************************************************/

XMLConfig::XMLConfig(const char* filename)
{
    _trackVect.clear();
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
        _title = ::getStrAttr(pbkrNode, "title");

        xmlNode *trackNode = NULL;
        for (trackNode = pbkrNode->children; trackNode; trackNode = trackNode->next) {
            if (trackNode->type == XML_ELEMENT_NODE && strEqual (trackNode->name,"Track") )
            {
                // process node!
                TrackNode t (trackNode);
                _trackVect.push_back(t);
                printf("Track %d (%s :%s)\n",t.id,t.title.c_str(),t.filename.c_str());
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

#if USE_MIDI_AS_TRACK
/*******************************************************************************
 * MIDI Event
 *******************************************************************************/
MIDI_Event::MIDI_Event(uint16_t event):
        _type(event >> 8),
        _val(event & 0xFF)
{}

bool MIDI_Event::isProgChange(void)
{
    return _type == 0x00;
} // MIDI_Event::isProgChange

void MIDI_Event::getProgChange(uint8_t& prg, bool& firstChannel)
{
    firstChannel = !(_val & 0x80);
    prg = _val & 0x7F;
} // MIDI_Event::getProgChange

bool MIDI_Event::isSoundEvent(void)
{
    return _type == 0xFF;
} // MIDI_Event::isSoundEvent

void MIDI_Event::getSoundEvent(uint8_t& sndId)
{
    sndId = _val;
}
 // MIDI_Event::getSoundEvent

bool MIDI_Event::isCtrlChange(void)
{
    return (_type > 0) && (_type < 0xFF);
} // MIDI_Event::isCtrlChange

void MIDI_Event::getCtrlChange(uint8_t& ctrl, uint8_t& val, bool& firstChannel)
{
    firstChannel = !(_type & 0x80);
    ctrl = _type & 0x7F;
    val = _val ;
} // MIDI_Event::getCtrlChange

/*******************************************************************************
 * MIDI Decoder
 *******************************************************************************/
/*
    COMMANDS:
    All commands are 2 bytes-encoded in WAV file:
    - "0" level => No command in progress
    - ">  0.01" level => bit "0"
    - "< -0.01" level => bit "1"
    - at least 1 "0 level" space between 2 commands
    - For readability in WAV file, two consecutive bits in WAV have a slightly different value (*0.8)
    - Sent order: byte 0 first, MSB bit first.

    [0b00000000 0bYXXXXXXX] => Program Change #XXXXXXX (0-127) on channel Y (0 or 1 only)
    [0b11111111 0xXX]       => Clic Event
                               XX = sound Id:
                                    - 0xFF= Clic High (1st measure clic)
                                    - 0xFE= Clic Low (other measure clics)
                                    - 0x80 = "One"
                                    - 0x81 = "Two"
                                    - 0x82 = "Three"
                                    - 0x83 = "Four"
                                    - other => user file (in folder PBKR/EVT/<XX>.WAV
    [0bYCCCCCCC 0xVV] => Control Change #CCCCCCC (1-126) on channel Y (0 or 1 only) with value VV
*/
MIDI_Decoder::MIDI_Decoder(void):_event(NULL){reset();}

void MIDI_Decoder::incoming (int16_t val)
{
    if (val < -256)
    {
        _currCmd |= _currBit;
    }
    else if (val < 256)
    {
        reset();
        return;
    }

    _currBit >>=1;
    if (_currBit == 0)
    {
        if (_event)
        {
            delete _event;
        }
        printf("Evt=%04X\n",_currCmd);
        _event = new MIDI_Event(_currCmd);
        reset();
    }
}

MIDI_Event* MIDI_Decoder::pop (void)
{
    MIDI_Event* res(_event);
    if (_event)
    {
        _event = NULL;
    }
    return res;
} // MIDI_Decoder::pop

#endif // USE_MIDI_AS_TRACK

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
        _lastR(0.0),
        _pConfig(NULL)
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

    if (_pConfig) delete _pConfig;
    _pConfig = NULL;

    std::string path (MOUNT_POINT);
    path += "pbkr.xml";
    _pConfig = new XMLConfig (path.c_str());
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
    if (_pConfig) delete _pConfig;

    std::string path (MOUNT_POINT);
    path += "pbkr.xml";

    try
    {
        _pConfig = new XMLConfig (path.c_str());
        _title = _pConfig->getTitle();
    }
    catch (...) {
        _pConfig = NULL;
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

#if USE_MIDI_AS_TRACK
void FileManager::process_midi(const int16_t& midi,float& l, float & r)
{
    _midiDecoder.incoming(midi);
    MIDI_Event* e (_midiDecoder.pop());
    if (e != NULL)
    {
        bool firstChannel;
        if (e->isProgChange())
        {
            uint8_t prg;
            e->getProgChange(prg, firstChannel);
            printf("Prog change %u on chann %d\n",prg, firstChannel);
        }
        if (e->isCtrlChange())
        {
            uint8_t ctrl;
            uint8_t val;
            e->getCtrlChange(ctrl, val, firstChannel);
            printf("Ctrl change %u->%u on chann %d\n",ctrl, val, firstChannel);
        }
        if (e->isSoundEvent())
        {
            uint8_t sndId;
            e->getSoundEvent(sndId);
            printf("Sound %02X\n", sndId);
        }
        // TODO!
        l = 0.0;
        r = 0.0;
        delete (e);
    }
    else
    {
        l = 0.0;
        r = 0.0;
    }

} // FileManager::process_midi
#endif // USE_MIDI_AS_TRACK

void FileManager::getSample(float& l, float & r, float& l2, float &r2)
{
    if (_reading && _file)
    {
        MIDI_Sample midi(0);
        if (not _file->getNextSample(l ,r, midi))
        {
            stopReading();
        }

#if USE_MIDI_AS_TRACK
        process_midi(midi,l2, r2);
#else
        // 3rd track is raw WAV mono file
        l2 = midi;
        r2 = l2;
#endif // USE_MIDI_AS_TRACK

        return;
    }

    l = _lastL;
    r = _lastR;
    l2 = 0.0;
    r2 = 0.0;
} // FileManager::getSample

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
