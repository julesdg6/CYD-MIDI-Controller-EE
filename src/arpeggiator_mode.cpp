#include "arpeggiator_mode.h"

// Global variable definitions
Arpeggiator arp;

const char* const patternNames[] = {"UP", "DOWN", "UP/DN", "RAND", "CHANCE"};
const char* const chordTypeNames[] = {"MAJ", "MIN", "7TH"};

int pianoOctave = 4;

// Function implementations

void initializeArpeggiatorMode() {
  arp.scaleType = 0;
  arp.chordType = 0;
  arp.pattern = 0;
  arp.octaves = 2;
  arp.speed = 8;
  arp.bpm = 120;
  arp.isPlaying = false;
  arp.currentStep = 0;
  arp.currentNote = -1;
  arp.triggeredKey = -1;
  arp.triggeredOctave = 4;
  pianoOctave = 4;
  calculateStepInterval();
  
  drawArpeggiatorMode();
}

void drawArpeggiatorMode() {
  tft.fillScreen(THEME_BG);
  drawHeader("ARPEGGIATOR", "Piano Chord Arps");
  
  drawArpControls();
  drawPianoKeys();
}

void drawArpControls() {
  int y = 55;
  int spacing = 25;
  
  // Pattern and chord type
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawString("Pattern:", 10, y + 6, 1);
  drawRoundButton(65, y, 60, 25, patternNames[arp.pattern], THEME_WARNING, false);
  drawRoundButton(130, y, 25, 25, "<", THEME_SECONDARY, false);
  drawRoundButton(160, y, 25, 25, ">", THEME_SECONDARY, false);
  
  // Chord type
  tft.drawString("Type:", 200, y + 6, 1);
  drawRoundButton(240, y, 50, 25, chordTypeNames[arp.chordType], THEME_ACCENT, false);
  
  y += spacing;
  
  // Octaves and Speed
  tft.drawString("Octaves:", 10, y + 6, 1);
  tft.drawString(String(arp.octaves), 70, y + 6, 1);
  drawRoundButton(90, y, 25, 25, "-", THEME_SECONDARY, false);
  drawRoundButton(120, y, 25, 25, "+", THEME_SECONDARY, false);
  
  // Speed
  tft.drawString("Speed:", 160, y + 6, 1);
  String speedText;
  if (arp.speed == 4) speedText = "4th";
  else if (arp.speed == 8) speedText = "8th";
  else if (arp.speed == 16) speedText = "16th";
  else if (arp.speed == 32) speedText = "32nd";
  tft.drawString(speedText, 210, y + 6, 1);
  drawRoundButton(240, y, 25, 25, "+", THEME_SECONDARY, false);
  drawRoundButton(270, y, 25, 25, "-", THEME_SECONDARY, false);
  
  y += spacing;
  
  // BPM Control
  tft.drawString("BPM:", 10, y + 6, 1);
  tft.drawString(String(arp.bpm), 50, y + 6, 1);
  drawRoundButton(80, y, 25, 25, "-", THEME_SECONDARY, false);
  drawRoundButton(110, y, 25, 25, "+", THEME_SECONDARY, false);
  
  y += spacing;
  
  // Piano octave controls
  tft.drawString("Piano Oct:", 10, y + 6, 1);
  tft.drawString(String(pianoOctave), 80, y + 6, 1);
  drawRoundButton(100, y, 25, 25, "-", THEME_SECONDARY, false);
  drawRoundButton(130, y, 25, 25, "+", THEME_SECONDARY, false);
  
  // Current status
  if (arp.isPlaying && arp.triggeredKey != -1) {
    tft.setTextColor(THEME_PRIMARY, THEME_BG);
    String keyName = getNoteNameFromMIDI(arp.triggeredKey);
    tft.drawString("Arping: " + keyName + " " + chordTypeNames[arp.chordType], 170, y + 6, 1);
  }
  
  y += spacing;
  
  // Current note display
  if (arp.currentNote != -1) {
    tft.setTextColor(THEME_ACCENT, THEME_BG);
    String currentNoteName = getNoteNameFromMIDI(arp.currentNote);
    tft.drawString("♪ " + currentNoteName, 10, y + 6, 2);
  }
}

void drawPianoKeys() {
  int keyY = 160;
  int keyWidth = SCREEN_WIDTH / NUM_PIANO_KEYS;
  int keyHeight = 50;
  
  for (int i = 0; i < NUM_PIANO_KEYS; i++) {
    int x = i * keyWidth;
    int note = (pianoOctave * 12) + i;
    String noteName = getNoteNameFromMIDI(note);
    
    bool isPressed = (arp.isPlaying && arp.triggeredKey == note);
    uint16_t bgColor = isPressed ? THEME_PRIMARY : THEME_SURFACE;
    uint16_t textColor = isPressed ? THEME_BG : THEME_TEXT;
    
    // Black key styling for sharps
    if (noteName.indexOf('#') != -1) {
      bgColor = isPressed ? THEME_ACCENT : THEME_TEXT;
      textColor = isPressed ? THEME_BG : THEME_SURFACE;
    }
    
    tft.fillRect(x + 1, keyY + 1, keyWidth - 2, keyHeight - 2, bgColor);
    tft.drawRect(x, keyY, keyWidth, keyHeight, THEME_PRIMARY);
    
    tft.setTextColor(textColor, bgColor);
    tft.drawCentreString(noteName, x + keyWidth/2, keyY + keyHeight/2 - 6, 1);
  }
}

