#ifndef I_pbkr_webserv_h_I
#define I_pbkr_webserv_h_I

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>

namespace PBKR
{

namespace WEB
{
/*******************************************************************************
 * GLOBAL CONSTANTS
 *******************************************************************************/

/*******************************************************************************
 * WEB SERVER
 *******************************************************************************/
class WebSrv
{
public:
    static WebSrv & instance (void);
    virtual ~WebSrv(void);
    void setValue(const std::string& name, const std::string& value);
private:
    WebSrv ();
    const std::string mResPath;
    const std::string mWebResPath;
    const std::string toSavePath(const std::string& name);
};



/*******************************************************************************
 * UTILS
 *******************************************************************************/
} // namespace WEB
} // namespace PBKR

#endif // I_pbkr_webserv_h_I
