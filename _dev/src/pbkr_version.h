#pragma once

#define PBKR_VERSION "2.0.0"
extern const int PBKR_BUILD_ID;

/*
 New in V1.0.2:
   - Add "negative" latency (applied to MIDI instead of samples)
   - Add version & build on "config WEB page" + refactor tabs
 New in V1.0.3:
   - TODO

 Known bugs:
   - Latency & ESP self-test do not takie into account actual
       sample rate (44100/48000)
   - Default MIDI volume is not applied until it is manually changed

 */

