#ifndef RANDOM_GENERATOR_MODE_H
#define RANDOM_GENERATOR_MODE_H

#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"

// Random Generator mode variables
struct RandomGen {
  int rootNote = 60; // C4
  int scaleType = 0; // Major
  int minOctave = 3;
  int maxOctave = 6;
  int probability = 50; // 0-100%
  int bpm = 120; // BPM instead of interval
  int subdivision = 4; // 4=quarter, 8=eighth, 16=sixteenth
  bool isPlaying = false;
  unsigned long lastNoteTime = 0;
  unsigned long nextNoteTime = 0;
  int currentNote = -1;
  unsigned long noteInterval = 500; // Calculated from BPM
};

RandomGen randomGen;

// Function declarations
void initializeRandomGeneratorMode();
void drawRandomGeneratorMode();
void handleRandomGeneratorMode();
void drawRandomGenControls();
void updateRandomGenerator();
void playRandomNote();
void calculateNoteInterval();

// Implementations
void initializeRandomGeneratorMode() {
  randomGen.rootNote = 60;
  randomGen.scaleType = 0;
  randomGen.minOctave = 3;
  randomGen.maxOctave = 6;
  randomGen.probability = 50;
  randomGen.bpm = 120;
  randomGen.subdivision = 4;
  randomGen.isPlaying = false;
  randomGen.currentNote = -1;
  calculateNoteInterval();
  randomGen.nextNoteTime = millis() + randomGen.noteInterval;
  
  drawRandomGeneratorMode();
}

void drawRandomGeneratorMode() {
  tft.fillScreen(THEME_BG);
  drawModuleHeader("RNG JAMS");
  
  drawRandomGenControls();
}

void drawRandomGenControls() {
  int y = 55;
  int btnHeight = 45;
  int spacing = 5;
  
  // Play/Stop and Root note on same line
  drawRoundButton(10, y, 60, btnHeight, randomGen.isPlaying ? "STOP" : "PLAY", 
                 randomGen.isPlaying ? THEME_ERROR : THEME_SUCCESS);
  
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawString("Key:", 80, y + 15, 1);
  String rootName = getNoteNameFromMIDI(randomGen.rootNote);
  drawRoundButton(110, y, 50, btnHeight, rootName, THEME_PRIMARY);
  drawRoundButton(165, y, 45, btnHeight, "+", THEME_SECONDARY);
  drawRoundButton(215, y, 45, btnHeight, "-", THEME_SECONDARY);
  
  // Scale selector
  drawRoundButton(270, y, 80, btnHeight, scales[randomGen.scaleType].name, THEME_ACCENT);
  
  y += btnHeight + spacing + 5;
  
  // Octave range
  tft.drawString("Oct:", 10, y + 15, 1);
  tft.drawString(String(randomGen.minOctave) + "-" + String(randomGen.maxOctave), 40, y + 15, 1);
  drawRoundButton(75, y, 55, btnHeight, "MIN-", THEME_SECONDARY);
  drawRoundButton(135, y, 55, btnHeight, "MIN+", THEME_SECONDARY);
  drawRoundButton(195, y, 55, btnHeight, "MAX-", THEME_SECONDARY);
  drawRoundButton(255, y, 55, btnHeight, "MAX+", THEME_SECONDARY);
  
  y += btnHeight + spacing + 5;
  
  // Probability with visual bar
  tft.drawString("Chance:", 10, y + 15, 1);
  tft.drawString(String(randomGen.probability) + "%", 60, y + 15, 1);
  drawRoundButton(105, y, 45, btnHeight, "-", THEME_SECONDARY);
  drawRoundButton(155, y, 45, btnHeight, "+", THEME_SECONDARY);
  
  // Compact probability bar - clear and redraw
  int barW = 80;
  int barX = 210;
  tft.fillRect(barX, y + 18, barW, 10, THEME_BG); // Clear old bar
  tft.drawRect(barX, y + 18, barW, 10, THEME_TEXT_DIM);
  int fillW = (barW * randomGen.probability) / 100;
  if (fillW > 0) {
    tft.fillRect(barX + 1, y + 19, fillW, 8, THEME_PRIMARY);
  }
  
  y += btnHeight + spacing + 5;
  
  // BPM and subdivision controls
  tft.drawString("BPM:", 10, y + 15, 1);
  tft.drawString(String(randomGen.bpm), 45, y + 15, 1);
  drawRoundButton(75, y, 45, btnHeight, "-", THEME_SECONDARY);
  drawRoundButton(125, y, 45, btnHeight, "+", THEME_SECONDARY);
  
  tft.drawString("Beat:", 180, y + 15, 1);
  String subdivText;
  if (randomGen.subdivision == 4) subdivText = "1/4";
  else if (randomGen.subdivision == 8) subdivText = "1/8";
  else if (randomGen.subdivision == 16) subdivText = "1/16";
  tft.drawString(subdivText, 220, y + 15, 1);
  drawRoundButton(260, y, 45, btnHeight, "<", THEME_SECONDARY);
  drawRoundButton(310, y, 45, btnHeight, ">", THEME_SECONDARY);
  
  y += btnHeight + spacing + 5;
  
  // Current note indicator
  if (randomGen.currentNote != -1) {
    tft.setTextColor(THEME_PRIMARY, THEME_BG);
    tft.drawString("Now: ", 10, y + 5, 1);
    String currentNoteName = getNoteNameFromMIDI(randomGen.currentNote);
    tft.setTextColor(THEME_ACCENT, THEME_BG);
    tft.drawString(currentNoteName, 50, y + 5, 2);
  }
}

