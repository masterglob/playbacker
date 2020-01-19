
#include "pbkr_config.h"
#include "pbkr_utils.h"
#include "pbkr_display.h"
#include "pbkr_console.h"
#include "pbkr_midi.h"

using namespace std;

namespace PBKR
{
//const GPIOs::Led led(GPIOs::GPIO::pinToId(15));
static  FILE*stdoutCpy(stdout);

const float Console::volumeFaderDurationS (0.1);

Console* Console::m_instance (NULL);

/*******************************************************************************/
Console::Console(void): Thread("Console"),
doSine(false),
_volume(0.11)
{
    m_instance = this;
}

/*******************************************************************************/
Console::~Console(void)
{
    m_instance = NULL;
}

/*******************************************************************************/
void
Console::body(void)
{
    printf("Console Ready\n");
    while (not isExitting())
    {
        printf("> ");
        fflush(stdoutCpy);
        const char c ( getch());
        printf("%c\n",c);
        switch (c) {
        case 'q':
        case 'Q':
            printf("Exit requested\n");
            PBKR::DISPLAY::DisplayManager::instance().onEvent(PBKR::DISPLAY::DisplayManager::evEnd);
            doExit();
            break;
            // Sine on/off
        case 's':doSine = not doSine;
            break;
        case '+':
            changeVolume (_volume + 0.02);
            break;
        case '-':
            changeVolume (_volume - 0.02);
            break;
        case 'v':
            printf("Current volume = %d%%\n", (int)(_volume *100));
            break;
        case 0x0A:
        case ' ':
            if (fileManager.reading())
                fileManager.stopReading();
            else
                fileManager.startReading ();
            break;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            fileManager.selectIndex (c - '0');
            break;
        default:
            printf("Unknown command :(0x%02X)\n",c);
            break;
        }
    }
    printf("Console Exiting\n");
    Thread::doExit();
}

/*******************************************************************************/
void Console::changeVolume (float v, const float duration)
{
    if (v > 1.0) v = 1.0;
    if (v < 0.0) v = 0.0;
    _volume = v;

    const uint8_t vol8 (v * 128.0);
    MidiOutMsg msg;
    msg.push_back(MIDI_CMD_CC | MIDI_CHANNEL_16);
    msg.push_back(MIDI_CC_VOLUME);
    msg.push_back(vol8);
    wemosControl.pushMessage(msg);

}

/*******************************************************************************/
float Console::volume(void)
{
    return _volume;
}

}
