#include "pbkr_osc.h"

#include <inttypes.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string>
#include <stdexcept>

/*******************************************************************************
 * LOCAL FUNCTIONS
 *******************************************************************************/
namespace
{
static size_t oscLen(const std::string& s){ return (s.length() +4) & ~3;}
static const std::string osc_noType(",");
static const std::string osc_intType(",i");
static const std::string osc_stringType(",s");
static const std::string osc_floatType(",f");
inline void* MALLOC(size_t len)
{
    void*res(malloc(len));
    if(!res) throw PBKR::EXCEPTION ("OOM!");
    return res;
}

} // namespace


/*******************************************************************************
 * EXTERNAL FUNCTIONS
 *******************************************************************************/
namespace PBKR
{
namespace OSC
{


/*******************************************************************************
 * OSC_Msg_Hdr
 *******************************************************************************/
/*******************************************************************************/
OSC_Msg_Hdr::OSC_Msg_Hdr(const std::string& name, const std::string& typeName,const size_t valLen):
                                m_nameLen(oscLen(name)),
                                m_typeLen(oscLen(typeName)),
                                m_valLen(valLen),
                                m_len(m_nameLen + m_typeLen + m_valLen),
                                m_data(::MALLOC(m_len)),
                                m_name((char*)m_data),
                                m_type(((char*)m_data) + m_nameLen),
                                m_value(((uint8_t*)m_type) + m_typeLen)
{
    memset(m_data, 0, m_len);
    strcpy(m_name, name.c_str());
    strcpy(m_type, typeName.c_str());
} // OSC_Msg_Hdr constructor

/*******************************************************************************/
OSC_Msg_Hdr::~OSC_Msg_Hdr(void)
{
    free(m_data);
} // OSC_Msg_Hdr destructor

/*******************************************************************************
 * OSC_Msg_To_Send
 *******************************************************************************/
/*******************************************************************************/
OSC_Msg_To_Send::OSC_Msg_To_Send(const std::string& name)
:
        OSC_Msg_Hdr(name, osc_noType, 0)
{
} // empty constructor

/*******************************************************************************/
OSC_Msg_To_Send::OSC_Msg_To_Send(const std::string& name,const std::string& strVal)
:
        OSC_Msg_Hdr(name, osc_stringType, oscLen(strVal))
{
    strcpy((char*)m_value, strVal.c_str());
} // string constructor

/*******************************************************************************/
OSC_Msg_To_Send::OSC_Msg_To_Send(const std::string& name,const float fltVal)
:
        OSC_Msg_Hdr(name, osc_floatType, sizeof(float))
{
    uint32_t& le=*((uint32_t *)&fltVal);  // hold "native" value
    uint32_t& be=*((uint32_t *)m_value);  // hold "big-endian" value

    be = htonl(le);
} // float constructor
/*******************************************************************************/
OSC_Msg_To_Send::OSC_Msg_To_Send(const std::string& name,const int32_t i32Val)
:
        OSC_Msg_Hdr(name, osc_intType, sizeof(uint32_t))
{
    uint32_t& le=*((uint32_t *)&i32Val);  // hold "native" value
    uint32_t& be=*((uint32_t *)m_value);  // hold "big-endian" value

    be = htonl(le);
} // int constructor

/*******************************************************************************
 * OSC CONTROLLER
 *******************************************************************************/

/*******************************************************************************/
OSC_Controller::OSC_Controller(const OSC_Ctrl_Cfg& cfg, OSC_Event& receiver):
        m_receiver(receiver),
        m_cfg(cfg)
{

    // Creating socket file descriptor
    m_inSockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_inSockfd < 0 )
        throw EXCEPTION(std::string ("OSC_Controller : Failed to CREATE socket1! "));

    m_clientAddr.s_addr = INADDR_ANY;
    struct sockaddr_in servaddr;

    memset(&servaddr, 0, sizeof(servaddr));

    // Filling server information
    servaddr.sin_family    = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(m_cfg.portIn);

    // Bind the socket with the server address
    if ( bind(m_inSockfd, (const struct sockaddr *)&servaddr,
            sizeof(servaddr)) < 0 )
    {
        throw EXCEPTION(std::string ("OSC_Controller : Failed to BIND socket!"));
    }

    m_outSockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_outSockfd < 0 )
        throw EXCEPTION(std::string ("OSC_Controller : Failed to CREATE socket2! "));

    start();
} // MIDI_Controller::MIDI_Controller

/*******************************************************************************/
OSC_Controller::~OSC_Controller(void)
{
    close (m_inSockfd);
    close (m_outSockfd);
} // MIDI_Controller::~MIDI_Controller

/*******************************************************************************/
void OSC_Controller::send(const OSC_Msg_To_Send& msg)
{

    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));

    // Filling server information
    addr.sin_family    = AF_INET; // IPv4
    addr.sin_addr = m_clientAddr;
    addr.sin_port = htons(m_cfg.portOut);

    const int n (sendto (m_outSockfd,msg.m_data,msg.m_len,
            MSG_CONFIRM,(const struct sockaddr *) &addr, sizeof(addr)));
    if (n <0 )
    {
        printf("OSC message send failed\n");
    }
    else
        printf("OSC message send %s to %s:%u\n",msg.m_name,
                inet_ntoa ( m_clientAddr),
                m_cfg.portOut);
}
/*******************************************************************************/
void OSC_Controller::body(void)
{

    static const size_t MAXLSIZE(256);
    void * buffer = malloc(MAXLSIZE);
    if (!buffer)
        throw EXCEPTION(std::string ("OSC_Controller : OOM!1"));

    try
    {

        printf("OSC_Controller started (In %d, out %d\n)", m_cfg.portIn, m_cfg.portOut);

        while (true)
        {


            struct sockaddr_in cliaddr;
            memset(&cliaddr, 0, sizeof(cliaddr));

            unsigned int len(sizeof(cliaddr));
            int n;
            n = recvfrom(m_inSockfd, buffer, MAXLSIZE,
                        MSG_WAITALL, ( struct sockaddr *) &cliaddr,
                        &len);
            if (n < 0)
            {
                printf("OSC received FAILED(%d)\n",n);
                sleep(1);
            }
            else
            {
                m_clientAddr = cliaddr.sin_addr;
                processMsg(buffer, n);
            }
        }
    }
    catch (...)
    {
        delete this;
    }
} // OSC_Controller::body

/*******************************************************************************/
void OSC_Controller::processMsg(const void* buff, const size_t len)
{
    const char* name((const char*)buff);
    const size_t nameLen(4 + (strlen(name) & ~3));
    const std::string type(nameLen < len ? &name[nameLen] : "");

    if (type == ",f")
    {
        const uint32_t* le ((const uint32_t*)&name[nameLen+4]);
        const uint32_t be (htonl(*le));
        const float*f ((const float*) &be);
        m_receiver.onFloatEvent(name, *f);
    }
    else if (type == ",i")
    {
        const uint32_t*f ((uint32_t*) &name[nameLen+4]);
        printf("OSC received INT event <%s> => <%u>\n",name,*f);
    }
    else if (type == ",s")
    {
        printf("OSC received STR event <%s> => <%s>\n",name,&name[nameLen+4]);
    }
    else if (type == ",")
    {
        m_receiver.onNoValueEvent(name);
    }
    else
    {
        printf("OSC received <%s> type <%s>:",
                name, type.c_str());
        printf("[");
        for (size_t i(0); i < len ; i++) printf("%02X ",((const uint8_t*)buff)[i]);
        printf("]\n");
    }

} // OSC_Controller::processMsg

} // namespace OSC
} // namespace PBKR