void handleRandomGeneratorMode() {
  // Back button - larger touch area
  if (touch.justPressed && isButtonPressed(BACK_BTN_X, BACK_BTN_Y, BTN_BACK_W, BTN_BACK_H)) {
    exitToMenu();
    return;
  }
  
  // Calculate button positions for press feedback
  int y1 = 55;
  int y2 = y1 + 22 + 5;
  int y3 = y2 + 22 + 5;
  int y4 = y3 + 22 + 5;
  
  // Check all button press states
  bool playPressed = touch.isPressed && isButtonPressed(10, y1, 60, 25);
  bool keyUpPressed = touch.isPressed && isButtonPressed(150, y1, 25, 25);
  bool keyDownPressed = touch.isPressed && isButtonPressed(180, y1, 25, 25);
  bool scalePressed = touch.isPressed && isButtonPressed(220, y1, 80, 25);
  
  bool minOctDownPressed = touch.isPressed && isButtonPressed(70, y2, 35, 25);
  bool minOctUpPressed = touch.isPressed && isButtonPressed(110, y2, 35, 25);
  bool maxOctDownPressed = touch.isPressed && isButtonPressed(150, y2, 35, 25);
  bool maxOctUpPressed = touch.isPressed && isButtonPressed(190, y2, 35, 25);
  
  bool probDownPressed = touch.isPressed && isButtonPressed(85, y3, 25, 25);
  bool probUpPressed = touch.isPressed && isButtonPressed(115, y3, 25, 25);
  
  bool bpmDownPressed = touch.isPressed && isButtonPressed(65, y4, 25, 25);
  bool bpmUpPressed = touch.isPressed && isButtonPressed(95, y4, 25, 25);
  bool subdivLeftPressed = touch.isPressed && isButtonPressed(190, y4, 25, 25);
  bool subdivRightPressed = touch.isPressed && isButtonPressed(220, y4, 25, 25);
  
  // Draw buttons with press feedback - only when pressed
  bool anyPressed = playPressed || keyUpPressed || keyDownPressed || scalePressed || minOctDownPressed || minOctUpPressed || maxOctDownPressed || maxOctUpPressed || probDownPressed || probUpPressed || bpmDownPressed || bpmUpPressed || subdivLeftPressed || subdivRightPressed;
  
  if (anyPressed) {
    drawRoundButton(10, y1, 60, 25, randomGen.isPlaying ? "STOP" : "PLAY", 
                   randomGen.isPlaying ? THEME_ERROR : THEME_SUCCESS, playPressed);
    
    String rootName = getNoteNameFromMIDI(randomGen.rootNote);
    drawRoundButton(110, y1, 35, 25, rootName, THEME_PRIMARY, false);
    drawRoundButton(150, y1, 25, 25, "+", THEME_SECONDARY, keyUpPressed);
    drawRoundButton(180, y1, 25, 25, "-", THEME_SECONDARY, keyDownPressed);
    drawRoundButton(220, y1, 80, 25, scales[randomGen.scaleType].name, THEME_ACCENT, scalePressed);
    
    drawRoundButton(70, y2, 35, 25, "MIN-", THEME_SECONDARY, minOctDownPressed);
    drawRoundButton(110, y2, 35, 25, "MIN+", THEME_SECONDARY, minOctUpPressed);
    drawRoundButton(150, y2, 35, 25, "MAX-", THEME_SECONDARY, maxOctDownPressed);
    drawRoundButton(190, y2, 35, 25, "MAX+", THEME_SECONDARY, maxOctUpPressed);
    
    drawRoundButton(85, y3, 25, 25, "-", THEME_SECONDARY, probDownPressed);
    drawRoundButton(115, y3, 25, 25, "+", THEME_SECONDARY, probUpPressed);
    
    drawRoundButton(65, y4, 25, 25, "-", THEME_SECONDARY, bpmDownPressed);
    drawRoundButton(95, y4, 25, 25, "+", THEME_SECONDARY, bpmUpPressed);
    drawRoundButton(190, y4, 25, 25, "<", THEME_SECONDARY, subdivLeftPressed);
    drawRoundButton(220, y4, 25, 25, ">", THEME_SECONDARY, subdivRightPressed);
  }
  
  if (touch.justPressed) {
    int y = 55;
    int btnHeight = 45;
    int spacing = 5;
    
    // Play/Stop and Root note controls
    if (isButtonPressed(10, y, 60, btnHeight)) {
      randomGen.isPlaying = !randomGen.isPlaying;
      if (randomGen.isPlaying) {
        randomGen.nextNoteTime = millis() + randomGen.noteInterval;
      } else if (randomGen.currentNote != -1) {
        sendNoteOff(randomGen.currentNote);
        randomGen.currentNote = -1;
      }
      drawRandomGenControls();
      return;
    }
    
    if (isButtonPressed(165, y, 45, btnHeight)) {
      randomGen.rootNote = min(127, randomGen.rootNote + 1);
      drawRandomGenControls();
      return;
    }
    if (isButtonPressed(215, y, 45, btnHeight)) {
      randomGen.rootNote = max(0, randomGen.rootNote - 1);
      drawRandomGenControls();
      return;
    }
    
    // Scale selector
    if (isButtonPressed(270, y, 80, btnHeight)) {
      randomGen.scaleType = (randomGen.scaleType + 1) % NUM_SCALES;
      drawRandomGenControls();
      return;
    }
    
    y += btnHeight + spacing + 5;
    
    // Octave controls
    if (isButtonPressed(75, y, 55, btnHeight)) {
      randomGen.minOctave = max(1, randomGen.minOctave - 1);
      if (randomGen.minOctave >= randomGen.maxOctave) {
        randomGen.maxOctave = randomGen.minOctave + 1;
      }
      drawRandomGenControls();
      return;
    }
    if (isButtonPressed(135, y, 55, btnHeight)) {
      randomGen.minOctave = min(8, randomGen.minOctave + 1);
      if (randomGen.minOctave >= randomGen.maxOctave) {
        randomGen.maxOctave = randomGen.minOctave + 1;
      }
      drawRandomGenControls();
      return;
    }
    if (isButtonPressed(195, y, 55, btnHeight)) {
      randomGen.maxOctave = max(randomGen.minOctave + 1, randomGen.maxOctave - 1);
      drawRandomGenControls();
      return;
    }
    if (isButtonPressed(255, y, 55, btnHeight)) {
      randomGen.maxOctave = min(9, randomGen.maxOctave + 1);
      drawRandomGenControls();
      return;
    }
    
    y += btnHeight + spacing + 5;
    
    // Probability controls
    if (isButtonPressed(105, y, 45, btnHeight)) {
      randomGen.probability = max(0, randomGen.probability - 5);
      drawRandomGenControls();
      return;
    }
    if (isButtonPressed(155, y, 45, btnHeight)) {
      randomGen.probability = min(100, randomGen.probability + 5);
      drawRandomGenControls();
      return;
    }
    
    y += btnHeight + spacing + 5;
    
    // BPM controls
    if (isButtonPressed(75, y, 45, btnHeight)) {
      randomGen.bpm = max(60, randomGen.bpm - 5);
      calculateNoteInterval();
      drawRandomGenControls();
      return;
    }
    if (isButtonPressed(125, y, 45, btnHeight)) {
      randomGen.bpm = min(200, randomGen.bpm + 5);
      calculateNoteInterval();
      drawRandomGenControls();
      return;
    }
    
    // Subdivision controls
    if (isButtonPressed(260, y, 45, btnHeight)) {
      if (randomGen.subdivision == 16) randomGen.subdivision = 8;
      else if (randomGen.subdivision == 8) randomGen.subdivision = 4;
      calculateNoteInterval();
      drawRandomGenControls();
      return;
    }
    if (isButtonPressed(310, y, 45, btnHeight)) {
      if (randomGen.subdivision == 4) randomGen.subdivision = 8;
      else if (randomGen.subdivision == 8) randomGen.subdivision = 16;
      calculateNoteInterval();
      drawRandomGenControls();
      return;
    }
  }
  
  // Update random generator
  updateRandomGenerator();
}

