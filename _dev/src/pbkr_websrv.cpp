#include "pbkr_webserv.h"

#include <stdio.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <fstream>
#include <sstream>

namespace
{

static const char* http_code_name(const int code)
{
    switch (code) {
    case 200: return "OK";
    case 404: return "PAGE NOT FOUND";
    default:
        return "BAD REQUEST";
    }
}
void http_send(const int fd, const int code, const std::string& body)
{
    char hdr[100];
    sprintf(hdr,"HTTP/1.1 %d %s\nContent-Type: text/html\nContent-Length: %u\n\n",
            code, http_code_name(code),body.length());
    const std::string ans(std::string(hdr) + body);
    send (fd, (const void*)ans.c_str(),(unsigned int) ans.length(),0);
}

}

namespace PBKR
{
namespace WEB
{

/*******************************************************************************/
SockListener::SockListener(WebSrv* srv, int id):
        Thread("SockListener"),
        m_srv(srv),
        m_fd(-1),
        m_id(id)
{
    Thread::start();
}

/*******************************************************************************/
SockListener::~SockListener(void)
{
}
 void SockListener::body(void)
 {
     do_listen(m_id);
 }

/*******************************************************************************/
void SockListener::do_listen(int id)
{
    while (not isExitting())
    {
        struct sockaddr_in cli_addr;
        socklen_t clilen (sizeof(cli_addr));

        // do accept
        {
            fd_set setReads;
            timeval timeout = {.tv_sec = 0, .tv_usec = 20000 };
            FD_SET(m_srv->sockfd(), &setReads);
            int selectResult = select(m_srv->sockfd() + 1, &setReads, nullptr, nullptr, &timeout);
            if (selectResult == 0)
            {
                continue;
            }
            else if (selectResult < 0)
                throw EXCEPTION(std::string ("Failed to ACCEPT socket! "));
        }

        m_fd = accept(m_srv->sockfd(),
                (struct sockaddr *) &cli_addr, &clilen);
        if (m_fd < 0)
            throw EXCEPTION(std::string ("Failed to ACCEPT socket! "));

        /*printf("server: got connection from %s port %d on accept %d\n",
                    inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port),id);*/

        std::string request;
        do
        {
            fd_set setReads;
            timeval timeout = {.tv_sec = 0, .tv_usec = 20000 };
            FD_SET(m_fd, &setReads);
            int selectResult = select(m_fd + 1, &setReads, nullptr, nullptr, &timeout);

            if (selectResult == -1)
                throw EXCEPTION(std::string ("Failed to SELECT socket! "));
            else if (selectResult > 0)
            {
                static const int BUFF_MAX(200);
                char buffer[BUFF_MAX];
                int ret = recv(m_fd, buffer, BUFF_MAX-1, 0);
                if (ret > 0)
                {
                    request += buffer;
                    continue;
                }
                if (ret < 0)
                {
                    break;
                }
            }
            else
                continue;
        } while(false);
         printf("%s\n",request.c_str());
        process_request(request);

        close(m_fd);
    }
} // SockListener::do_listen

/*******************************************************************************/
void SockListener::process_request(const std::string& req)
{
    const char* s(req.c_str());

    while (s[0] != 0)
    {
        if (strncmp (s, "GET ", 4) == 0)
        {
            const char* addr(&s[4]);
            int i (0);
            while (addr[i] != 0 && addr[i] != ' ') i ++;
            char addr2[i+1];
            snprintf(addr2, i+1,"%s", addr);


            ParamVect paramVect;
            // extract params
            std::string s(addr2);
            const size_t pos (s.find ('&'));
            const std::string page (s.substr(0, pos));
            if (pos != std::string::npos)
            {
                // with params
                const std::string params ((pos == std::string::npos ? "" : s.substr(pos+1)));
                const char* params2(params.c_str());
                bool isValue(false);
                HTMLParam hparam;
                size_t pos0(0);
                for (size_t posp (0); posp < params.length(); posp++)
                {
                    const char c(params2 [posp]);
                    if (not isValue)
                    {
                        if(c == '=')
                        {
                            isValue = true;
                            hparam.name = params.substr(pos0, posp-pos0);
                            pos0 = posp + 1;
                        }
                        else if (c == '&')
                        {
                            hparam.value = params.substr(pos0, posp-pos0);
                            paramVect.push_back(hparam);
                            hparam.value = "";
                            hparam.name = "";
                            pos0 = posp + 1;
                        }
                    }
                    else
                    {
                        if (c == '&')
                        {
                            hparam.value = params.substr(pos0, posp-pos0);
                            paramVect.push_back(hparam);
                            hparam.value = "";
                            hparam.name = "";
                            pos0 = posp + 1;
                            isValue = false;
                        }
                    }
                }
                if (hparam.name != "")
                {
                    hparam.value = params.substr(pos0, params.length()-pos0);
                    paramVect.push_back(hparam);
                }
            }

            const std::string answer (m_srv -> onGET (page, paramVect));
            std::string html("<html><head><title>WT Player</title></head>\n<body>");
            html += answer;
            html += "\n</body></html>";
            if (answer.length() == 0)
                http_send(m_fd, 404,"Page not found!");
            else
                http_send(m_fd, 200,html);
            return;
            // goto next EOL
        }
        while (s[0] != 0 && s[0] != '\n' ) s++;
    }
    http_send(m_fd, 404,"Page not found!");
} // SockListener::process_request

/*******************************************************************************
 * WEB SERVER
 *******************************************************************************/

/*******************************************************************************/
WebSrv::WebSrv (uint16_t port, size_t max_clients):
                m_sockfd (socket(AF_INET, SOCK_STREAM, 0)),
                m_nb_clients(max_clients)
{
    int res;
    if (m_sockfd < 0)
        throw EXCEPTION(std::string ("Failed to create socket! "));

    int enable = 1;
    if (setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        throw EXCEPTION("setsockopt(SO_REUSEADDR) failed");

    bzero((char *) &m_serv_addr, sizeof(m_serv_addr));
    m_serv_addr.sin_family = AF_INET;
    m_serv_addr.sin_addr.s_addr = INADDR_ANY;
    m_serv_addr.sin_port = htons(port);
    res = bind(m_sockfd, (struct sockaddr *) &m_serv_addr, sizeof(m_serv_addr));
    if (res < 0)
        throw EXCEPTION(std::string ("Failed to bind port "+ std::to_string(port)));
    listen (m_sockfd,max_clients);

    m_listeners = (SockListener**) malloc(max_clients * sizeof(SockListener*));
    for (size_t i(0); i<m_nb_clients ; i++)
    {
        m_listeners[i] = new SockListener(this,i);
    }
    printf("[WEB] listening to %d clients on %s:%u\n",
            m_nb_clients,"192.168.22.1",port);
}

/*******************************************************************************/
WebSrv::~WebSrv(void)
{
    for (size_t i(0); i<m_nb_clients ; i++)
    {
        delete m_listeners[i];
    }
    close(m_sockfd);
}

/*******************************************************************************
 * BasicWebSrv (exemple)
 *******************************************************************************/
/*******************************************************************************/
BasicWebSrv::BasicWebSrv(FileManager& manager, MIDI::MIDI_Controller_Mgr &midiMgr)
:
        WebSrv(80,4),
        m_manager(manager),
        m_midiMgr(midiMgr)
{}
/*******************************************************************************/
BasicWebSrv::~BasicWebSrv(void){}

/*******************************************************************************/
std::string BasicWebSrv::onGET (const std::string& page, const WEB::ParamVect& params)
{
    printf("PAge requested:%s\n",page.c_str());
    if (page == "/")
        return pageRoot(params);
    if (page == "/midi")
        return pageMIDI(params);
    if (page == "/test")
        return pageTEST(params);
    if (page == "/play")
        return pagePLAY(params);
    // otherwise just check files!
    try
    {
        static const std::string WWW_HOME ("/home/tc/www");
        const std::string filename (WWW_HOME + page + ".html");
        printf("Checking for file %s\n",filename.c_str());
        std::ifstream inFile(filename);
        std::stringstream strStream;
        strStream << inFile.rdbuf(); //read the file
        return strStream.str();
    }
    catch (...) {
        return "";
    }
}

/*******************************************************************************/
std::string BasicWebSrv::pageRoot(const WEB::ParamVect& params)
{
    std::ifstream inFile("/home/tc/www/index.html");
    std::stringstream strStream;
    strStream << inFile.rdbuf(); //read the file
    return strStream.str();
}

/*******************************************************************************/
std::string BasicWebSrv::pageMIDI(const WEB::ParamVect& params)
{
    std::string res(WEB::toTitle1("MIDI configuration."));
    res += WEB::toLink("Return to main page", "/");
    res += WEB::newline();
    res += WEB::toLink("Refresh", "/midi");
    res += WEB::newline();

    for (auto it = params.begin();it != params.end();it++)
    {
        const WEB::HTMLParam& p(*it);
        if(p.name == "dev")
        {
            const std::string s (configureMIDI(p.value));
            res +=s;
            return res;
        }
    }

    res += "MIDI devices:<BR>";
    const MIDI::MIDI_Ctrl_Cfg_Vect vect(m_midiMgr.getControllers());
    for (auto it (vect.begin()); it != vect.end();it++)
    {
        const MIDI::MIDI_Ctrl_Cfg& cfg(*it);
        res += "<a href='/midi&dev=";
        res += cfg.device;
        res += "'> Configure <B>";
        res += cfg.name;
        res += "</B></a>";
        res += "<BR>";
    }
    return res;
}

/*******************************************************************************/
std::string BasicWebSrv::configureMIDI(const std::string& dev)
{
    using namespace std;
    string res;
    const MIDI::MIDI_Ctrl_Cfg_Vect vect(m_midiMgr.getControllers());
    for (auto it (vect.begin()); it != vect.end();it++)
    {
        const MIDI::MIDI_Ctrl_Cfg& cfg(*it);
        if (dev == cfg.device)
        {
            string t1 ("Configuration of MIDI input:");
            t1 += cfg.name;
            res += WEB::toTitle2(t1);
        }
    }
    return res;
}

/*******************************************************************************/
std::string BasicWebSrv::pagePLAY(const WEB::ParamVect& params)
{
    using namespace std;

    const std::string idx(findParamValue(params,"idx"));
    if (idx != "")
    {
        try {
            const unsigned int trackid (std::stoi(idx));
            m_manager.selectIndex(trackid);
        } catch (...) {}
    }
    const std::string playpause(findParamValue(params,"playpause"));
    if (playpause != "")
    {
        m_manager.startReading();
    }

    std::string res(WEB::toTitle1("Active playlist."));
    res += WEB::toLink("Return to main page", "/");
    res += WEB::newline();
    res += WEB::toLink("Refresh", "/play");
    res += WEB::newline();

    const size_t nbFiles(m_manager.nbFiles());
    res += "<table border='1' bgcolor='silver'><thead><tr><th colspan='2'>";
    res += WEB::toTitle2(string ("Playlist :") + m_manager.title() +
            "(" + to_string(nbFiles) + " files)");

    res += "</th></tr></thead><tbody>";
    for (size_t i(0); i < nbFiles;i++)
    {
        res += "<tr><td>";
        res += WEB::toLink(m_manager.fileTitle(i),
                string("/play&idx=")+to_string(i));
        res += "</td></tr>";
    }

    res += "</tbody></table><BR>";

    if (m_manager.indexPlaying() < m_manager.nbFiles())
    {
        res += "Selected:";
        const std::string title (m_manager.fileTitle(m_manager.indexPlaying()));
        res += WEB::toBold(title);
        res += WEB::newline();
        res += WEB::toBold(WEB::toLink("Play/Pause","/play&playpause=1"));
    }
    return res;
} // pagePLAY

/*******************************************************************************/
std::string BasicWebSrv::pageTEST(const WEB::ParamVect& params)
{
    std::string res("TEST Page.\n");
    res += "Params=";
    for (auto it = params.begin(); it != params.end();it++)
    {
        const WEB::HTMLParam& p (*it);
        res += "[";
        res += p.name;
        res += "/";
        res += p.value;
        res += "]";
    }
    return res;
}

/*******************************************************************************/
std::string BasicWebSrv::findParamValue(
        const WEB::ParamVect& params,
        const std::string & name)
{
    for (auto it = params.begin(); it != params.end();it++)
    {
        const WEB::HTMLParam& p (*it);
        if (name == p.name) return p.value;
    }
    return "";
}


/*******************************************************************************
 * UTILS
 *******************************************************************************/
/*******************************************************************************/
using namespace std;
std::string toLink(const std::string& s,const std::string &hl)
{ return string ("<A href='") + hl + "'>" + s + "</A>";}
std::string toBold(const std::string& s) { return string ("<B>") + s + "</B>";}
std::string toTitle1(const std::string& s) { return string ("<H1>") + s + "</H1><BR>";}
std::string toTitle2(const std::string& s) { return string ("<H2>") + s + "</H2><BR>";}
} // namespace WEB
} // namespace PBKR

