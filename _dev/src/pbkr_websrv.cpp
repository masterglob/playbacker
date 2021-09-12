#include "pbkr_websrv.h"
#include "pbkr_config.h"
#include "pbkr_api.h"
#include "pbkr_projects.h"
#include "pbkr_menu.h"

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
    Thread("WebSrv-cmds"),
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
    start (false);
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

/*******************************************************************************/
void
WebSrv::body(void)
{
    static const std::string filename (std::string (WEB_ROOT_PATH) + "/cmd");
    static std::ifstream file(filename);
    std::string line;
    while (not isExitting())
    {
        if (std::getline(file, line))
        {
            process (line);
        }
        else
        {
            usleep(1000*100);
            file.close();
            file.open(filename);
        }
    }
} // WebSrv::body


/*******************************************************************************/
void
WebSrv::process(const std::string& line)
{
    const size_t slashPos (line.find("/"));
    const string cmd (substring (line, 0, slashPos));
    const string param (substring (line, slashPos + 1));

    /*printf ("WebSrv::process (\"%s\"). Cmd = %s, param = %s\n" ,
            line.c_str(),
            cmd.c_str(),
            param.c_str());*/

    if (cmd == "pPlay")
    {
        API::onPlayEvent();
    }
    else if (cmd == "pStop")
    {
        API::onStopEvent();
    }
    else if (cmd == "pBackward")
    {
        API::onBackward();
    }
    else if (cmd == "pFastForward")
    {
        API::onFastForward();
    }
    else if (cmd == "pRefresh")
    {
        // refresh all
        API::forceRefresh();
    }
    else if (cmd == "mtTrackSel")
    {
        try {
            const int tid(std::atoi (param.c_str())-1);
            API::onChangeTrack(tid);
        } catch (...) {
            printf("Invalid parameters in mtTrackSel.IGNORED\n");
        }
    }
    else if (cmd == "project")
    {
        ProjectVect list;
        getProjects (list, projectSourceInternal);
        try
        {
            const size_t pid(std::atoi (param.c_str())-1);
            if (pid < list.size()){
                Project* p(list[pid]);
                fileManager.loadProject(p);
            }
            else
            {
                printf ("Bad project Id:%u\n",pid);
            }
        }
        catch(...) {};
    }
    else if (cmd == "pUp")
    {
        globalMenu.pressKey(MainMenu::KEY_UP);
    }
    else if (cmd == "pDown")
    {
        globalMenu.pressKey(MainMenu::KEY_DOWN);
    }
    else if (cmd == "pLeft")
    {
        globalMenu.pressKey(MainMenu::KEY_LEFT);
    }
    else if (cmd == "pRight")
    {
        globalMenu.pressKey(MainMenu::KEY_RIGHT);
    }
    else if (cmd == "pOk")
    {
        globalMenu.pressKey(MainMenu::KEY_OK);
    }
    else if (cmd == "pClose")
    {
        globalMenu.pressKey(MainMenu::KEY_CANCEL);
    }
    else
    {
        printf ("WebSrv::process (\"%s\"). UNKNOWN  Cmd = %s, param = %s\n" ,
                    line.c_str(),
                    cmd.c_str(),
                    param.c_str());
    }
}

/*******************************************************************************
 * UTILS
 *******************************************************************************/
/*******************************************************************************/
} // namespace WEB
} // namespace PBKR

