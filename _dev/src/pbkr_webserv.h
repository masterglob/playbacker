#ifndef I_pbkr_webserv_h_I
#define I_pbkr_webserv_h_I

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <thread>
#include <vector>

#include "pbkr_config.h"
#include "pbkr_utils.h"
#include "pbkr_midi.h"

namespace PBKR
{

namespace WEB
{
/*******************************************************************************
 * GLOBAL CONSTANTS
 *******************************************************************************/
class WebSrv;

class SockListener : std :: thread
{
public:
    SockListener(WebSrv* srv, int id);
    virtual ~SockListener(void);
    // The thread implementation
    void do_listen(int id);
private:
    void process_request(const std::string& req);
    WebSrv* m_srv;
    int m_fd;
    std::thread m_thread;
};


struct HTMLParam
{
    std::string name;
    std::string value;
};

typedef std::vector<HTMLParam,std::allocator<HTMLParam>> ParamVect ;

/*******************************************************************************
 * WEB SERVER
 *******************************************************************************/
class WebSrv
{
public:
    WebSrv (uint16_t port, size_t max_clients);
    virtual ~WebSrv(void);
    int sockfd(void)const{return m_sockfd;}
    virtual std::string onGET (const std::string& page, const ParamVect& params) = 0;
private:
    int m_sockfd;
    size_t m_nb_clients;
    struct sockaddr_in m_serv_addr;
    SockListener** m_listeners;
};


/*******************************************************************************
 * BasicWebSrv (exemple)
 *******************************************************************************/

class BasicWebSrv : public WEB::WebSrv
{
public:
    BasicWebSrv(FileManager& manager,
            MIDI::MIDI_Controller_Mgr &midiMgr);
    virtual ~BasicWebSrv(void);
    virtual std::string onGET (const std::string& page, const WEB::ParamVect& params);
    std::string pageRoot(const WEB::ParamVect& params);
    std::string pageMIDI(const WEB::ParamVect& params);
    std::string configureMIDI(const std::string& dev);

    std::string pagePLAY(const WEB::ParamVect& params);

    std::string pageTEST(const WEB::ParamVect& params);
    static std::string findParamValue(
            const WEB::ParamVect& params,
            const std::string & name);
private:
    FileManager& m_manager;
    MIDI::MIDI_Controller_Mgr& m_midiMgr;
};


/*******************************************************************************
 * UTILS
 *******************************************************************************/
std::string toLink(const std::string& s,const std::string &hl);
std::string toBold(const std::string& s);
std::string toTitle1(const std::string& s);
std::string toTitle2(const std::string& s);
inline std::string newline(void){return "<BR>";}
} // namespace WEB
} // namespace PBKR

#endif // I_pbkr_webserv_h_I
