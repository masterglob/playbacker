#include "pbkr_midi.h"
#include "pbkr_display_mgr.h"
#include "pbkr_api.h"

#include <inttypes.h>
#include <alsa/asoundlib.h>
#include <sys/soundcard.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include <stdexcept>

/*******************************************************************************
 * LOCAL FUNCTIONS
 *******************************************************************************/
namespace
{
using namespace PBKR;

uint8_t* dupMem(const uint8_t* data, const size_t len)
{
    uint8_t* res ((uint8_t*)malloc (len));
    memcpy(res, data, len);
    return res;
}


//////////////////////////////
//
// is_input -- returns true if specified card/device/sub can output MIDI data.
//

int is_input(snd_ctl_t *ctl, int card, int device, int sub) {
   snd_rawmidi_info_t *info;
   int status;

   snd_rawmidi_info_alloca(&info);
   snd_rawmidi_info_set_device(info, device);
   snd_rawmidi_info_set_subdevice(info, sub);
   snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_INPUT);

   if ((status = snd_ctl_rawmidi_info(ctl, info)) < 0 && status != -ENXIO) {
      return status;
   } else if (status == 0) {
      return 1;
   }

   return 0;
}



//////////////////////////////
//
// is_output -- returns true if specified card/device/sub can output MIDI data.
//

int is_output(snd_ctl_t *ctl, int card, int device, int sub) {
   snd_rawmidi_info_t *info;
   int status;

   snd_rawmidi_info_alloca(&info);
   snd_rawmidi_info_set_device(info, device);
   snd_rawmidi_info_set_subdevice(info, sub);
   snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_OUTPUT);

   if ((status = snd_ctl_rawmidi_info(ctl, info)) < 0 && status != -ENXIO) {
      return status;
   } else if (status == 0) {
      return 1;
   }

   return 0;
}

void insert_subdevice_list(snd_ctl_t *ctl, int card, int device,
        MIDI::MIDI_Ctrl_Cfg_Vect& vect) {
   snd_rawmidi_info_t *info;
   const char *name;
   const char *sub_name;
   int subs, subs_in, subs_out;
   int sub, in, out;
   int status;

   snd_rawmidi_info_alloca(&info);
   snd_rawmidi_info_set_device(info, device);

   snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_INPUT);
   snd_ctl_rawmidi_info(ctl, info);
   subs_in = snd_rawmidi_info_get_subdevices_count(info);
   snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_OUTPUT);
   snd_ctl_rawmidi_info(ctl, info);
   subs_out = snd_rawmidi_info_get_subdevices_count(info);
   subs = subs_in > subs_out ? subs_in : subs_out;

   sub = 0;
   in = out = 0;
   if ((status = is_output(ctl, card, device, sub)) < 0) {
      printf("cannot get rawmidi information %d:%d: %s\n",
            card, device, snd_strerror(status));
      return;
   } else if (status)
      out = 1;

   if (status == 0) {
      if ((status = is_input(ctl, card, device, sub)) < 0) {
          printf("cannot get rawmidi information %d:%d: %s\n",
               card, device, snd_strerror(status));
         return;
      }
   } else if (status)
      in = 1;

   if (status == 0)
      return;

   name = snd_rawmidi_info_get_name(info);
   sub_name = snd_rawmidi_info_get_subdevice_name(info);
   char devname[32];
   if (sub_name[0] == '\0') {
       sprintf(devname,"hw:%d,%d",card,device);
       MIDI::MIDI_Ctrl_Cfg cfg;
       cfg.device = devname;
       cfg.name = name;
       cfg.isInput = in;
       cfg.isOutput = out;
       vect.push_back(cfg);
   } else {
      sub = 0;
      for (;;) {
          MIDI::MIDI_Ctrl_Cfg cfg;
          sprintf(devname,"hw:%d,%d,%d",card,device, sub);
          cfg.device = devname;
          cfg.name = sub_name;
          cfg.isInput = in;
          cfg.isOutput = out;
          vect.push_back(cfg);
         if (++sub >= subs)
            break;

         in = is_input(ctl, card, device, sub);
         out = is_output(ctl, card, device, sub);
         snd_rawmidi_info_set_subdevice(info, sub);
         if (out) {
            snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_OUTPUT);
            if ((status = snd_ctl_rawmidi_info(ctl, info)) < 0) {
                printf("cannot get rawmidi information %d:%d:%d: %s\n",
                     card, device, sub, snd_strerror(status));
               break;
            }
         } else {
            snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_INPUT);
            if ((status = snd_ctl_rawmidi_info(ctl, info)) < 0) {
                printf("cannot get rawmidi information %d:%d:%d: %s\n",
                     card, device, sub, snd_strerror(status));
               break;
            }
         }
         sub_name = snd_rawmidi_info_get_subdevice_name(info);
      }
   }
} // get_subdevice_list

} // namespace


