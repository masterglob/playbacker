#pragma once

#include "pbkr_config.h"
#include "pbkr_utils.h"
#include "pbkr_gpio.h"

#include <iostream>
#include <fstream>

namespace PBKR
{
/*******************************************************************************
 * MAIN MENU CONFIG
 *******************************************************************************/
struct MenuCfg
{
    MenuCfg(void);
    const GPIOs::Button leftGpio;
    const GPIOs::Button rightGpio;
    const GPIOs::Button selectGpio;
};

class MainMenu;

class MenuItem
{
public:
    MenuItem(const std::string & menuName=""):
        name(menuName){}
    virtual ~MenuItem(void){}
    virtual void onSelPress(const float& duration){}
    virtual void onLeftRightPress(const bool isLeft){}
    virtual const std::string menul1(void)const{return "";}
    virtual const std::string menul2(void)const{return "";}
    const std::string name;
};

/*******************************************************************************
 * MAIN MENU MANAGERS
 *******************************************************************************/
class MainMenu : private Thread
{
public:
    static MainMenu& instance (void);
    void setMenu(MenuItem* menu);
    virtual const std::string menul1(void)const{return m_currentMenu->menul1();}
    virtual const std::string menul2(void)const{return m_currentMenu->menul2();}
private:
    MainMenu(void);
    virtual void body(void);
    MenuCfg m_cfg;
    MenuItem* m_currentMenu;
};
extern MainMenu &mainMenu;
} // namespace PBKR
