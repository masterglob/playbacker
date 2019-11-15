#ifndef I_pbkr_osc_h_I
#define I_pbkr_osc_h_I

#include "pbkr_config.h"
#include "pbkr_utils.h"
#include <inttypes.h>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <mutex>

namespace PBKR
{
namespace OSC
{
static const size_t OSC_TRACK_NB_X (7u);
static const size_t OSC_TRACK_NB_Y (2u);
static const size_t NB_OSC_TRACK (OSC_TRACK_NB_X * OSC_TRACK_NB_Y);


struct OSC_Ctrl_Cfg
{
    OSC_Ctrl_Cfg (unsigned short in, unsigned short out):
        portIn(in),
        portOut(out)
    {
    }
    unsigned short portIn;
    unsigned short portOut;
};

class OSC_Controller;

/***
 * Derive this class to receive MIDI events from OSC_Controller
 */
struct OSC_Event
{
    virtual ~OSC_Event(void){}
    virtual void onPlayEvent    (void) = 0;
    virtual void onStopEvent    (void) = 0;
    virtual void forceRefresh    (void) = 0;
    virtual void onChangeTrack  (const uint32_t idx) = 0;
};

/**
 * Common OSC header for OSC_Msg_To_Send
 */
struct OSC_Msg_Hdr
{
protected:
    OSC_Msg_Hdr(const std::string& name,const std::string&typeName,const size_t valLen);
    virtual ~OSC_Msg_Hdr(void);
    const size_t m_nameLen;
    const size_t m_typeLen;
    const size_t m_valLen;
public :
    const size_t m_len;
    void* const m_data;
    char* const m_name;
    char* const m_type;
protected:
    uint8_t* const m_value;
};

/**
 * A message to OSC
 */
struct OSC_Msg_To_Send : public OSC_Msg_Hdr
{
    OSC_Msg_To_Send(const OSC_Msg_To_Send& ref);
    OSC_Msg_To_Send(const std::string& name);
    OSC_Msg_To_Send(const std::string& name,const std::string& strVal);
    OSC_Msg_To_Send(const std::string& name,const float fltVal);
    OSC_Msg_To_Send(const std::string& name,const int32_t i32Val);
    size_t len(void)const{return m_len;}
    const void* data(void)const{return m_data;}
};
typedef std::vector<OSC_Msg_To_Send,std::allocator<OSC_Msg_To_Send>> OSC_Msg_To_Send_Vect;

/*******************************************************************************
 * OSC CONTROLLER
 *******************************************************************************/

class OSC_Controller : private Thread
{
public:
    OSC_Controller(const OSC_Ctrl_Cfg& cfg, OSC_Event& receiver);
    virtual ~OSC_Controller(void);
    void send(const OSC_Msg_To_Send& msg);
    void setProjectName(const std::string& title);
    void setFileName(const std::string& title);
    void setTrackName (const std::string& name, size_t trackIdx);
    void setActiveTrack (int trackIdx);
    void sendLabelMessage(const std::string& msg);
    void setPbCtrlStatus(const bool isPlaying);
private:
    virtual void body(void);
    void processMsg(const void* buff, const size_t len);
    OSC_Event & m_receiver;
    const OSC_Ctrl_Cfg m_cfg;
    int m_inSockfd;
    int m_outSockfd;
    in_addr m_clientAddr;
    bool  m_isClientKnown;
    OSC_Msg_To_Send_Vect m_toSend;
}; // class OSC_Controller
extern OSC_Controller* p_osc_instance;


}  // namespace OSC
} // namespace PBKR
#endif // I_pbkr_osc_h_I
