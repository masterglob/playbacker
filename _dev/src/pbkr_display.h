#ifndef _pbkr_display_h_
#define _pbkr_display_h_

#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <inttypes.h>
#include <stdint.h>
#include <string>
#include <thread>
#include <mutex>

#include "pbkr_gpio.h"

namespace PBKR
{
namespace DISPLAY
{
static const int WARNING_DISPLAY_SEC(4);
static const int INFO_DISPLAY_SEC(2);

/*******************************************************************************
 *
 *******************************************************************************/
class I2C_Display
{
public:
    I2C_Display(const int address,
            const GPIOs::GPIO_id powPin,
            const int nb_lines = 2);
    virtual ~I2C_Display(void);
    void begin (void);
    void noDisplay();
    void display();
    void noBlink();
    void blink();
    void noCursor();
    void cursor();
    void noBacklight();
    void backlight();

    void clear();
    void home();
    void write(char value);
    void print(const char* txt);
    /** Col & row start at 0 */
    void setCursor(uint8_t col, uint8_t row);
    void line2(void){setCursor(0, 1);}
private:
    void command(uint8_t value);
    void send(uint8_t value, uint8_t mode);
    void write4bits(uint8_t value);
    void expanderWrite(uint8_t value);
    void pulseEnable(uint8_t _data);
    int _file;
    unsigned char _address;
    GPIOs::Output _powPin;
    uint8_t _displayfunction;
    uint8_t _displaycontrol;
    uint8_t _displaymode;
    uint8_t _backlightval;
    uint8_t _currLine;
    uint8_t _nb_lines;
    // GPIO actually not used, but referenced to monitor used GPIOs
    GPIOs::GPIO _sda;
    GPIOs::GPIO _scl;
}; // class I2C_Display



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
class DisplayManager
{
public:
    static DisplayManager& instance(void);
    enum Event {evBegin,evEnd,
        evProjectTitle, evProjectTrackCount,
        evUsbIn, evUsbOut,
        evPlay, evStop,
        evTrack, evFile};
    void info (const std::string& msg);
    void warning (const std::string& msg);
    void onEvent (const Event e, const std::string& param="");
    void setFilename (const std::string& filename);
    void stopReading (void);
    void startReading (void);
    void setProjectTitle (const std::string& title);
private:
    DisplayManager(void);
    ~DisplayManager(void);
    void refresh(void);
    void incrementTime(void);
    bool m_running;
    bool m_ready;
    volatile uint32_t m_printIdx;
    bool m_isInfo;
    volatile bool m_canEvent;
    std::string m_warning;
    std::string m_line1;
    std::string m_line2;
    std::string m_title;
    std::string m_event;
    std::string m_filename;
    std::string m_trackIdx;
    std::string m_trackCount;
    bool m_reading;
    std::mutex m_mutex;
    std::thread m_thread;
}; // class
extern DisplayManager displayManager;

}  // namespace DISPLAY
}
#endif
