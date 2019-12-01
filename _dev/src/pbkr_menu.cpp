
#include <unistd.h>

#include "pbkr_menu.h"

namespace
{
using namespace PBKR;

/*******************************************************************************
 * SUB MENUS
 *******************************************************************************/
struct PlayMenuItem : MenuItem
{
    PlayMenuItem(void): MenuItem("PLAY Menu"){}
    virtual ~PlayMenuItem(void){}
    virtual void onSelPress(const float& duration);
    virtual void onLeftRightPress(const bool isLeft);
};
PlayMenuItem playMenuItem;

struct MainMenuItem : MenuItem
{
    MainMenuItem(void): MenuItem("Main Menu"){}
    virtual ~MainMenuItem(void){}
    virtual void onSelPress(const float& duration);
    virtual const std::string menul1(void)const;
    virtual const std::string menul2(void)const;
};
MainMenuItem mainMenuItem;

/*******************************************************************************
 * IMPLEMENTATIONS
 *******************************************************************************/

void PlayMenuItem::onSelPress(const float& duration)
{
    if (duration < 1.2)
    {
        fileManager.startReading();
    }
    else
    {
        mainMenu.setMenu(&mainMenuItem);
    }
}

void PlayMenuItem::onLeftRightPress(const bool isLeft)
{
    if (isLeft)
        fileManager.prevTrack();
    else
        fileManager.nextTrack();
}

void MainMenuItem::onSelPress(const float& duration)
{
    mainMenu.setMenu(&playMenuItem);
}
const std::string MainMenuItem::menul1(void)const
{
    return "MAIN MENU";
}
const std::string MainMenuItem::menul2(void)const
{
    return "<Exit>";
}

} // namespace


namespace PBKR
{
MainMenu& mainMenu(MainMenu::instance());

/*******************************************************************************
 * MAIN MENU CONFIG
 *******************************************************************************/
static const float MAX_BTN_WAIT (1.4);
/*******************************************************************************/
MenuCfg::MenuCfg(void):
                leftGpio (GPIOs::GPIO_27_PIN13,MAX_BTN_WAIT, "LEFT MENU"),
                rightGpio (GPIOs::GPIO_23_PIN16,MAX_BTN_WAIT,  "RIGHT MENU"),
                selectGpio (GPIOs::GPIO_17_PIN11,MAX_BTN_WAIT,  "SEL MENU")
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

        if (m_cfg.selectGpio.pressed(duration))
        {
            m_currentMenu->onSelPress(duration);
        }
    }
}

} // namespace PBKR

