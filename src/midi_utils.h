#ifndef MIDI_UTILS_H
#define MIDI_UTILS_H

#include "common_definitions.h"

// External variables
extern uint8_t midiChannel;
extern bool bleEnabled;

// Scale definitions (implementations in midi_utils.cpp)
extern const Scale scales[];
extern const int NUM_SCALES;

// MIDI utility functions (sendMIDI kept inline for performance)
inline void sendMIDI(byte cmd, byte note, byte vel) {
  if (!deviceConnected) return;
  
  // Apply MIDI channel (channels 1-16 are encoded as 0-15 in the lower nibble)
  byte channelCmd = (cmd & 0xF0) | ((midiChannel - 1) & 0x0F);
  
  midiPacket[2] = channelCmd;
  midiPacket[3] = note;
  midiPacket[4] = vel;
  pCharacteristic->setValue(midiPacket, 5);
  pCharacteristic->notify();
}

inline void stopAllModes() {
  // Stop all MIDI notes
  for (int i = 0; i < 128; i++) {
    sendMIDI(0x80, i, 0);
  }
}

// Function declarations (implementations in midi_utils.cpp)
int getNoteInScale(int scaleIndex, int degree, int octave);
String getNoteNameFromMIDI(int midiNote);

#endif