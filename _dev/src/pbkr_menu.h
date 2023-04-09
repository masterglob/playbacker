#pragma once

#include "pbkr_config.h"
#include "pbkr_utils.h"
#include "pbkr_gpio.h"

#include <iostream>
#include <fstream>
#include <deque>

namespace PBKR
{

/*******************************************************************************/
class Button: protected PBKR::GPIOs::Input
{
public:
   Button (const unsigned int id, const float & maxWait, const char*name="BUTTON"):
       Input(id, name),m_pressed(false),
       m_must_release(false),
       m_t0 (VirtualTime::now()),
       m_maxWait(maxWait){};
   ~Button(void){}
   bool   pressed(bool& longPress)const;
private :
   mutable bool m_pressed;
   mutable bool m_must_release;
   mutable VirtualTime::Time m_t0;
   const float m_maxWait;
};

/*******************************************************************************
 * MAIN MENU CONFIG
 *******************************************************************************/
struct MenuCfg
{
    MenuCfg(void);
    const Button leftGpio;
    const Button rightGpio;
    const Button upGpio;
    const Button downGpio;
    const Button enterGpio;
    const Button exitGpio;
};

class MainMenu;

class MenuItem
{
public:
    MenuItem(const std::string & title, MenuItem* parent = 0);
    virtual ~MenuItem(void){}
    virtual void onShow(void){}
    virtual void onEnter(const bool longPressed=false);
    virtual void onExit(void);
    virtual void onLeftRightPress(const bool isLeft);
    virtual void onUpDownPress(const bool isUp){}
    virtual const std::string menul1(void);
    virtual const std::string menul2(void);
    virtual const std::string subMenuName(void)const{return name;}
    const std::string name;
protected:
    typedef std::deque<MenuItem*, std::allocator<MenuItem*>> MenuItemQueue;
    MenuItemQueue m_subMenus;
    MenuItem*    m_parent;
private:
    MenuItemQueue::iterator m_iter;
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
    typedef enum {KEY_LEFT,KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_OK, KEY_CANCEL} Key;
    void pressKey(const Key& key);
private:
    MainMenu(void);
    virtual void body(void);
    MenuCfg m_cfg;
    MenuItem* m_currentMenu;
};
extern MainMenu &globalMenu;
extern void SelectProjectMenu(Project* proj);
extern void openCopyFromUSBMenu();
extern void openNetMenuFromUSBMenu();
extern void openDeleteProjectMenu();

extern void refreshMidiVolume    (void);
extern void refreshLatency    (void);

void setDefaultProject(void);
} // namespace PBKR
