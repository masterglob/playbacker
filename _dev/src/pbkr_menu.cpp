
#include <unistd.h>

#include "pbkr_menu.h"
#include "pbkr_display.h"

namespace
{
using namespace PBKR;

// http://patorjk.com/software/taag/#p=display&f=Banner3&t=NET%20MENU

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
    if (isUp)
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
            return "ETH / IP:";
        case 1:
            return "ETH / Netmask:";
        default:
            break;
        }
        return "ETH:";
    case 1:
        switch (m_upDownIdx)
        {
        case 0:
            return "WIFI / IP:";
        case 1:
            return "WIFI / Netmask:";
        default:
            break;
        }
        return "WIFI:";
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

void MenuItem::onSelPressShort(void)
{
    printf("MenuItem(%s) Selection\n",name.c_str());
    if (m_iter != m_subMenus.end())
    {
        MenuItem* menuItem (*m_iter);
        globalMenu.setMenu(menuItem);
    }
}

void MenuItem::onSelPressLong(void)
{
    printf("MenuItem(%s) Exit\n",name.c_str());
    if (m_parent)
    {
        globalMenu.setMenu(m_parent);
    }
}

const std::string MenuItem::menul2(void)const
{
    if (m_iter != m_subMenus.end())
    {
        MenuItem* menuItem (*m_iter);
        return std::string ("<") + menuItem->subMenuName() + ">";
    }
    return "##No items##";
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

