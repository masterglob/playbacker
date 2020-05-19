#include "pbkr_webserv.h"
#include "pbkr_config.h"

#include <stdio.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <sstream>

namespace
{

bool checkDir(const std::string &path)
{
  struct stat buffer;
  return (stat (path.c_str(), &buffer) == 0);
}
}

namespace PBKR
{
namespace WEB
{
WebSrv& webInstance (WebSrv::instance());

/*******************************************************************************/
WebSrv::WebSrv():
    mWebResPath(std::string (WEB_ROOT_PATH) + "/res")
{
    if (! checkDir (mWebResPath))
    {
        mkdir(mWebResPath.c_str(), 0777);
        if (! checkDir (mWebResPath))
        {
            printf ("Failed to create  WEB path : %s\n", mWebResPath.c_str());
        }
    }
}

/*******************************************************************************/
WebSrv::~WebSrv(void)
{
}

/*******************************************************************************/
WebSrv &
WebSrv::instance (void)
{
    static WebSrv srv;
    return srv;
}



/*******************************************************************************/
const std::string
WebSrv::toSavePath(const std::string& name)
{
    return mWebResPath + "/" + name;
}

/*******************************************************************************/
void
WebSrv::setValue(const std::string& name, const std::string& value)
{
    std::ofstream myfile;
    myfile.open (toSavePath(name));
    myfile << value;
    myfile.close();
} // WebSrv::setValue


/*******************************************************************************
 * UTILS
 *******************************************************************************/
/*******************************************************************************/
} // namespace WEB
} // namespace PBKR

