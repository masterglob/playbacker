#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdexcept>

#include "pbkr_menu.h"
#include "pbkr_display_mgr.h"
#include "pbkr_osc.h"
#include "pbkr_api.h"
#include "pbkr_websrv.h"
#include "pbkr_projects.h"

// #define DEBUG_DISPLAY printf
#define DEBUG_DISPLAY(...)

namespace PBKR
{
namespace DISPLAY
{

/*******************************************************************************
 *   Property
 *******************************************************************************/
Property::
Property (const std::string& name, const std::string & value):
    mName(name),
    mValue(value)
{
    refresh();
}

/*******************************************************************************/
void Property::set (const std::string & value)
{
    mValue = value;
    refresh();
}

void Property::refresh(void)const
{
    static WEB::WebSrv& web (WEB::WebSrv::instance());
    web.setValue (mName, mValue);
    if (OSC::p_osc_instance) OSC::p_osc_instance->setProperty(mName, mValue);
}
/*******************************************************************************
 *  DisplayManager
 *******************************************************************************/

DisplayManager& DisplayManager::instance(void)
{
    static DisplayManager singleton;
    return singleton;
}

/*******************************************************************************/
DisplayManager::DisplayManager(void):
        Thread("DisplayManager"),
        m_display(DISPLAY_I2C_ADDRESS),
        m_running(true),
        m_ready(false),
        m_printIdx(0),
        m_isInfo(true),
        m_canEvent(false),
        m_title("projectName", "Unnamed project"),
        m_lMessage("lMessage", " "),
        m_lTrack("lTrack",""),
        m_trackIdx("lTrackIdx",""),
        m_timecode("timecode",""),
        m_lPlayStatus("lPlayStatus",""),
        m_lBuild("lBuild", std::to_string(PBKR_BUILD_ID)),
        m_lVersion("lVersion",PBKR_VERSION),
        m_lMenuL1("lMenuL1",globalMenu.menul1()),
        m_lMenuL2("lMenuL2",globalMenu.menul2()),
        m_event(""),
        m_filename(""),
        m_trackCount(""),
        m_reading(false),
        m_pause(false)
{
    static const std::string lTrack("lTrack");
    static const std::string projectName("project");
    for (int i = 0 ; i < MAX_NB_TRACKS ; i++)
    {
        const std::string name(lTrack + std::to_string(m_trackName.size()+1));
        m_trackName.push_back(Property(name,""));
        // printf("Create (%d => %s)\n",m_trackName.size(), name.c_str());
    }
    for (int i = 0 ; i < MAX_NB_PROJECTS ; i++)
    {
        const std::string name(projectName + std::to_string(m_projectNames.size()));
        m_projectNames.push_back(Property(name,""));
        // printf("Create (%d => %s)\n",m_trackName.size(), name.c_str());
    }


    Thread::start();
}

/*******************************************************************************/
DisplayManager::~DisplayManager(void)
{
    m_running = false;
}

/*******************************************************************************/
void DisplayManager::body(void)
{
    while (m_running)
    {
        for (int i(0);(i<6) && m_running ;i++)
        {
            usleep(1000*100);
            m_mutex.lock();
            m_canEvent = true;
            m_mutex.unlock();
        }
        m_mutex.lock();
        m_printIdx++;
        refresh();
        m_mutex.unlock();
    }
}

/*******************************************************************************/
void DisplayManager::refresh(void)
{
    if (!(m_ready&&m_canEvent)) return;
    std::string l1 (m_title.get());
    std::string l2 ("");
    const uint32_t idx(m_printIdx%20);
    if (m_warning != "")
    {
        l2 = m_warning;
        if (m_isInfo)
        {
            l1 ="Info:";
            if (m_printIdx >= INFO_DISPLAY_SEC)
            {
                m_warning = "";
                m_lMessage.set ("");
            }
        }
        else
        {
            l1 ="Error:";
            if (m_printIdx >= WARNING_DISPLAY_SEC)
            {
                m_warning = "";
                m_lMessage.set ("");
            }
        }
    }
    else if (m_reading)
    {
        if (m_pause)
        {
            l1 = m_title.get();
            l2 = "Paused...";
        }
        else
        {
            if (idx < 8)
            {
                l1 = m_title.get();
            }
            else if (idx <12)
                l1 = std::string("Track ") + m_trackIdx.get() + "/" + m_trackCount;
            else
                l1 ="Reading...";
            const char scroll[4] = {'/', '-', '/', '|'};
            l2 = m_filename + (scroll[m_printIdx % sizeof(scroll)]);
        }
    }
    else
    {
        if (m_filename.length() >0)
        {
            if (idx < 8)
                l2 = std::string("Track ") + m_trackIdx.get() + "/" + m_trackCount;
            else if (idx < 12)
                l2 = m_filename;
            else
                l2 = "Stopped";
        }
        else
        {
            if (idx < 6)
                l2 = "No sel. track";
            else
                l2 = std::string("Track -") + "/" + m_trackCount;
        }
    }

    const std::string l1_menu(globalMenu.menul1());
    if (l1_menu != "")
    {
        l1 = l1_menu;
    }
    const std::string l2_menu(globalMenu.menul2());
    if (l2_menu != "")
    {
        l2 = l2_menu;
    }

    if (OSC::p_osc_instance) OSC::p_osc_instance->setMenuTxt(l1, l2);
    if (m_line1 != l1 || m_line2 != l2)
    {
        m_line1 = l1;
        m_line2 = l2;
        m_display.clear();
        m_display.setCursor(0,0);
        m_display.print(l1.c_str());
        m_display.setCursor(0, 1);
        m_display.print(l2.c_str());
        updateMenu();
    }
    m_canEvent = false;
} // DisplayManager::refresh

/*******************************************************************************/
void DisplayManager::info (const std::string& msg)
{
    if (m_warning == msg) return ;

    m_mutex.lock();
    m_warning = msg;
    m_isInfo = true;
    m_printIdx = 0;
    m_lMessage.set (std::string ("Info:" + msg));
    refresh();

    m_mutex.unlock();
} // DisplayManager::info

/*******************************************************************************/
void DisplayManager::warning (const std::string& msg)
{
    if (m_warning == msg) return ;

    m_mutex.lock();
    m_warning = msg;
    m_isInfo = false;
    m_printIdx = 0;
    m_lMessage.set (std::string ("Warning:" + msg));
    refresh();
    m_mutex.unlock();
} // DisplayManager::warning

/*******************************************************************************/
void  DisplayManager::setTrackName (const std::string& name, size_t trackIdx)
{
    // printf("DisplayManager::setTrackName (%s, %d)\n", name.c_str(), trackIdx);
    if (trackIdx < MAX_NB_TRACKS)
    {
        // printf("Set (%d => %s)\n",trackIdx,name.c_str());
        m_trackName[trackIdx].set(substring (name, 0, 13));
    }
} // DisplayManager::setTrackName

/*******************************************************************************/
void DisplayManager::setTimeCode(const string & timecode)
{
    const std::string prev (m_timecode.get());
    if (prev != timecode)
        m_timecode.set(timecode);
}


/*******************************************************************************/
void DisplayManager::updateMenu(void)
{
    m_lMenuL1.set(globalMenu.menul1());
    m_lMenuL2.set(globalMenu.menul2());
}

/*******************************************************************************/
void DisplayManager:: updateProjectList(void)
{
    ProjectVect list;
    getProjects (list, projectSourceInternal);
    const string currName(fileManager.title());
    int idx(0);
    FOR (it, list)
    {
        idx++;
        const Project* p(*it);
        const string color (currName == p->m_title ? "green" : "gray");

        m_projectNames[idx].set(p->m_title);
        //setColor (name, color);
        //setVisible (name, true);
    }
    for (idx++; idx <= 10 ; idx ++)
    {
        m_projectNames[idx].set("");
        //send (OSC_Msg_To_Send (name, "<Empty>"));
        //setVisible (name, false);
    }
} // DisplayManager::updateProjectList


/*******************************************************************************/
void DisplayManager::forceRefresh(bool full)
{
    m_title.refresh();
    m_lTrack.set(m_filename);

    onEvent(evRefresh);

    if (full) {
        updateProjectList();
        FOR (iter, m_trackName){
            Property& p(*iter);
            p.refresh();
        }
        FOR (iter, m_projectNames){
            Property& p(*iter);
            p.refresh();
        }
    }

    if (OSC::p_osc_instance)
    {
        OSC::p_osc_instance->setActiveTrack(atoi (m_trackIdx.get().c_str())-1);
    }
}

/*******************************************************************************/
void DisplayManager::onEvent (const Event e, const std::string& param)
{
    m_mutex.lock();
    switch (e) {
    case evBegin:
        m_display.begin();
        m_display.backlight();
        m_display.noBlink();
        m_display.noCursor();

        m_lMessage.set ("Connected!");
        m_lPlayStatus.set ("Stopped");
        m_lTrack.set("");
        if (OSC::p_osc_instance)
        {
            OSC::p_osc_instance->setProjectName("");
            if (OSC::p_osc_instance)
                OSC::p_osc_instance->setActiveTrack(-1);
        }
        FOR  (iter,m_trackName) {  (*iter).set (""); }
        FOR  (iter,m_projectNames) {  (*iter).set (""); }
        m_title.set ("No project");
        m_event = "Starting...";
        m_filename = "";
        m_ready = true;
        break;
    case evEnd:
        m_display.clear();
        m_display.noBacklight();
        m_display.noDisplay();
        m_display.noCursor();
        m_lMessage.set ("Disconnected...");
        m_timecode.set ("");
        m_lTrack.set("");
        if (OSC::p_osc_instance)
        {
            OSC::p_osc_instance->setProjectName("");
            if (OSC::p_osc_instance)
                OSC::p_osc_instance->setActiveTrack(-1);
        }
        m_title.set ("");
        m_lPlayStatus.set ("Disconnected");
        m_timecode.set ("");
        m_event = "Exiting...";
        m_filename = "";
        m_reading = false;
        FOR  (iter,m_trackName) {  (*iter).set (""); }
        FOR  (iter,m_projectNames) {  (*iter).set (""); }
        break;
    case evProjectTrackCount:
        m_trackCount = param;
        for (size_t i(0); i< MAX_NB_TRACKS;i++)
        {
            m_trackName[i].set ("");
        }
        break;
    case evUsbIn:
        m_title.set ("");
        m_event = "Reading USB...";
        m_filename = "";
        m_lTrack.set("...reading USB...");
        if (OSC::p_osc_instance)
        {
            OSC::p_osc_instance->setProjectName("...reading USB...");
        }
        for (size_t i(0); i< OSC::NB_OSC_TRACK ;++i)
            setTrackName("", i);
        break;
    case evUsbOut:
        m_title.set ("No USB key");
        m_event = "USB unplugged!";
        m_filename = "";
        m_trackCount = "";
        m_trackIdx.set("");
        for (size_t i(0); i< MAX_NB_TRACKS;i++)
        {
            m_trackName[i].set ("");
        }
        if (OSC::p_osc_instance) OSC::p_osc_instance->setProjectName("No USB Key...");
        for (size_t i(0); i< OSC::NB_OSC_TRACK ;++i)
            setTrackName("", i);
        break;
    case evProjectTitle:
        m_title.set (param);
        break;
    case evTrack:
        m_trackIdx.set(param);
        try
        {
            if (OSC::p_osc_instance)
                OSC::p_osc_instance->setActiveTrack(atoi (m_trackIdx.get().c_str())-1);
        } catch (...) {}
        break;
    case evPause:
        m_event = "Paused...";
        m_lPlayStatus.set ("Paused...");
        m_pause = true;
        break;
    case evPlay:
        m_event = "Reading...";
        m_lPlayStatus.set ("Reading...");
        m_reading = true;
        m_pause = false;
        break;
    case evStop:
        m_event = "Stopped...";
        m_lPlayStatus.set ("Stopped");
        m_reading = false;
        m_pause = false;
        break;
    case evFile:
        m_event = std::string ("Reading ") + param;
        m_filename = param;
        m_lTrack.set(m_filename);
        break;
    case evRefresh:
        if (OSC::p_osc_instance)
            OSC::p_osc_instance->setActiveTrack(atoi (m_trackIdx.get().c_str())-1);
        break;
    default:
        break;
    }
    if (OSC::p_osc_instance)

    m_warning = "";
    if (OSC::p_osc_instance)
    {
        OSC::p_osc_instance->setPbCtrlStatus(m_reading, m_pause);
        m_lMessage.set ("");
    }
    m_printIdx = 0;
    refresh();
    m_mutex.unlock();
} // DisplayManager::onEvent


} // namespace DISPLAY
} // namespace PBKR
