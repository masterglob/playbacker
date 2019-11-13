#ifndef I_pbkr_osc_h_I
#define I_pbkr_osc_h_I

#include "pbkr_config.h"
#include "pbkr_utils.h"
#include <inttypes.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdexcept>

namespace PBKR
{
namespace OSC
{

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
    virtual void onNoValueEvent (const std::string& name) = 0;
    virtual void onFloatEvent (const std::string& name, float f) = 0;
};



/*******************************************************************************
 * OSC CONTROLLER
 *******************************************************************************/

class OSC_Controller : private Thread
{
public:
    OSC_Controller(const OSC_Ctrl_Cfg& cfg, OSC_Event& receiver);
    virtual ~OSC_Controller(void);
    void sendPing(void);
private:
    virtual void body(void);
    void send(const void* buff, const size_t len);
    void processMsg(const void* buff, const size_t len);
    OSC_Event & m_receiver;
    const OSC_Ctrl_Cfg m_cfg;
    int m_inSockfd;
    int m_outSockfd;
    in_addr m_clientAddr;
}; // class OSC_Controller


}  // namespace OSC
} // namespace PBKR
#endif // I_pbkr_osc_h_I
