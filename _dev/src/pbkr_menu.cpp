
#include <unistd.h>

#include "pbkr_menu.h"
#include "pbkr_cfg.h"
#include "pbkr_display.h"
#include "pbkr_projects.h"

namespace
{
using namespace PBKR;

// http://patorjk.com/software/taag/#p=display&f=Banner3&t=NET%20MENU

static const char* label_empty ("<EMPTY>");
static const std::string edit_me ("->");
/*******************************************************************************/
static const std::string addVertArrow(const std::string& title)
{
    char result[DISPLAY::DISPLAY_WIDTH+1];
    snprintf(result, DISPLAY::DISPLAY_WIDTH-2, title.c_str());

    size_t i;
    for (i = strlen(result); i < DISPLAY::DISPLAY_WIDTH-2;i++)
    {
        result[i]=' ';
    }
    if (i > DISPLAY::DISPLAY_WIDTH-2) i = DISPLAY::DISPLAY_WIDTH-2;
    result[i++] = 0x7F;
    result[i++] = 0x7E;
    result[i++] = 0;
    return result;
}

/*******************************************************************************/
struct NumParam
{
    NumParam (const std::string& name, const std::string& unit,
                        const int max_val,const int min_val,const int inc_val, const int val):
            m_name(name), m_unit(unit),
            m_max_val(max_val), m_min_val(min_val), m_inc_val(inc_val),m_val(val) {}
    const std::string m_name;
    const std::string m_unit;
    const int m_max_val;
    const int m_min_val;
    const int m_inc_val;
    int m_val;
    bool canEdit(void)const{return m_min_val < m_max_val;}
};

/*******************************************************************************/
struct FixedNumParam :NumParam
{
    FixedNumParam (const std::string& name,const std::string& unit,const int val):
            NumParam(name,unit, val,val,val,val){}
};

/*******************************************************************************
 * SUB MENUS
 *******************************************************************************/
struct PlayMenuItem : MenuItem
{
    PlayMenuItem(void): MenuItem("PLAY Menu"){}
    virtual ~PlayMenuItem(void){}
    virtual void onSelPressLong(void);
    virtual void onSelPressShort(void);
    virtual void onLeftRightPress(const bool isLeft);
    virtual const std::string menul1(void){return "";}
    virtual const std::string menul2(void){return "";}
};
PlayMenuItem playMenuItem;

MenuItem mainMenuItem("Main Menu", &playMenuItem);
MenuItem netMenuItem ("Network", &mainMenuItem);

/*******************************************************************************/
struct ListMenuItem : MenuItem
{
    ListMenuItem(const std::string & title, MenuItem* parent, const uint32_t maxItems);
    virtual ~ListMenuItem(void){}
    virtual void onLeftRightPress(const bool isLeft);
    virtual const std::string menul1(void);
protected:
    const uint32_t m_lrIdx_Max;
    uint32_t m_lrIdx;
};

/*******************************************************************************/
struct NetShowMenuItem : ListMenuItem
{
    NetShowMenuItem(const std::string & title, MenuItem* parent);
    virtual ~NetShowMenuItem(void){}
    virtual void onUpDownPress(const bool isUp);
    virtual const std::string menul1(void);
    virtual const std::string menul2(void);
private:
    uint32_t m_upDownIdx;
    uint32_t m_upDownIdxMax;
};
NetShowMenuItem netShowMenuItem ("Show config", &netMenuItem);

/*******************************************************************************/
struct SelectProjectShowMenuItem : ListMenuItem
{
    SelectProjectShowMenuItem(const std::string & title, MenuItem* parent);
    virtual ~SelectProjectShowMenuItem(void){}/*
    virtual void onUpDownPress(const bool isUp);
    virtual const std::string menul1(void);*/
    virtual const std::string menul2(void);
    virtual void onSelPressShort(void);
    void setDefaultProject(void);
private:
    uint32_t m_upDownIdx;
    uint32_t m_upDownIdxMax;
    string m_projectTitle;
    ProjectVect m_allproj;
    static const string m_saveSection;
};
const string SelectProjectShowMenuItem::m_saveSection("SelectProjectShowMenuItem");
SelectProjectShowMenuItem selectProjectShowMenuItem ("Select show", &mainMenuItem);


/*******************************************************************************/
struct ClicSettingsMenuItem : ListMenuItem
{
    ClicSettingsMenuItem(const std::string & title, MenuItem* parent);
    virtual ~ClicSettingsMenuItem(void){}
    virtual void onSelPressShort(void);
    virtual void onUpDownPress(const bool isUp);
    virtual const std::string menul1(void);
    virtual const std::string menul2(void);
private:
    static int paramToVolume(const int param){return (param * 127) /100;}
    void sendVolume(void);
    void setLatency(void);
    typedef enum {
        ID_VOLUME = 0,
        ID_LATENCY,
        ID_CHANN_L,
        ID_CHANN_R,
        ID_PRIM_NOTE,
        ID_SECN_NOTE,
        ID_COUNT
    } ItemId;
    NumParam m_pVolume;
    NumParam m_pLatency;
    NumParam m_pChannL;
    NumParam m_pChannR;
    NumParam m_pPriNote;
    NumParam m_pSecNote;
    NumParam* m_param[ID_COUNT];
};
ClicSettingsMenuItem clicSettingsMenuItem ("Clic settings", &mainMenuItem);


/*******************************************************************************
 *******************************************************************************
 * IMPLEMENTATIONS
 *******************************************************************************
 *******************************************************************************/

ListMenuItem::ListMenuItem(const std::string & title, MenuItem* parent, const uint32_t maxItems):
    MenuItem(title, parent),
    m_lrIdx_Max(maxItems),
    m_lrIdx(0)
{}

/*******************************************************************************/
const std::string ListMenuItem::menul1(void)
{
    return addVertArrow(subMenuName());
}

/*******************************************************************************/
void ListMenuItem::onLeftRightPress(const bool isLeft)
{
    if (isLeft)
    {
        if (m_lrIdx > 0)
            m_lrIdx--;
    }
    else
    {
        if (m_lrIdx + 1 < m_lrIdx_Max)
            m_lrIdx++;
    }
}


/*******************************************************************************


########  ##          ###    ##    ##    ##     ## ######## ##    ## ##     ##
##     ## ##         ## ##    ##  ##     ###   ### ##       ###   ## ##     ##
##     ## ##        ##   ##    ####      #### #### ##       ####  ## ##     ##
########  ##       ##     ##    ##       ## ### ## ######   ## ## ## ##     ##
##        ##       #########    ##       ##     ## ##       ##  #### ##     ##
##        ##       ##     ##    ##       ##     ## ##       ##   ### ##     ##
##        ######## ##     ##    ##       ##     ## ######## ##    ##  #######


 *******************************************************************************/
void PlayMenuItem::onSelPressShort(void)
{
    fileManager.startReading();
}

/*******************************************************************************/
void PlayMenuItem::onSelPressLong(void)
{
    globalMenu.setMenu(&mainMenuItem);
}

/*******************************************************************************/
void PlayMenuItem::onLeftRightPress(const bool isLeft)
{
    if (isLeft)
        fileManager.prevTrack();
    else
        fileManager.nextTrack();
}

/*******************************************************************************
 *
########  ########   #######        ## ########  ######  ########
##     ## ##     ## ##     ##       ## ##       ##    ##    ##
##     ## ##     ## ##     ##       ## ##       ##          ##
########  ########  ##     ##       ## ######   ##          ##
##        ##   ##   ##     ## ##    ## ##       ##          ##
##        ##    ##  ##     ## ##    ## ##       ##    ##    ##
##        ##     ##  #######   ######  ########  ######     ##

*******************************************************************************/
SelectProjectShowMenuItem::SelectProjectShowMenuItem (const std::string & title, MenuItem* parent)
:
        ListMenuItem(title, parent, 4),
        m_upDownIdx(0),
        m_upDownIdxMax(3),
        m_projectTitle("")
{
}

/*******************************************************************************/
void
SelectProjectShowMenuItem::setDefaultProject(void)
{
    m_projectTitle = Config::instance().loadStr(m_saveSection);
    m_allproj = getAllProjects();
    if (m_projectTitle.length() > 0)
    {
        printf("Trying to auto load previous show: '%s'\n",m_projectTitle.c_str());
        onSelPressShort();
    }
}

/*******************************************************************************/
const std::string
SelectProjectShowMenuItem::menul2(void)
{
    m_allproj = getAllProjects();
    if (m_lrIdx < m_allproj.size())
    {
        m_projectTitle = m_allproj[m_lrIdx]->m_title;
        return m_projectTitle;
    }
    m_projectTitle = "";
    return std::string("<EMPTY") + std::to_string(m_lrIdx + 1) +">";
}

/*******************************************************************************/
void
SelectProjectShowMenuItem::onSelPressShort(void)
{
    if (m_projectTitle.length() > 0)
    {
        Project* p(findProjectByTitle(m_allproj, m_projectTitle));
        if (p)
        {
            fileManager.loadProject(p);
            globalMenu.setMenu(&playMenuItem);
            Config::instance().saveStr(m_saveSection, m_projectTitle);
        }
        else
        {
            printf("Failed to load:%s\n",m_projectTitle.c_str());
        }
    }
}

/*******************************************************************************
 *

##    ## ######## ########    ##     ## ######## ##    ## ##     ##
###   ## ##          ##       ###   ### ##       ###   ## ##     ##
####  ## ##          ##       #### #### ##       ####  ## ##     ##
## ## ## ######      ##       ## ### ## ######   ## ## ## ##     ##
##  #### ##          ##       ##     ## ##       ##  #### ##     ##
##   ### ##          ##       ##     ## ##       ##   ### ##     ##
##    ## ########    ##       ##     ## ######## ##    ##  #######


*******************************************************************************/
NetShowMenuItem::NetShowMenuItem (const std::string & title, MenuItem* parent)
:
        ListMenuItem(title, parent, 4),
        m_upDownIdx(0),
        m_upDownIdxMax(3)
{}

/*******************************************************************************/
void NetShowMenuItem::onUpDownPress(const bool isUp)
{
    if (not isUp)
    {
        if (m_upDownIdx > 0) m_upDownIdx --;
    }
    else
    {
        if (m_upDownIdx + 1 < m_upDownIdxMax) m_upDownIdx ++;
    }
}

/*******************************************************************************/
const std::string NetShowMenuItem::menul1(void)
{
    switch (m_lrIdx)
    {
    case 0:
        switch (m_upDownIdx)
        {
        case 0:
            return addVertArrow("ETH / IP:");
        case 1:
            return addVertArrow("ETH / Netmsk:");
        default:
            break;
        }
        return addVertArrow ("ETH:");
    case 1:
        switch (m_upDownIdx)
        {
        case 0:
            return addVertArrow ("WIFI / IP:");
        case 1:
            return addVertArrow ("WIFI / Netmsk:");
        default:
            break;
        }
        return addVertArrow ("WIFI:");
    case 2:
        return "#3";
    case 3:
        return "#4";
    default:
        break;
    }
    return "##noitem##";
}

/*******************************************************************************/
const std::string NetShowMenuItem::menul2(void)
{
    switch (m_lrIdx)
    {
    case 0:
        switch (m_upDownIdx)
        {
        case 0:
            return getIPAddr(NET_DEV_ETH);
        case 1:
            return getIPNetMask(NET_DEV_ETH);
        default:
            break;
        }
        break;
    case 1:
        switch (m_upDownIdx)
        {
        case 0:
            return getIPAddr(NET_DEV_WIFI);
        case 1:
            return getIPNetMask(NET_DEV_WIFI);
        default:
            break;
        }
        break;
    default:
        break;
    }
    return "##noitem##";
}

/*******************************************************************************
 *
 ######  ##       ####  ######     ##     ## ######## ##    ## ##     ##
##    ## ##        ##  ##    ##    ###   ### ##       ###   ## ##     ##
##       ##        ##  ##          #### #### ##       ####  ## ##     ##
##       ##        ##  ##          ## ### ## ######   ## ## ## ##     ##
##       ##        ##  ##          ##     ## ##       ##  #### ##     ##
##    ## ##        ##  ##    ##    ##     ## ##       ##   ### ##     ##
 ######  ######## ####  ######     ##     ## ######## ##    ##  #######
*******************************************************************************/
ClicSettingsMenuItem::ClicSettingsMenuItem (const std::string & title, MenuItem* parent)
:
        ListMenuItem(title, parent, ID_COUNT),
         m_pVolume(NumParam("Volume", "", 100, 10, 10, 90)),
         m_pLatency(NumParam("Latency", "ms", 30, 0, 2, 2)),
         m_pChannL(FixedNumParam ("Left Chann.", "", 15)),
         m_pChannR(FixedNumParam ("Right Chann.", "", 15)),
         m_pPriNote(FixedNumParam ("Primary Note", "", 24)),
         m_pSecNote(FixedNumParam ("Second. Note", "", 25)),
        m_param{&m_pVolume, &m_pLatency,&m_pChannL,&m_pChannR, &m_pPriNote,&m_pSecNote}
{
    m_pVolume.m_val = Config::instance().loadInt("MIDI_VOLUME", 90);
    sendVolume ();
    m_pLatency.m_val = ( Config::instance().loadInt("MIDI_LATENCY", 2));
    setLatency ();
}

/*******************************************************************************/
void ClicSettingsMenuItem::onSelPressShort(void)
{
    onSelPressLong();
}

/*******************************************************************************/
void  ClicSettingsMenuItem::sendVolume(void)
{
    const uint8_t vol8 (paramToVolume(m_pVolume.m_val));

    MidiOutMsg msg;
    msg.push_back(vol8); // Volume value
    wemosControl.pushSysExMessage(WemosControl::SYSEX_COMMAND_VOLUME, msg);
}

/*******************************************************************************/
void  ClicSettingsMenuItem::setLatency(void)
{
    leftLatency.setMs(m_pLatency.m_val);
    rightLatency.setMs(m_pLatency.m_val);
}

/*******************************************************************************/
void ClicSettingsMenuItem::onUpDownPress(const bool isUp)
{
    if (! m_param[m_lrIdx]) return;
    NumParam& param(* m_param[m_lrIdx]);
    int& value (param.m_val);

    // update param
    if (isUp)
    {
        if (value + param.m_inc_val >= param.m_max_val)
            value = param.m_max_val;
        else
            value += param.m_inc_val;
    }
    else
    {
        if (value <= param.m_min_val + param.m_inc_val)
            value = param.m_min_val;
        else
            value -= param.m_inc_val;
    }

    // Processing
    switch (m_lrIdx)
    {
    case ID_VOLUME:
    {
        Config::instance().saveInt("MIDI_VOLUME", value);
        sendVolume ();
        break;
    }
    case ID_LATENCY:
        Config::instance().saveInt("MIDI_LATENCY", value);
        setLatency();
        break;
    default:
        break;
    }
}

/*******************************************************************************/
const std::string ClicSettingsMenuItem::menul1(void)
{
    NumParam* param( m_param[m_lrIdx]);
    if (!param) return label_empty;
    return (m_lrIdx < ID_COUNT ? addVertArrow (param->m_name) : label_empty);
}

/*******************************************************************************/
const std::string ClicSettingsMenuItem::menul2(void)
{
    NumParam* param( m_param[m_lrIdx]);
    if (!param) return label_empty;
    return (param->canEdit() ? edit_me : std::string (""))
            + std::to_string(param->m_val) + param->m_unit;
}

/*******************************************************************************
 *
##     ## ######## #### ##        ######
##     ##    ##     ##  ##       ##    ##
##     ##    ##     ##  ##       ##
##     ##    ##     ##  ##        ######
##     ##    ##     ##  ##             ##
##     ##    ##     ##  ##       ##    ##
 #######     ##    #### ########  ######

*******************************************************************************/

} // namespace


