#ifndef AUTO_CHORD_MODE_H
#define AUTO_CHORD_MODE_H

#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"

// Auto Chord mode variables - traditional piano chords
struct ChordType {
  String name;
  int intervals[4]; // Root, 3rd, 5th, optional 7th
  int numNotes;
};

// Traditional chord types - classic piano voicings
ChordType diatonicChords[] = {
  {"I", {0, 4, 7, -1}, 3},      // Major triad
  {"ii", {0, 3, 7, -1}, 3},     // Minor triad  
  {"iii", {0, 3, 7, -1}, 3},    // Minor triad
  {"IV", {0, 4, 7, -1}, 3},     // Major triad
  {"V", {0, 4, 7, -1}, 3},      // Major triad
  {"vi", {0, 3, 7, -1}, 3},     // Minor triad
  {"vii°", {0, 3, 6, -1}, 3},   // Diminished triad
  {"I+", {0, 4, 7, -1}, 3}      // Major triad (octave)
};

int chordOctave = 4;
int chordScale = 0;
int activeChordNotes[8][4]; // [chord][note index]
bool chordPressed[8] = {false}; // 8 diatonic chords

// Function declarations
void initializeAutoChordMode();
void drawAutoChordMode();
void handleAutoChordMode();
void drawChordKeys();
void playChord(int scaleDegree, bool on);
void stopAllChords();

// Implementations
void initializeAutoChordMode() {
  chordOctave = 4;
  chordScale = 0;
  stopAllChords();
  for (int i = 0; i < 8; i++) {
    chordPressed[i] = false;
    for (int j = 0; j < 4; j++) {
      activeChordNotes[i][j] = -1;
    }
  }
  
  drawAutoChordMode();
}

void drawAutoChordMode() {
  tft.fillScreen(THEME_BG);
  drawHeader("CHORD MODE", scales[chordScale].name + " Diatonic");
  
  drawChordKeys();
  
  // Calculate control button layout from screen dimensions
  int btnSpacing = 10;
  int ctrlY = SCREEN_HEIGHT - 80;
  int btnH = 35;
  int btnW = (SCREEN_WIDTH - (5 * btnSpacing)) / 4;
  
  drawRoundButton(btnSpacing, ctrlY, btnW, btnH, "OCT-", THEME_SECONDARY);
  drawRoundButton(btnSpacing * 2 + btnW, ctrlY, btnW, btnH, "OCT+", THEME_SECONDARY);
  drawRoundButton(btnSpacing * 3 + btnW * 2, ctrlY, btnW, btnH, "SCALE", THEME_ACCENT);
  drawRoundButton(btnSpacing * 4 + btnW * 3, ctrlY, btnW, btnH, "CLEAR", THEME_ERROR);
  
  // Status
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawString("Oct " + String(chordOctave), btnSpacing, SCREEN_HEIGHT - 15, 2);
  tft.drawString("Classic piano chords", SCREEN_WIDTH / 2 - 60, ctrlY - 25, 1);
}

void drawChordKeys() {
  int keyWidth = SCREEN_WIDTH / 8;
  int keyY = CONTENT_TOP + 10;
  int availableHeight = SCREEN_HEIGHT - keyY - 120; // Leave space for controls
  int keyHeight = availableHeight;
  
  uint16_t degreeColors[] = {
    THEME_PRIMARY, THEME_SECONDARY, THEME_ACCENT, THEME_SUCCESS,
    THEME_WARNING, THEME_ERROR, 0xF81F, 0x07E0
  };
  
  for (int i = 0; i < 8; i++) {
    int x = i * keyWidth;
    
    uint16_t bgColor = chordPressed[i] ? degreeColors[i] : THEME_SURFACE;
    uint16_t textColor = chordPressed[i] ? THEME_BG : degreeColors[i];
    
    tft.fillRect(x + 2, keyY + 2, keyWidth - 4, keyHeight - 4, bgColor);
    tft.drawRect(x, keyY, keyWidth, keyHeight, degreeColors[i]);
    tft.drawRect(x + 1, keyY + 1, keyWidth - 2, keyHeight - 2, degreeColors[i]);
    
    // Roman numeral
    tft.setTextColor(textColor, bgColor);
    tft.drawCentreString(diatonicChords[i].name, x + keyWidth/2, keyY + keyHeight/3, 4);
    
    // Root note name
    int rootNote;
    if (i == 7) { // I+ octave
      rootNote = getNoteInScale(chordScale, 0, chordOctave + 1);
    } else {
      rootNote = getNoteInScale(chordScale, i, chordOctave);
    }
    String rootName = getNoteNameFromMIDI(rootNote);
    tft.drawCentreString(rootName, x + keyWidth/2, keyY + (keyHeight * 2)/3, 2);
  }
}

