
#include <stdio.h>
#include <iostream>

#include "pbkr_types.h"
#include "pbkr_config.h"
#include "pbkr_utils.h"
#include "pbkr_display_mgr.h"
#include "pbkr_console.h"
#include "pbkr_projects.h"
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
_volume(0.11),
m_escape(-1)
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
        if (m_escape <0)
        {
            printf("%c\n",c);
            switch (c) {
            case 0x1B:
                m_escape = 0;
                break;
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
            case 'i': // Show current projext infos
                show_current_infos();
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
        else if (m_escape == 0)
        {
            cout << endl;
            if (c == 0x5B)
            {
                m_escape = 0x100 + c;
            }
            else
            {
                printf("Unknown Escape sequence :(0x1B %02X)\n",c);
                m_escape = -1;
            }
        }
        else
        {
            const int esc(m_escape & 0xFF);
            m_escape = -1;
            if (esc == 0x5B)
            {
                continue;
            }
            printf("Unknown Escape sequence :(0x1B %02X %02X)\n", esc, c);
        }
    }
    printf("Console Exiting\n");
    Thread::doExit();
}

/*******************************************************************************/
void
Console::show_current_infos(void)const
{
    ProjectVect projects(getAllProjects());
    FOR (it, projects)
    {
        const Project* p(*it);
        if (!p) continue;
        cout << "Project '" << p->m_title << "' (" << p->m_source.pName << ")" << dec << endl;
        const TrackVect tracks(p->tracks());
        FOR (ptr, tracks)
        {
            const Track& t (*ptr);
            cout << " - " << "#" << t.m_index << " " << t.m_title << "(" << t.m_title << ")" << endl;
        }

    }
} // Console::show_current_infos

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
