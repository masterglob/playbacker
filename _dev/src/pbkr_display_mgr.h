#pragma once

#include <inttypes.h>
#include <stdint.h>
#include <string>
#include <thread>
#include <vector>
#include <mutex>

#include "pbkr_config.h"
#include "pbkr_display.h"
#include "pbkr_utils.h"

namespace PBKR
{
namespace DISPLAY
{

static const int WARNING_DISPLAY_SEC(3);
static const int INFO_DISPLAY_SEC(2);
static const int MAX_NB_TRACKS (64);
static const int MAX_NB_PROJECTS (10);

/*******************************************************************************
 *   Property
 *******************************************************************************/
class Property
{
public:
    Property (const std::string& name, const std::string & value);
    void set (const std::string & value);
    inline std::string get (void)const{return mValue;}
    void refresh(void)const;
private:
    const std::string mName;
    std::string mValue;
};

/*******************************************************************************
 *   DisplayManager
 *******************************************************************************/
/**
 * Possible statuses:
 *   - NO USB Key
 *   - USB Key:
 *     - No files
 *     - Files:
 *       - Title? (Y/N)
 *       - No Track selected
 *       -
 */
class DisplayManager : Thread
{
public:
    static DisplayManager& instance(void);
    enum Event {evBegin,evEnd,
        evProjectTitle, evProjectTrackCount,
        evUsbIn, evUsbOut,
        evPlay, evStop, evPause,
        evTrack, evFile, evRefresh};
    void info (const std::string& msg);
    void warning (const std::string& msg);
    void onEvent (const Event e, const std::string& param="");
    void setFilename (const std::string& filename);
    void setProjectTitle (const std::string& title);
    void setTrackName (const std::string& name, size_t trackIdx);
    void setTimeCode(const string & timecode);
    void forceRefresh(void);
    uint32_t printIdx(void)const{return m_printIdx;}
    void updateProjectList(void);
    void updateMenu(void);
private:
    DisplayManager(void);
    virtual ~DisplayManager(void);
    void refresh(void);
    virtual void body(void);
    I2C_Display m_display;
    bool m_running;
    bool m_ready;
    volatile uint32_t m_printIdx;
    bool m_isInfo;
    volatile bool m_canEvent;
    std::string m_warning;
    std::string m_line1;
    std::string m_line2;
    Property m_title;
    Property m_lMessage;
    Property m_lTrack;
    Property m_trackIdx;
    Property m_timecode;
    Property m_lPlayStatus;
    Property m_lBuild;
    Property m_lVersion;
    Property m_lMenuL1;
    Property m_lMenuL2;
    typedef std::vector<Property> Properties;
    Properties m_trackName;
    Properties m_projectNames;
    std::string m_event;
    std::string m_filename;
    std::string m_trackCount;
    bool m_reading;
    bool m_pause;
    std::mutex m_mutex;
}; // class

}  // namespace DISPLAY
}