void updateRandomGenerator() {
  if (!randomGen.isPlaying || !globalState.bleConnected) return;
  
  unsigned long now = millis();
  
  if (now >= randomGen.nextNoteTime) {
    playRandomNote();
    randomGen.nextNoteTime = now + randomGen.noteInterval;
  }
}

void playRandomNote() {
  // Stop current note if playing
  if (randomGen.currentNote != -1) {
    sendNoteOff(randomGen.currentNote);
    randomGen.currentNote = -1;
  }
  
  // Check probability
  if (random(100) < randomGen.probability) {
    // Generate random note in scale and octave range
    const Scale& scale = scales[randomGen.scaleType];
    int degree = random(scale.numNotes);
    int octave = random(randomGen.minOctave, randomGen.maxOctave + 1);
    int note = randomGen.rootNote % 12 + scale.intervals[degree] + (octave * 12);
    
    if (note >= 0 && note <= 127) {
      sendNoteOn(note, 100);
      randomGen.currentNote = note;
      
      Serial.printf("Random note: %s (prob: %d%%)\n", 
                   getNoteNameFromMIDI(note).c_str(), randomGen.probability);
      
      // Update display
      drawRandomGenControls();
    }
  }
}

void calculateNoteInterval() {
  // Calculate note interval from BPM and subdivision
  float beatsPerSecond = randomGen.bpm / 60.0;
  float notesPerSecond = beatsPerSecond * (randomGen.subdivision / 4.0);
  randomGen.noteInterval = 1000.0 / notesPerSecond;
}

#endif