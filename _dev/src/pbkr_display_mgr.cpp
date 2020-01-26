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

// #define DEBUG_DISPLAY printf
#define DEBUG_DISPLAY(...)

namespace PBKR
{
namespace DISPLAY
{

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
        m_title( "Unnamed project"),
        m_event(""),
        m_filename(""),
        m_trackIdx(""),
        m_trackCount(""),
        m_reading(false)
{
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
    std::string l1 (m_title);
    std::string l2 ("");
    const uint32_t idx(m_printIdx%10);
    if (m_warning != "")
    {
        l2 = m_warning;
        if (m_isInfo)
        {
            l1 ="Info:";
            if (m_printIdx >= INFO_DISPLAY_SEC)
            {
                m_warning = "";
                if (OSC::p_osc_instance)
                    OSC::p_osc_instance->sendLabelMessage("");
            }
        }
        else
        {
            l1 ="Error:";
            if (m_printIdx >= WARNING_DISPLAY_SEC)
            {
                m_warning = "";
                if (OSC::p_osc_instance)
                    OSC::p_osc_instance->sendLabelMessage("");
            }
        }
    }
    else if (m_reading)
    {
        if (idx < 3)
        {
            l1 = m_title;
        }
        else if (idx <6)
            l1 = std::string("Track ") + m_trackIdx + "/" + m_trackCount;
        else
            l1 ="Reading...";
        const char scroll[4] = {'/', '-', '/', '|'};
        l2 = m_filename + (scroll[m_printIdx % sizeof(scroll)]);
    }
    else
    {
        if (m_filename.length() >0)
        {
            if (idx < 4)
                l2 = std::string("Track ") + m_trackIdx + "/" + m_trackCount;
            else if (idx < 6)
                l2 = m_filename;
            else
                l2 = "Stopped";
        }
        else
        {
            if (idx < 2)
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
    if (m_line1 != l1 || m_line2 != l2)
    {
        m_line1 = l1;
        m_line2 = l2;
        m_display.clear();
        m_display.setCursor(0,0);
        m_display.print(l1.c_str());
        m_display.setCursor(0, 1);
        m_display.print(l2.c_str());
        if (OSC::p_osc_instance) OSC::p_osc_instance->setMenuTxt(l1, l2);
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
    if (OSC::p_osc_instance)
        OSC::p_osc_instance->sendLabelMessage(std::string ("Info:" + msg));
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
    if (OSC::p_osc_instance)
        OSC::p_osc_instance->sendLabelMessage(std::string ("Warning:" + msg));
    refresh();
    m_mutex.unlock();
} // DisplayManager::warning

/*******************************************************************************/
void  DisplayManager::setTrackName (const std::string& name, size_t trackIdx)
{
    if (trackIdx < MAX_NB_TRACKS)
    {
            m_trackNames[trackIdx] = name;
        if (OSC::p_osc_instance)
        {
            OSC::p_osc_instance->setTrackName(name, trackIdx);
        }
    }
} // DisplayManager::setTrackName

/*******************************************************************************/
void DisplayManager::forceRefresh(void)
{
    if (OSC::p_osc_instance)
    {
        OSC::p_osc_instance->setProjectName(m_title);
        OSC::p_osc_instance->setFileName(m_filename);
        OSC::p_osc_instance->setActiveTrack(atoi (m_trackIdx.c_str())-1);
        for (size_t i(0); i< MAX_NB_TRACKS;i++)
        {
            OSC::p_osc_instance->setTrackName(m_trackNames[i], i);
        }
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
        if (OSC::p_osc_instance)
        {
            OSC::p_osc_instance->setProjectName("");
            OSC::p_osc_instance->setFileName("");
            OSC::p_osc_instance->sendLabelMessage("Connected!");
            OSC::p_osc_instance->setPbCtrlStatus(false);
            if (OSC::p_osc_instance)
                OSC::p_osc_instance->setActiveTrack(-1);
            for (size_t i(0); i< OSC::NB_OSC_TRACK ;++i)
                OSC::p_osc_instance->setTrackName("", i);
        }
        for (size_t i(0); i< MAX_NB_TRACKS;i++)
        {
            m_trackNames[i] = "";
        }
        m_title = "No project";
        m_event = "Starting...";
        m_filename = "";
        m_ready = true;
        break;
    case evEnd:
        m_display.clear();
        m_display.noBacklight();
        m_display.noDisplay();
        m_display.noCursor();
        if (OSC::p_osc_instance)
        {
            OSC::p_osc_instance->sendLabelMessage("Disconnected...");
            OSC::p_osc_instance->setProjectName("");
            OSC::p_osc_instance->setFileName("");
            OSC::p_osc_instance->setPbCtrlStatus(false);
            if (OSC::p_osc_instance)
                OSC::p_osc_instance->setActiveTrack(-1);
            for (size_t i(0); i< OSC::NB_OSC_TRACK ;++i)
                OSC::p_osc_instance->setTrackName("", i);
        }
        m_title = "";
        m_event = "Exiting...";
        m_filename = "";
        for (size_t i(0); i< MAX_NB_TRACKS;i++)
        {
            m_trackNames[i] = "";
        }
        break;
    case evProjectTrackCount:
        m_trackCount = param;
        break;
    case evUsbIn:
        m_title = "";
        m_event = "Reading USB...";
        m_filename = "";
        for (size_t i(0); i< MAX_NB_TRACKS;i++)
        {
            m_trackNames[i] = "";
        }
        if (OSC::p_osc_instance)
        {
            OSC::p_osc_instance->setProjectName("...reading USB...");
            OSC::p_osc_instance->setFileName("...reading USB...");
            for (size_t i(0); i< OSC::NB_OSC_TRACK ;++i)
                OSC::p_osc_instance->setTrackName("", i);
        }
        break;
    case evUsbOut:
        m_title = "No USB key";
        m_event = "USB unplugged!";
        m_filename = "";
        m_trackCount = "";
        m_trackIdx = "";
        for (size_t i(0); i< MAX_NB_TRACKS;i++)
        {
            m_trackNames[i] = "";
        }
        if (OSC::p_osc_instance) OSC::p_osc_instance->setProjectName("No USB Key...");
        for (size_t i(0); i< OSC::NB_OSC_TRACK ;++i)
            OSC::p_osc_instance->setTrackName("", i);
        break;
    case evProjectTitle:
        m_title = param;
        if (OSC::p_osc_instance)
            OSC::p_osc_instance->setProjectName(m_title);
        break;
    case evTrack:
        m_trackIdx = param;
        try
        {
            if (OSC::p_osc_instance)
                OSC::p_osc_instance->setActiveTrack(atoi (m_trackIdx.c_str())-1);
        } catch (...) {}
        break;
    case evPlay:
        m_event = "Reading...";
        m_reading = true;
        if (OSC::p_osc_instance) OSC::p_osc_instance->setPbCtrlStatus(true);
        break;
    case evStop:
        m_event = "Stopped...";
        m_reading = false;

        if (OSC::p_osc_instance)
            OSC::p_osc_instance->setPbCtrlStatus(false);
        break;
    case evFile:
        m_event = std::string ("Reading ") + param;
        m_filename = param;
        if (OSC::p_osc_instance)
            OSC::p_osc_instance->setFileName(m_filename);
        break;
    default:
        break;
    }
    m_warning = "";
    if (OSC::p_osc_instance)
        OSC::p_osc_instance->sendLabelMessage("");
    m_printIdx = 0;
    refresh();
    m_mutex.unlock();
} // DisplayManager::onEvent


} // namespace DISPLAY
} // namespace PBKR
