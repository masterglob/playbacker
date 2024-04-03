#pragma once

#define PBKR_VERSION "2.0.3"
extern const int PBKR_BUILD_ID;

/*
 New in V1.0.2:
   - Add "negative" latency (applied to MIDI instead of samples)
   - Add version & build on "config WEB page" + refactor tabs
 New in V1.0.3:
   - Correction for MIDI Volume issue (not checked)
 New in V2.0.0:
   - Add support for 4 channels audio file. 2 additional channels
       provide left & right click directly (WIP)
 New in V2.0.1:
   - Modification for RPI 3B+:
   - Remove WEMOS and reuse WEMOS button for Enter Menu
   - Red LED is now driven by clic (flashes when signal on CLIC line)
 New in V2.0.2:
   - Add "/pbkrctrl/nTrack" on OSC
   - Add individual volume level for each song (Clic & Pb)
 New in V2.0.3:
   - Give more options to Tinypas (Menu/prev/next..)
   - Add Python manager!
   - Support PROTOOLS files
   - Initiate "MIDI learn" feature (Not completed yet)
   - General improvements for OSC & MIDI

 Known bugs:
   - Default MIDI volume is not applied until it is manually changed
 TODO:
   - handle multiple OSC clients (change to Multicast or ahndle a list of connected clients)
   - Manage volume & clicVolume (See main_pkbr.cpp)
   - Auto-identify HW ? (currently set by caller script in argv[])

 */