void handleArpeggiatorMode() {
  // Back button - larger touch area
  if (touch.justPressed && isButtonPressed(BACK_BTN_X, BACK_BTN_Y, BTN_BACK_W, BTN_BACK_H)) {
    exitToMenu();
    return;
  }
  
  // Calculate button positions for press feedback
  int y1 = 55;
  int y2 = y1 + 25;
  int y3 = y2 + 25;
  int y4 = y3 + 25;
  
  // Check button press states
  bool patternLeftPressed = touch.isPressed && isButtonPressed(130, y1, 25, 25);
  bool patternRightPressed = touch.isPressed && isButtonPressed(160, y1, 25, 25);
  bool chordTypePressed = touch.isPressed && isButtonPressed(240, y1, 50, 25);
  
  bool octDownPressed = touch.isPressed && isButtonPressed(90, y2, 25, 25);
  bool octUpPressed = touch.isPressed && isButtonPressed(120, y2, 25, 25);
  bool speedUpPressed = touch.isPressed && isButtonPressed(240, y2, 25, 25);
  bool speedDownPressed = touch.isPressed && isButtonPressed(270, y2, 25, 25);
  
  bool bpmDownPressed = touch.isPressed && isButtonPressed(80, y3, 25, 25);
  bool bpmUpPressed = touch.isPressed && isButtonPressed(110, y3, 25, 25);
  
  bool pianoOctDownPressed = touch.isPressed && isButtonPressed(100, y4, 25, 25);
  bool pianoOctUpPressed = touch.isPressed && isButtonPressed(130, y4, 25, 25);
  
  // Draw buttons with press feedback
  drawRoundButton(65, y1, 60, 25, patternNames[arp.pattern], THEME_WARNING, false);
  drawRoundButton(130, y1, 25, 25, "<", THEME_SECONDARY, patternLeftPressed);
  drawRoundButton(160, y1, 25, 25, ">", THEME_SECONDARY, patternRightPressed);
  drawRoundButton(240, y1, 50, 25, chordTypeNames[arp.chordType], THEME_ACCENT, chordTypePressed);
  
  drawRoundButton(90, y2, 25, 25, "-", THEME_SECONDARY, octDownPressed);
  drawRoundButton(120, y2, 25, 25, "+", THEME_SECONDARY, octUpPressed);
  drawRoundButton(240, y2, 25, 25, "+", THEME_SECONDARY, speedUpPressed);
  drawRoundButton(270, y2, 25, 25, "-", THEME_SECONDARY, speedDownPressed);
  
  drawRoundButton(80, y3, 25, 25, "-", THEME_SECONDARY, bpmDownPressed);
  drawRoundButton(110, y3, 25, 25, "+", THEME_SECONDARY, bpmUpPressed);
  
  drawRoundButton(100, y4, 25, 25, "-", THEME_SECONDARY, pianoOctDownPressed);
  drawRoundButton(130, y4, 25, 25, "+", THEME_SECONDARY, pianoOctUpPressed);
  
  if (touch.justPressed) {
    int y = 55;
    int spacing = 25;
    
    // Pattern controls (first line)
    if (patternLeftPressed) {
      arp.pattern = (arp.pattern - 1 + 5) % 5;
      drawArpControls();
      return;
    }
    if (patternRightPressed) {
      arp.pattern = (arp.pattern + 1) % 5;
      drawArpControls();
      return;
    }
    
    // Chord type control (first line)
    if (chordTypePressed) {
      arp.chordType = (arp.chordType + 1) % 3;
      drawArpControls();
      return;
    }
    
    y += spacing;
    
    // Octave controls
    if (octDownPressed) {
      arp.octaves = max(1, arp.octaves - 1);
      drawArpControls();
      return;
    }
    if (octUpPressed) {
      arp.octaves = min(4, arp.octaves + 1);
      drawArpControls();
      return;
    }
    
    // Speed controls (+ = faster, - = slower)
    if (speedUpPressed) {
      // Faster (+ button)
      if (arp.speed == 32) arp.speed = 16;
      else if (arp.speed == 16) arp.speed = 8;
      else if (arp.speed == 8) arp.speed = 4;
      calculateStepInterval();
      drawArpControls();
      return;
    }
    if (speedDownPressed) {
      // Slower (- button)
      if (arp.speed == 4) arp.speed = 8;
      else if (arp.speed == 8) arp.speed = 16;
      else if (arp.speed == 16) arp.speed = 32;
      calculateStepInterval();
      drawArpControls();
      return;
    }
    
    y += spacing;
    
    // BPM controls
    if (bpmDownPressed) {
      arp.bpm = max(60, arp.bpm - 5);
      calculateStepInterval();
      drawArpControls();
      return;
    }
    if (bpmUpPressed) {
      arp.bpm = min(200, arp.bpm + 5);
      calculateStepInterval();
      drawArpControls();
      return;
    }
    
    y += spacing;
    
    // Piano octave controls
    if (pianoOctDownPressed) {
      pianoOctave = max(1, pianoOctave - 1);
      drawPianoKeys();
      drawArpControls();
      return;
    }
    if (pianoOctUpPressed) {
      pianoOctave = min(7, pianoOctave + 1);
      drawPianoKeys();
      drawArpControls();
      return;
    }
    
    // Piano key handling
    int keyY = 160;
    int keyWidth = SCREEN_WIDTH / NUM_PIANO_KEYS;
    int keyHeight = 45;
    
    for (int i = 0; i < NUM_PIANO_KEYS; i++) {
      int x = i * keyWidth;
      if (isButtonPressed(x, keyY, keyWidth, keyHeight)) {
        int note = (pianoOctave * 12) + i;
        
        if (arp.isPlaying && arp.triggeredKey == note) {
          // Stop current arp
          arp.isPlaying = false;
          if (arp.currentNote != -1) {
            sendNoteOff(arp.currentNote);
            arp.currentNote = -1;
          }
        } else {
          // Start new arp - keep timing if already playing
          if (arp.isPlaying && arp.currentNote != -1) {
            sendNoteOff(arp.currentNote);
          }
          arp.triggeredKey = note;
          arp.triggeredOctave = pianoOctave;
          if (!arp.isPlaying) {
            arp.isPlaying = true;
            arp.currentStep = 0;
            arp.lastStepTime = millis();
          }
        }
        drawPianoKeys();
        drawArpControls();
        return;
      }
    }
  }
  
  // Update arpeggiator
  updateArpeggiator();
}

