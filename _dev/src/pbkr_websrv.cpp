#include "pbkr_webserv.h"

#include <stdio.h>
#include <arpa/inet.h>
#include <sys/time.h>

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
SockListener::SockListener(WebSrv* srv, int id):m_srv(srv),
        m_fd(-1)
{
    m_thread=std::thread ([this,id](){this->do_listen(id);});
}

/*******************************************************************************/
SockListener::~SockListener(void)
{
}


/*******************************************************************************/
void SockListener::do_listen(int id)
{
    while (1)
    {
        struct sockaddr_in cli_addr;
        socklen_t clilen (sizeof(cli_addr));
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