/*******************************************************************************
 * EXTERNAL FUNCTIONS
 *******************************************************************************/
namespace PBKR
{
namespace MIDI
{

MIDI_Msg::MIDI_Msg(const uint8_t* data, const size_t len):
    m_len(len),m_msg(::dupMem(data,len)) {}
MIDI_Msg::~MIDI_Msg (void){ free ((void*)m_msg);}

/*******************************************************************************/
MIDI_Controller::MIDI_Controller(const MIDI_Ctrl_Cfg& cfg, MIDI_Controller_Mgr& mgr):
        Thread(std::string("MIDI_Controller:") + cfg.name),
        m_midiin(NULL),
        m_midiout(NULL),
        m_cfg(cfg),
        m_mgr(mgr)
{
    DISPLAY::DisplayManager::instance().info(cfg.name + " connected.");
    // note : MIDI read fail if the open is not done in the same thread as read
    start();
} // MIDI_Controller::MIDI_Controller

/*******************************************************************************/
MIDI_Controller::~MIDI_Controller(void)
{
    printf("Closing %s\n",m_cfg.name.c_str());
    if (m_midiin)
        snd_rawmidi_close (m_midiin);
    m_midiin= NULL;
}// MIDI_Controller::~MIDI_Controller

/*******************************************************************************/
void MIDI_Controller::body(void)
{
    int status;
    static const size_t MAX_MSG_LEN(128);
    uint8_t buffer[MAX_MSG_LEN];
    bool sysex(false);
    size_t explen(0);
    size_t pos (0);

    try
    {
        static const int mode = SND_RAWMIDI_SYNC | SND_RAWMIDI_NONBLOCK;
        if ((status = snd_rawmidi_open(&m_midiin, &m_midiout, m_cfg.device.c_str(), mode)) < 0) {
            throw EXCEPTION(std::string ("Failed to open ") + m_cfg.name +
                    "(" + m_cfg.device + ")");
        }
        printf("Opened MIDI device: %s\n",m_cfg.device.c_str());

        while (not isExitting())
        {
            pos = 0;
            sysex = false;
            explen = 0;
            while (pos < MAX_MSG_LEN)
                // break this loop to send a message
            {
                uint8_t& c (buffer[pos]);

                while ((status = snd_rawmidi_read(m_midiin, (char*)&c, 1)) < 0)
                {
                    if (isExitting())
                        throw EXCEPTION("Exit required");
                    if (status == -EAGAIN)
                    {
                        usleep (1000);
                        continue;
                    }
                    printf("%s MIDI status = %d\n",m_cfg.device.c_str(), status);
                    throw EXCEPTION("Disconnected");
                }

                pos++;
                if (sysex)
                {
                    // End of SYSEX Message ?
                    if (c == 0XF7) break;
                }
                else if (explen > 0)
                {
                    // Currently receiving msg
                    if (pos == explen) break;
                }
                else if (c >= 0xF0) sysex = true;
                else if (c >= 0x80) {
                    const uint8_t cmd (c & 0xF0);
                    if (cmd == 0xC0 || cmd == 0xD0)
                        explen = 2; // Prog change or channel aftertouch
                    else
                        explen = 3;
                }
                else
                {
                    // Unexpected data
                    printf ("Unexpected MIDI data:%02X\n",c);
                    pos = 0;
                    continue;
                }
            }
            if (pos >= MAX_MSG_LEN) continue; // drop
            // printf ("Rcvd MIDI msg (len =%d)\n",pos);
            const MIDI_Msg msg (buffer,pos);
            API::onMidiEvent(msg, m_cfg);
        }
    }
    catch (...)
    {
        m_mgr.onDisconnect(m_cfg);
        delete this;
    }
} // MIDI_Controller::body


/*******************************************************************************
 * MIDI CONTROLLER MANAGER
 *******************************************************************************/
MIDI_Controller_Mgr& MIDI_Controller_Mgr::instance()
{
    static MIDI_Controller_Mgr _instance;
    return _instance;
}
MIDI::MIDI_Controller_Mgr& midiMgrInstance(MIDI_Controller_Mgr::instance());

/*******************************************************************************/
MIDI_Controller_Mgr::MIDI_Controller_Mgr(void):
        Thread("MIDI_Controller_Mgr"),
        mMidiLearn(MainMenu::KEY_NONE)
{
    m_InputControllers.clear();
    start();
} // MIDI_Controller_Mgr::MIDI_Controller_Mgr

/*******************************************************************************/
void MIDI_Controller_Mgr::body(void)
{
    while (!isExitting())
    {
        loop();
        usleep(100 * 1000);
    }
};

/*******************************************************************************/
void MIDI_Controller_Mgr::onInputConnect (const MIDI::MIDI_Ctrl_Cfg& cfg)
{
    printf("Found: %s (%s)\n", cfg.name.c_str(), cfg.device.c_str());
    MIDI::MIDI_Controller* midi_ctrl(new MIDI_Controller (cfg, *this));
    (void)midi_ctrl;
}


/*******************************************************************************/
void MIDI_Controller_Mgr::onDisconnect (const MIDI_Ctrl_Cfg& cfg)
{
    DISPLAY::DisplayManager::instance().info(cfg.name + " disconnected.");
    for (MIDI_Ctrl_Cfg_Vect::iterator it (m_InputControllers.begin()); it != m_InputControllers.end(); it++)
    {
        const MIDI_Ctrl_Cfg& cfg2 (*it);
        if (cfg.device == cfg2.device)
        {
            m_InputControllers.erase(it);
            break;
        }
    }
}

/*******************************************************************************/
void MIDI_Controller_Mgr::loop(void)
{
    // check presence of MIDI devices.
    MIDI_Ctrl_Cfg_Vect newMIDIs;
    newMIDIs.clear();
    int status;
    int card = -1;
    if ((status = snd_card_next(&card)) < 0){
        printf("cannot determine card number: %s\n", snd_strerror(status));
        return;
    }
    if (card < 0) card = -1;

    while (card >= 0) {
        int device = -1;
        snd_ctl_t *ctl;
        char name[32];
        sprintf(name, "hw:%d", card);
        if ((status = snd_ctl_open(&ctl, name, 0)) < 0)
        {
            printf("cannot open control for card %d: %s\n",
                    card, snd_strerror(status));
            break;
        }
        do {
            status = snd_ctl_rawmidi_next_device(ctl, &device);
            if (status < 0) {
                // printf("cannot determine device number for card %d: %s\n", card, snd_strerror(status));
                break;
            }
            if (device >= 0) {
                insert_subdevice_list(ctl, card, device,newMIDIs);
            }
        } while (device >= 0);
        snd_ctl_close(ctl);

        if ((status = snd_card_next(&card)) < 0) {
            printf("cannot determine card number: %s\n", snd_strerror(status));
            break;
        }
    }

    for (auto it (newMIDIs.begin()); it != newMIDIs.end(); it++)
    {
        const MIDI_Ctrl_Cfg& cfg (*it);
        if (cfg.isInput)
        {
            m_mutex.lock();
            bool found(false);
            // Was it already present?
                    for (auto jt (m_InputControllers.begin()); jt != m_InputControllers.end(); jt++)
                    {
                        const MIDI_Ctrl_Cfg& prev (*jt);
                        if (cfg.device == prev.device) found = true;
                    }
                    if (not found)
                    {
                        m_InputControllers.push_back(cfg);
                        onInputConnect (cfg);
                    }
                    m_mutex.unlock();
        }
    }

} // MIDI_Controller_Mgr::body

/*******************************************************************************/
const MIDI_Ctrl_Cfg_Vect  MIDI_Controller_Mgr::getControllers(void)
{
    m_mutex.lock();
    const MIDI_Ctrl_Cfg_Vect v(m_InputControllers);
    m_mutex.unlock();
    return v;
} // MIDI_Controller_Mgr::getControllers


} // namespace MIDI
} // namespace PBKR
