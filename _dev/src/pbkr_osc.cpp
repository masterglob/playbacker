#include "pbkr_types.h"
#include "pbkr_osc.h"

#include <inttypes.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string>
#include <stdexcept>

#define DO_DEBUG_IN 0
#define DO_DEBUG_OUT 0

#define OSC_PAGE "4"
#define OSC_NAME(x) "/" OSC_PAGE "/" x

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

/*******************************************************************************/
std::string splitStringPath(std::string& s)
{
    const size_t pos(s.find('/'));
    if (pos == std::string::npos)
    {
        const std::string res (s);
        s = "";
        return res;
    }
    else
    {
        const std::string res (PBKR::substring (s, 0, pos));
        s = PBKR::substring (s, 1 + pos);
        return res;
    }
}
} // namespace


/*******************************************************************************
 * EXTERNAL FUNCTIONS
 *******************************************************************************/
namespace PBKR
{
namespace OSC
{
OSC_Controller* p_osc_instance = NULL;

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
OSC_Msg_To_Send::OSC_Msg_To_Send(const OSC_Msg_To_Send& ref)
:
        OSC_Msg_Hdr(std::string (ref.m_name), ref.m_type, ref.m_valLen)
{
    memcpy (m_value, ref.m_value, m_valLen);
} // copy constructor

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
        Thread("OSC_Controller"),
        m_receiver(receiver),
        m_cfg(cfg),
        m_isClientKnown(false)
{

    setLowPriority();

    if (p_osc_instance)
        throw EXCEPTION(std::string ("OSC_Controller : Cannot create several instances!"));
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


    struct timeval read_timeout;
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = 1000;
    setsockopt(m_inSockfd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

    start();
    p_osc_instance = this;

} // OSC_Controller::OSC_Controller

/*******************************************************************************/
OSC_Controller::~OSC_Controller(void)
{
    close (m_inSockfd);
    close (m_outSockfd);
} // OSC_Controller::~OSC_Controller

/*******************************************************************************/
void OSC_Controller::sendLabelMessage(const std::string& msg)
{
    send (OSC_Msg_To_Send (OSC_NAME("lMessage"), msg));
} // OSC_Controller::sendMessage

/*******************************************************************************/
void OSC_Controller::setPbCtrlStatus(const bool isPlaying)
{
    send (OSC_Msg_To_Send (OSC_NAME("pPlay"), (float)(isPlaying ? 1.0 : 0.0)));
    send (OSC_Msg_To_Send (OSC_NAME("pStop"),  (float)(isPlaying ? 0.0 : 1.0)));
    send (OSC_Msg_To_Send (OSC_NAME("pRec"),  (float)0.0));
} // OSC_Controller::setPbCtrlStatus

/*******************************************************************************/
void OSC_Controller::setClicVolume  (const float& v)
{
    send (OSC_Msg_To_Send (OSC_NAME("clicVolume"), v));
} // OSC_Controller::setClicVolume

/*******************************************************************************/
void OSC_Controller::setProjectName(const std::string& title)
{
    send (OSC_Msg_To_Send (OSC_NAME("lPlaylist"), title));
}

/*******************************************************************************/
void OSC_Controller::setTrackName (const std::string& name, size_t trackIdx)
{
    const std::string obj (OSC_NAME("lTrack") + std::to_string(trackIdx));
    send (OSC_Msg_To_Send (obj, substring (name, 0, 5)));
}

/*******************************************************************************/
void OSC_Controller::setActiveTrack (int trackIdx)
{
    if (trackIdx >=0)
    {
        const size_t X (trackIdx % OSC_TRACK_NB_X);
        const size_t Y (trackIdx / OSC_TRACK_NB_X);
        char buff[32];
        sprintf(buff,"/%s/mtTrackSel/%u/%u",OSC_PAGE,OSC_TRACK_NB_Y - Y,X+1);
        send (OSC_Msg_To_Send (buff, (int32_t) 1));
    }
}

/*******************************************************************************/
void OSC_Controller::setFileName(const std::string& title)
{
    send (OSC_Msg_To_Send (OSC_NAME("lTrack"), title));
}

/*******************************************************************************/
void OSC_Controller::send(const OSC_Msg_To_Send& msg)
{
    if (!m_isClientKnown)
    {
        m_toSend.push_back(OSC_Msg_To_Send(msg));
        return;
    }

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
    {
#if DO_DEBUG_OUT
        printf("OSC message send to %s:%u[",
                inet_ntoa ( m_clientAddr),
                m_cfg.portOut);
        for (size_t i(0); i < msg.m_len; ++i)
        {
            printf("%02X ",((unsigned char*) msg.m_data)[i]);
        }
        printf("] => %s\n",msg.m_name);
#endif
    }
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

        printf("OSC_Controller started (In %d, out %d)\n", m_cfg.portIn, m_cfg.portOut);

        while (not isExitting())
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
                // timeout
                // usleep(10*1000);
                continue;
            }
            m_clientAddr = cliaddr.sin_addr;
            processMsg(buffer, n);
            m_isClientKnown = true;

            for (auto it(m_toSend.begin()); it != m_toSend.end(); it++)
            {
                OSC_Msg_To_Send& msg (*it);
                send(msg);
            }
            m_toSend.clear();
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
    const char* cbuff((const char*)buff);
    const std::string name(cbuff);
    const size_t nameLen(4 + (name.length() & ~3));
    const std::string type(nameLen < len ? &cbuff[nameLen] : "");

    const uint32_t* le ((const uint32_t*)(&cbuff[nameLen+4]));
    const uint32_t be (htonl(*le));
    float paramF (0.0);
    uint32_t paramI (0);
    std::string paramS("");

    if (cbuff[0] != '/') return;

    if (type == ",f")
    {
        paramF = *((const float*)&be);
#if DO_DEBUG_IN
        printf("OSC received FLOAT event <%s> => <%f>\n",name.c_str(),paramF);
#endif
    }
    else if (type == ",i")
    {
        paramI = *((uint32_t*) &be);
        (void)paramI;
#if DO_DEBUG_IN
        printf("OSC received INT event <%s> => <%u>\n",name.c_str(),paramI);
#endif
    }
    else if (type == ",s")
    {
        paramS = &name[nameLen+4];
#if DO_DEBUG_IN
        printf("OSC received STR event <%s> => <%s>\n",name.c_str(),paramS.c_str());
#endif
    }
    else if (type == ",")
    {
        //
    }
    else
    {
#if DO_DEBUG_IN
        printf("OSC received <%s> type <%s>:",
                name.c_str(), type.c_str());
        printf("[");
        for (size_t i(0); i < len ; i++) printf("%02X ",((const uint8_t*)buff)[i]);
        printf("]\n");
#endif
    }

    std::string s(&cbuff[1]);
    const std::string cmd1 (::splitStringPath(s));
    const std::string cmd2 (::splitStringPath(s));
    const std::string cmd3 (::splitStringPath(s));
    const std::string cmd4 (::splitStringPath(s));

    if (cmd1 == "ping")
    {
        static const OSC::OSC_Msg_To_Send pingMsg("/ping");
        send(pingMsg);
        m_receiver.forceRefresh();
        return;
    }

    if (cmd1 == "4")
    {
        if (paramF > 0.01)
        {
            if (cmd2 == "pPlay")
            {
                m_receiver.onPlayEvent();
            }
            else if (cmd2 == "pStop")
            {
                m_receiver.onStopEvent();

            }
            else if (cmd2 == "pRefresh")
            {
                // refresh all
                m_receiver.forceRefresh();
            }
            else if (cmd2 == "mtTrackSel")
            {
                try {
                    const int y(OSC_TRACK_NB_Y - std::atoi (cmd3.c_str()));
                    const int x(std::atoi (cmd4.c_str()) - 1);
                    m_receiver.onChangeTrack(x + y * OSC_TRACK_NB_X);
                } catch (...) {
                    printf("Invalid parameters in mtTrackSel.IGNORED\n");
                }
            }
            else if (cmd2 == "clicVolume")
            {
                m_receiver.setClicVolume(paramF);
            }

        }
    } // Page 4
    else if (cmd1 == "kbd")
    {
        if (paramF > 0.01)
        {
            processKbd(cmd2);
        }
    } // OSC keyboard
#if DO_DEBUG_IN
    printf("cmd= <%s>/<%s>/<%s>/<%s> \n",cmd1.c_str(),
            cmd2.c_str(),
            cmd3.c_str(),
            cmd4.c_str());
#endif

} // OSC_Controller::processMsg
void OSC_Controller::processKbd(const std::string key)
{
    static std::string input;
    static std::string output1;
    static std::string output2;
    if (key.length() == 1)
    {
        input += key;
    }
    else if (key == "Enter")
    {
        output2 = output1;
        output1 = m_receiver.onKeyboardCmd(input);
        input = "";
        send (OSC_Msg_To_Send ("/kbd/Output1", output1));
        send (OSC_Msg_To_Send ("/kbd/Output2", output2));
    }
    else if (key == "DEL")
    {
        if (input.length() > 0)
        {
            input.pop_back();
        }
    }
    else if (key == "Slash")
    {
        input += "/";
    }
    else if (key == "BSlash")
    {
        input += "\\";
    }
    else if (key == "Comma")
    {
        input += ",";
    }
    else if (key == "Excl")
    {
        input += "!";
    }
    else if (key == "SemiColon")
    {
        input += ";";
    }
    else if (key == "Space")
    {
        input += " ";
    }
    send (OSC_Msg_To_Send ("/kbd/InputFB", input + "|"));
}// OSC_Controller::processKbd
} // namespace OSC
} // namespace PBKR