namespace PBKR
{

/*******************************************************************************


##     ## ######## ##    ## ##     ##    #### ######## ######## ##     ##
###   ### ##       ###   ## ##     ##     ##     ##    ##       ###   ###
#### #### ##       ####  ## ##     ##     ##     ##    ##       #### ####
## ### ## ######   ## ## ## ##     ##     ##     ##    ######   ## ### ##
##     ## ##       ##  #### ##     ##     ##     ##    ##       ##     ##
##     ## ##       ##   ### ##     ##     ##     ##    ##       ##     ##
##     ## ######## ##    ##  #######     ####    ##    ######## ##     ##


 *******************************************************************************/
MenuItem::MenuItem (const std::string& title, MenuItem* parent):
        name(title),m_parent(parent),m_iter(m_subMenus.end())
{
    // printf("MenuItem:%s\n",title.c_str());
    if (m_parent)
    {
        m_parent->m_subMenus.push_back(this);
        m_parent->m_iter = m_parent->m_subMenus.begin();
    }
}

/*******************************************************************************/
void MenuItem::onLeftRightPress(const bool isLeft)
{
    if (m_iter == m_subMenus.end()) return;
    if (isLeft)
    {
        if (m_iter != m_subMenus.begin())
            m_iter--;
        else
            m_iter = m_subMenus.end() - 1;
    }
    else
    {
        m_iter++;
        if (m_iter == m_subMenus.end()) m_iter = m_subMenus.begin();
    }
}

/*******************************************************************************/
void MenuItem::onSelPressShort(void)
{
    if (m_iter != m_subMenus.end())
    {
        MenuItem* menuItem (*m_iter);
        globalMenu.setMenu(menuItem);
    }
}

/*******************************************************************************/
void MenuItem::onSelPressLong(void)
{
    if (m_parent)
    {
        globalMenu.setMenu(m_parent);
    }
}

/*******************************************************************************/
const std::string MenuItem::menul1(void)
{
    if (m_iter != m_subMenus.end() && m_subMenus.size() > 1)
    {
        return addVertArrow(subMenuName());
    }
    return subMenuName();
}

/*******************************************************************************/
const std::string MenuItem::menul2(void)
{
    if (m_iter != m_subMenus.end())
    {
        MenuItem* menuItem (*m_iter);
        return std::string ("<") + menuItem->subMenuName() + ">";
    }
    return label_empty;
}

MainMenu& globalMenu(MainMenu::instance());

/*******************************************************************************
 * MAIN MENU CONFIG
 *******************************************************************************/
static const float MAX_BTN_WAIT (1.0);
/*******************************************************************************/
MenuCfg::MenuCfg(void):
                leftGpio (GPIOs::GPIO_27_PIN13,MAX_BTN_WAIT,   "LEFT MENU"),
                rightGpio (GPIOs::GPIO_22_PIN15,MAX_BTN_WAIT,  "RIGHT MENU"),
                upGpio (GPIOs::GPIO_24_PIN18,MAX_BTN_WAIT,     "UP MENU"),
                downGpio (GPIOs::GPIO_23_PIN16,MAX_BTN_WAIT,   "DOWN MENU"),
                selectGpio (GPIOs::GPIO_17_PIN11,MAX_BTN_WAIT, "SEL MENU")
{}

/*******************************************************************************
 * MAIN MENU
 *******************************************************************************/

/*******************************************************************************
 *MENU MANAGER
 *******************************************************************************/

/*******************************************************************************/
MainMenu::MainMenu(void):
        Thread("MainMenu"),
        m_cfg (MenuCfg()),
        m_currentMenu(&::playMenuItem)
{
    Thread::start();
} // MainMenu::MainMenu

/*******************************************************************************/
MainMenu& MainMenu::instance (void)
{
    static MainMenu ins;
    return ins;
} // MainMenu::instance

/*******************************************************************************/
void MainMenu::setMenu(MenuItem* menu)
{
    m_currentMenu = menu;
    printf("New menu => %s\n",m_currentMenu->name.c_str());
} // MainMenu::setMenu

/*******************************************************************************/
void MainMenu::body(void)
{
    while (not isExitting())
    {
        float duration(0.0);
        usleep(100*1000);
        if (m_cfg.leftGpio.pressed(duration))
        {
            m_currentMenu->onLeftRightPress(true);
        }
        if (m_cfg.rightGpio.pressed(duration))
        {
            m_currentMenu->onLeftRightPress(false);
        }

        if (m_cfg.upGpio.pressed(duration))
        {
            m_currentMenu->onUpDownPress(true);
        }
        if (m_cfg.downGpio.pressed(duration))
        {
            m_currentMenu->onUpDownPress(false);
        }

        if (m_cfg.selectGpio.pressed(duration))
        {
            if (duration > MAX_BTN_WAIT)
            {
                m_currentMenu->onSelPressLong();
            }
            else
            {
                m_currentMenu->onSelPressShort();
            }
        }
    }
} // MainMenu::body

/*******************************************************************************/
void setDefaultProject(void)
{
    selectProjectShowMenuItem.setDefaultProject();
} // setDefaultProject


} // namespace PBKR

