#ifndef ARPEGGIATOR_MODE_H
#define ARPEGGIATOR_MODE_H

#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"

// Arpeggiator mode variables
struct Arpeggiator {
  int scaleType = 0; // Scale for chord generation
  int chordType = 0; // 0=Major, 1=Minor, 2=7th
  int pattern = 0; // 0=Up, 1=Down, 2=UpDown, 3=Random
  int octaves = 2;
  int speed = 8; // 16th notes
  int bpm = 120; // BPM control
  bool isPlaying = false;
  int currentStep = 0;
  int currentNote = -1; // Current single note being played
  unsigned long lastStepTime = 0;
  unsigned long stepInterval = 125; // Calculated from BPM
  int triggeredKey = -1; // Which piano key triggered the arp
  int triggeredOctave = 4; // Octave of the triggered key
};

// Extern declarations (definitions in arpeggiator_mode.cpp)
extern Arpeggiator arp;
extern const char* const patternNames[];
extern const char* const chordTypeNames[];
extern int pianoOctave;

#define NUM_PIANO_KEYS 12

// Function declarations
void initializeArpeggiatorMode();
void drawArpeggiatorMode();
void handleArpeggiatorMode();
void drawArpControls();
void drawPianoKeys();
void updateArpeggiator();
void playArpNote();
int getArpNote();
void calculateStepInterval();

#endif