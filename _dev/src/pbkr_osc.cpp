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

} // namespace


/*******************************************************************************
 * EXTERNAL FUNCTIONS
 *******************************************************************************/
namespace PBKR
{
namespace OSC
{


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
void OSC_Controller::send(const void* buff, const size_t len)
{

    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));

    // Filling server information
    addr.sin_family    = AF_INET; // IPv4
    addr.sin_addr = m_clientAddr;
    addr.sin_port = htons(m_cfg.portOut);

    const int n (sendto (m_outSockfd,buff,len,
            MSG_CONFIRM,(const struct sockaddr *) &addr, sizeof(addr)));
    if (n <0 )
    {
        printf("OSC message send failed\n");
    }
    else
        printf("OSC message send %s to %s:%u\n",(const char*)buff,
                inet_ntoa ( m_clientAddr),
                m_cfg.portOut);
}

/*******************************************************************************/
void OSC_Controller::sendPing(void)
{
    static const char* buffer ("/ping\0\0\0,\0\0\0\0");
    send((const void*)buffer, 12u);


} // MIDI_Controller::sendPing

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
