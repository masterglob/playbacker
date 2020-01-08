
#include <unistd.h>

#include "pbkr_menu.h"
#include "pbkr_display.h"

namespace
{
using namespace PBKR;

// http://patorjk.com/software/taag/#p=display&f=Banner3&t=NET%20MENU

static const char* label_empty ("<EMPTY>");
static const std::string edit_me ("->");
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
    virtual const std::string menul1(void)const{return "";}
    virtual const std::string menul2(void)const{return "";}
};
PlayMenuItem playMenuItem;

MenuItem mainMenuItem("Main Menu", &playMenuItem);
MenuItem netMenuItem ("Network", &mainMenuItem);

struct ListMenuItem : MenuItem
{
    ListMenuItem(const std::string & title, MenuItem* parent, const uint32_t maxItems);
    virtual ~ListMenuItem(void){}
    virtual void onLeftRightPress(const bool isLeft);
protected:
    const uint32_t m_lrIdx_Max;
    uint32_t m_lrIdx;
};

struct NetShowMenuItem : ListMenuItem
{
    NetShowMenuItem(const std::string & title, MenuItem* parent);
    virtual ~NetShowMenuItem(void){}
    virtual void onUpDownPress(const bool isUp);
    virtual const std::string menul1(void)const;
    virtual const std::string menul2(void)const;
private:
    uint32_t m_upDownIdx;
    uint32_t m_upDownIdxMax;
};
NetShowMenuItem netShowMenuItem ("Show config", &netMenuItem);


struct ClicSettingsMenuItem : ListMenuItem
{
    ClicSettingsMenuItem(const std::string & title, MenuItem* parent);
    virtual ~ClicSettingsMenuItem(void){}
    virtual void onUpDownPress(const bool isUp);
    virtual const std::string menul1(void)const;
    virtual const std::string menul2(void)const;
private:
    typedef enum {
        ID_VOLUME = 0,
        ID_CHANN_L,
        ID_CHANN_R,
        ID_PRIM_NOTE,
        ID_SECN_NOTE,
        ID_COUNT
    } ItemId;
    uint32_t m_upDownIdx;
    uint32_t m_upDownIdxMax;
    uint32_t m_volume;
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

void PlayMenuItem::onSelPressLong(void)
{
    globalMenu.setMenu(&mainMenuItem);
}

void PlayMenuItem::onLeftRightPress(const bool isLeft)
{
    if (isLeft)
        fileManager.prevTrack();
    else
        fileManager.nextTrack();
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

const std::string NetShowMenuItem::menul1(void)const
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

const std::string NetShowMenuItem::menul2(void)const
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
        m_upDownIdx(0),
        m_upDownIdxMax(3),
        m_volume(80)
{}

void ClicSettingsMenuItem::onUpDownPress(const bool isUp)
{
    switch (m_lrIdx)
    {
    case ID_VOLUME:
    {
        static const size_t increment (10);
        static const size_t min_vol (10);
        if (isUp)
        {
            if (m_volume + increment >= 100)
                m_volume = 100;
            else
                m_volume +=increment;
        }
        else
        {
            if (m_volume <= min_vol + increment)
                m_volume = min_vol;
            else
                m_volume -= increment;
        }
        const uint8_t vol8 ((m_volume * 127) /100);

        MidiOutMsg msg; // TODO : encapsulate!
        msg.push_back(0xF0);
        msg.push_back(0x43);
        msg.push_back(0x4D);
        msg.push_back(0x4D);
        msg.push_back(0x06); // Volume command
        msg.push_back(vol8); // Volume value
        msg.push_back(0xF7);
        wemosControl.pushMessage(msg);
        break;
    }
    case ID_CHANN_L:
        break;
    case ID_CHANN_R:
        break;
    case ID_PRIM_NOTE:
        break;
    case ID_SECN_NOTE:
        break;
    default:
        break;
    }
}

const std::string ClicSettingsMenuItem::menul1(void)const
{
    const char* labels[ID_COUNT] = {
            "Volume", "Left Channel", "Right Channel", "Primary Note", "Second. Note"
    };
    return (m_lrIdx < ID_COUNT ? addVertArrow (labels[m_lrIdx]) : label_empty);
}

const std::string ClicSettingsMenuItem::menul2(void)const
{
    switch (m_lrIdx)
    {
    case ID_VOLUME:
        return edit_me + std::to_string(m_volume) + "%";
    case ID_CHANN_L:
        return "15";
    case ID_CHANN_R:
        return "16";
    case ID_PRIM_NOTE:
        return "24";
    case ID_SECN_NOTE:
        return "25";
    default:
        break;
    }
    return label_empty;
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
    printf("MenuItem:%s\n",title.c_str());
    if (m_parent)
    {
        m_parent->m_subMenus.push_back(this);
        m_parent->m_iter = m_parent->m_subMenus.begin();
    }
}
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

void MenuItem::onSelPressShort(void)
{
    if (m_iter != m_subMenus.end())
    {
        MenuItem* menuItem (*m_iter);
        globalMenu.setMenu(menuItem);
    }
}

void MenuItem::onSelPressLong(void)
{
    if (m_parent)
    {
        globalMenu.setMenu(m_parent);
    }
}

const std::string MenuItem::menul1(void)const
{
    if (m_iter != m_subMenus.end() && m_subMenus.size() > 1)
    {
        return addVertArrow(subMenuName());
    }
    return subMenuName();
}

const std::string MenuItem::menul2(void)const
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
        Thread(),
        m_cfg (MenuCfg()),
        m_currentMenu(&::playMenuItem)
{
    Thread::start();
}

/*******************************************************************************/
MainMenu& MainMenu::instance (void)
{
    static MainMenu ins;
    return ins;
}

/*******************************************************************************/
void MainMenu::setMenu(MenuItem* menu)
{
    m_currentMenu = menu;
    printf("New menu => %s\n",m_currentMenu->name.c_str());
}

/*******************************************************************************/
void MainMenu::body(void)
{
    while (1)
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
}

} // namespace PBKR