void updateArpeggiator() {
  if (!arp.isPlaying) return;
  
  unsigned long now = millis();
  
  // Use MIDI clock if available
  unsigned long effectiveInterval;
  if (midiClock.isReceiving && midiClock.clockInterval > 0) {
    // Calculate interval based on arp speed and MIDI clock
    // MIDI clock is 24 ppqn
    int clocksPerNote = 24 / arp.speed;  // e.g., 8th note = 12 clocks
    effectiveInterval = midiClock.clockInterval * clocksPerNote;
  } else {
    effectiveInterval = arp.stepInterval;
  }
  
  if (now - arp.lastStepTime >= effectiveInterval) {
    playArpNote();
    arp.lastStepTime = now;
  }
}

void playArpNote() {
  // Turn off previous note
  if (arp.currentNote != -1) {
    sendNoteOff(arp.currentNote);
  }
  
  // Check if we should skip this note (for CHANCE pattern)
  if (arp.pattern == 4) { // CHANCE pattern
    if (random(100) < 30) { // 30% chance to skip
      arp.currentNote = -1;
      return; // Skip this note
    }
  }
  
  // Get next chord tone
  arp.currentNote = getArpNote();
  
  // Play single note
  sendNoteOn(arp.currentNote, 100);
  
  // Update display
  drawArpControls();
}

int getArpNote() {
  // Generate chord intervals based on chord type
  int chordIntervals[4];
  int chordLength;
  
  switch (arp.chordType) {
    case 0: // Major
      chordIntervals[0] = 0; chordIntervals[1] = 4; chordIntervals[2] = 7;
      chordLength = 3;
      break;
    case 1: // Minor
      chordIntervals[0] = 0; chordIntervals[1] = 3; chordIntervals[2] = 7;
      chordLength = 3;
      break;
    case 2: // 7th (dominant)
      chordIntervals[0] = 0; chordIntervals[1] = 4; chordIntervals[2] = 7; chordIntervals[3] = 10;
      chordLength = 4;
      break;
  }
  
  int totalSteps = chordLength * arp.octaves;
  int step = 0;
  
  switch (arp.pattern) {
    case 0: // Up
      step = arp.currentStep % totalSteps;
      arp.currentStep++;
      break;
    case 1: // Down
      step = (totalSteps - 1) - (arp.currentStep % totalSteps);
      arp.currentStep++;
      break;
    case 2: // Up/Down
      {
        int cycle = (totalSteps - 1) * 2;
        int pos = arp.currentStep % cycle;
        if (pos < totalSteps) {
          step = pos;
        } else {
          step = cycle - pos;
        }
        arp.currentStep++;
      }
      break;
    case 3: // Random
      step = random(totalSteps);
      break;
    case 4: // Chance (like UP but with random skips)
      step = arp.currentStep % totalSteps;
      arp.currentStep++;
      break;
  }
  
  int octaveOffset = step / chordLength;
  int chordStep = step % chordLength;
  
  return arp.triggeredKey + chordIntervals[chordStep] + (octaveOffset * 12);
}

void calculateStepInterval() {
  // Calculate step interval from BPM and speed
  // BPM = beats per minute, speed = notes per beat (4, 8, 16, 32)
  float beatsPerSecond = arp.bpm / 60.0;
  float notesPerSecond = beatsPerSecond * (arp.speed / 4.0);
  arp.stepInterval = 1000.0 / notesPerSecond;
}
