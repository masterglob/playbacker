
#include <Arduino.h>
#include <HardwareSerial.h>
#include "slave_consts.h"

extern void on_MIDI_note(const uint8_t noteNum, const uint8_t velocity, const bool onChannel1);
extern void set_volume(const  float & volume);

/**
 * Manage reception from serial line
 * - Receive on channel 16
 * - Otherwise forward to MIDI out
 * Expecting a MIDI format.
 * Accept SYSEX messages for configuration (TODO):
 * - Program samples
 */

#define MIDI_SYSEX_CLIC1_LVL    1       // Not implemented
#define MIDI_SYSEX_CLIC2_LVL    2       // Not implemented
#define MIDI_SYSEX_CLIC1_NOTE   3       // Not implemented
#define MIDI_SYSEX_CLIC2_NOTE   4       // Not implemented
#define MIDI_SYSEX_MIDI_CHANN   5       // Not implemented
#define MIDI_SYSEX_VOLUME       6
#define MIDI_SYSEX_UPLOAD       0x7F    // Not implemented


uint8_t external_sample[MAX_EXTERNAL_SAMPLE_SIZE];
int external_sample_size(0);
int mustPlayExternalSample(0);
extern void playExternalSample(void);

/*******************************************************************************
 *         DEFINITIONS
 *******************************************************************************/
struct MIDI_SysEx
{
  MIDI_SysEx(void);
  ~MIDI_SysEx(void);
  bool start_receiving(void);
  bool receiving(void);
  void rcv(const uint8_t c);
  void execute();
private:
  bool m_Receiving;
  uint16_t m_idx;
  uint8_t m_cmd;
  bool    m_ignore;
};

struct MIDI_Msg
{
  MIDI_Msg(void):rem_bytes(0),len(0),pos(0){}
  // ==0 no message
  int rem_bytes;

  MIDI_SysEx sysex;
  uint8_t buff[3];
  uint8_t len;
  uint8_t pos;
  void rcv(const uint8_t c);
  void midi_event(void);
};

/*******************************************************************************
 *         MIDI_SysEx
 *******************************************************************************/
MIDI_SysEx::MIDI_SysEx(void):
        m_Receiving(false),
        m_idx(0),
        m_cmd(0),
        m_ignore(false)
{
}

MIDI_SysEx::~MIDI_SysEx(void)
{
}

bool MIDI_SysEx::start_receiving(void){m_Receiving = true;}
bool MIDI_SysEx::receiving(void){return m_Receiving;}
void MIDI_SysEx::execute()
{
  if (m_cmd == MIDI_SYSEX_UPLOAD)
  {
    if (external_sample_size > 0) mustPlayExternalSample = external_sample_size;
    PRINTLN (("MIDI_SYSEX_UPLOAD msg END"));
  }
  m_idx = 0;
  m_cmd =0;
  m_ignore = false;
  external_sample_size  = 0;
  m_Receiving = false;
}

void MIDI_SysEx::rcv(const uint8_t c)
{
    static const char * sys_id="CMM";
    if (m_ignore) return;
    if (m_idx < 3)
    {
        if ((const char)c != sys_id[m_idx])
        {    
          m_ignore = true;
        }
    }
    else if (m_idx == 3)
    {
        // Command type
        m_cmd = c;
    }
    else
    {
        switch (m_cmd) {
        case MIDI_SYSEX_CLIC1_LVL:
            // TODO
            break;
        case MIDI_SYSEX_CLIC2_LVL:
            // TODO
            break;
        case MIDI_SYSEX_CLIC1_NOTE:
            // TODO
            break;
        case MIDI_SYSEX_CLIC2_NOTE:
            // TODO
            break;
        case MIDI_SYSEX_MIDI_CHANN:
            // TODO
            break;
        case MIDI_SYSEX_VOLUME:
        {
            if (m_idx == 4)
            {
                const float level (((float)c) / 128.0);
                PRINTF (("New global volume level:%f\n",level));
                set_volume (level);
            }
            break;
        }
        case MIDI_SYSEX_UPLOAD:
            if (external_sample_size == 0) {PRINTLN (("MIDI_SYSEX_UPLOAD msg BEGIN"));}
            if (external_sample_size < MAX_EXTERNAL_SAMPLE_SIZE)
            {
              external_sample[external_sample_size] = c * 2;
              external_sample_size++;
            }
            break;
        default:
            break;
        }
    }
    m_idx++;
    // TODO!
}

/*******************************************************************************
 *         MIDI_Msg
 *******************************************************************************/
void MIDI_Msg::midi_event(void)
{
  
  const uint8_t channel (1 + (buff[0] & 0xF));
  if (channel == MIDI_CHANNEL1 || channel == MIDI_CHANNEL2)
  {
    const uint8_t event (buff[0] >> 4);
    if (event == 0x9)
    {
       // Note On event. 
       const uint8_t note (buff[1]);
       const uint8_t vel (buff[2]);
       if (vel > 0)
       {
         on_MIDI_note (note,vel, channel == MIDI_CHANNEL1);
       }
    }
  }
  else
  {
    // forward on serial link
    MIDI_SEND((const uint8_t *)buff, len);
  }
}

void MIDI_Msg::rcv(const uint8_t c)
{
  if (sysex.receiving())
  {
    if (c == 0xF7)
    {
      sysex.execute();
    }
    else
    {
      sysex.rcv(c);
    }
  }
  else if (rem_bytes > 0)
  {
    // continue current msg
    if (pos < 3)
    {
      buff[pos++] = c;
      rem_bytes--;
      if (rem_bytes == 0)
      {
        // post message
        midi_event();
      }
    }
    else
    {
      // Just for robustness..
      pos = 0;
      rem_bytes = 0;
      len = 0;
    }
  }
  else
  {
    // start new message
    const uint8_t cmd (c>>4);
    if (cmd == 0xF)
    {
      // System Common Messages 
      // Only accept SYSEX for reprogrammation
      if (c == 0xF0)
      {
        sysex.start_receiving();
      }
      else
      {
        // Simply ignored
      }
    }
    else
    {
      // Normal messages
      pos = 0;
      buff[pos++] = c;
      rem_bytes = 2;
      if (cmd == 0xC ||  // Prog change
          cmd == 0xD)    // Channel After touch
      {
        rem_bytes--; // Only 2 data bytes
      }
      len = rem_bytes + 1;
    }
  }
}


/*******************************************************************************
 *        EXTERNAL FUNCTIONS
 *******************************************************************************/
static MIDI_Msg msg;
void commmgt_rcv(const uint8_t c)
{
  msg.rcv(c);
}
