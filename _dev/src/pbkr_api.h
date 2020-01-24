#pragma once

#include "pbkr_config.h"
#include "pbkr_midi.h"

#include <string>

namespace PBKR
{

using namespace std;
/*******************************************************************************
 * GLOBAL CONSTANTS
 *******************************************************************************/

/*******************************************************************************
 *
 *******************************************************************************/
namespace API
{
/**
 * @param v volume between 0 and 1
 */
void setClicVolume  (const float& v);

/**
 * A command entered on OSC or WEB (TODO)
 */
std::string onKeyboardCmd  (const std::string& msg);

/**
 * Process an incoming MIDI event received from a given controller
 */
void onMidiEvent(const MIDI::MIDI_Msg& msg, const MIDI::MIDI_Ctrl_Cfg& cfg);

} // namespace API

} // namespace PBKR