void handleAutoChordMode() {
  // Back button - larger touch area
  if (touch.justPressed && isButtonPressed(BACK_BTN_X, BACK_BTN_Y, BTN_BACK_W, BTN_BACK_H)) {
    exitToMenu();
    return;
  }
  
  // Calculate button layout from screen dimensions
  int btnSpacing = 10;
  int ctrlY = SCREEN_HEIGHT - 80;
  int btnH = 35;
  int btnW = (SCREEN_WIDTH - (5 * btnSpacing)) / 4;
  
  if (touch.justPressed) {
    // Octave controls
    if (isButtonPressed(btnSpacing, ctrlY, btnW, btnH)) {
      chordOctave = max(2, chordOctave - 1);
      drawAutoChordMode();
      return;
    }
    if (isButtonPressed(btnSpacing * 2 + btnW, ctrlY, btnW, btnH)) {
      chordOctave = min(6, chordOctave + 1);
      drawAutoChordMode();
      return;
    }
    
    // Scale selector
    if (isButtonPressed(btnSpacing * 3 + btnW * 2, ctrlY, btnW, btnH)) {
      chordScale = (chordScale + 1) % NUM_SCALES;
      drawAutoChordMode();
      return;
    }
    
    // Clear all
    if (isButtonPressed(btnSpacing * 4 + btnW * 3, ctrlY, btnW, btnH)) {
      stopAllChords();
      drawChordKeys();
      return;
    }
    
    // Chord keys - calculate dimensions to match drawChordKeys()
    int keyWidth = SCREEN_WIDTH / 8;
    int keyY = CONTENT_TOP + 10;
    int availableHeight = SCREEN_HEIGHT - keyY - 120;
    int keyHeight = availableHeight;
    
    for (int i = 0; i < 8; i++) {
      int x = i * keyWidth;
      if (isButtonPressed(x, keyY, keyWidth, keyHeight)) {
        if (!chordPressed[i]) {
          // Turn on chord
          playChord(i, true);
          chordPressed[i] = true;
          drawChordKeys();
        }
        return;
      }
    }
  }
  
  // Handle single key press/hold functionality
  if (touch.isPressed) {
    int keyWidth = SCREEN_WIDTH / 8;
    int keyHeight = 100;
    int keyY = CONTENT_TOP + 20;
    
    int currentKey = -1;
    
    // Find which key is being pressed
    for (int i = 0; i < 8; i++) {
      int x = i * keyWidth;
      if (touch.x >= x && touch.x < x + keyWidth && 
          touch.y >= keyY && touch.y < keyY + keyHeight) {
        currentKey = i;
        break;
      }
    }
    
    // Only allow one chord at a time
    if (currentKey != -1) {
      // Turn off all other chords first
      for (int i = 0; i < 8; i++) {
        if (i != currentKey && chordPressed[i]) {
          playChord(i, false);
          chordPressed[i] = false;
        }
      }
      
      // Turn on the current chord if not already on
      if (!chordPressed[currentKey]) {
        playChord(currentKey, true);
        chordPressed[currentKey] = true;
        drawChordKeys();
      }
    }
  } else {
    // Touch released - turn off all chords
    bool anyChanged = false;
    for (int i = 0; i < 8; i++) {
      if (chordPressed[i]) {
        playChord(i, false);
        chordPressed[i] = false;
        anyChanged = true;
      }
    }
    if (anyChanged) {
      drawChordKeys();
    }
  }
}

void playChord(int scaleDegree, bool on) {
  if (!globalState.bleConnected) return;
  
  // Get root note for this scale degree
  int rootNote;
  if (scaleDegree == 7) { // I+ octave
    rootNote = getNoteInScale(chordScale, 0, chordOctave + 1);
  } else {
    rootNote = getNoteInScale(chordScale, scaleDegree, chordOctave);
  }
  
  if (on) {
    // Play traditional diatonic chord (root, 3rd, 5th)
    ChordType chord = diatonicChords[scaleDegree];
    
    for (int i = 0; i < chord.numNotes; i++) {
      if (chord.intervals[i] >= 0) {
        int chordNote = rootNote + chord.intervals[i];
        if (chordNote >= 24 && chordNote <= 108) {
          sendNoteOn(chordNote, 100);
          activeChordNotes[scaleDegree][i] = chordNote;
        }
      }
    }
  } else {
    // Stop chord for this specific scale degree
    for (int i = 0; i < 4; i++) {
      if (activeChordNotes[scaleDegree][i] != -1) {
        sendNoteOff(activeChordNotes[scaleDegree][i]);
        activeChordNotes[scaleDegree][i] = -1;
      }
    }
  }
}

void stopAllChords() {
  for (int i = 0; i < 8; i++) {
    if (chordPressed[i]) {
      playChord(i, false);
      chordPressed[i] = false;
    }
  }
}

#endif