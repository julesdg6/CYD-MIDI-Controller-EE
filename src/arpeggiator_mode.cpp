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
  drawModuleHeader("ARPEGGIATOR");
  
  drawArpControls();
  drawPianoKeys();
}

void drawArpControls() {
  int y = 55;
  int btnHeight = 45;
  int spacing = 5;
  
  // Pattern and chord type
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawString("Pattern:", 10, y + 12, 1);
  drawRoundButton(65, y, 70, btnHeight, patternNames[arp.pattern], THEME_WARNING);
  drawRoundButton(140, y, 45, btnHeight, "<", THEME_SECONDARY);
  drawRoundButton(190, y, 45, btnHeight, ">", THEME_SECONDARY);
  
  // Chord type
  tft.drawString("Type:", 245, y + 12, 1);
  drawRoundButton(285, y, 60, btnHeight, chordTypeNames[arp.chordType], THEME_ACCENT);
  
  y += btnHeight + spacing + 5;
  
  // Octaves and Speed
  tft.drawString("Oct:", 10, y + 12, 1);
  tft.drawString(String(arp.octaves), 45, y + 12, 1);
  drawRoundButton(60, y, 45, btnHeight, "-", THEME_SECONDARY);
  drawRoundButton(110, y, 45, btnHeight, "+", THEME_SECONDARY);
  
  // Speed
  tft.drawString("Spd:", 165, y + 12, 1);
  String speedText;
  if (arp.speed == 4) speedText = "4th";
  else if (arp.speed == 8) speedText = "8th";
  else if (arp.speed == 16) speedText = "16th";
  else if (arp.speed == 32) speedText = "32nd";
  tft.drawString(speedText, 200, y + 12, 1);
  drawRoundButton(240, y, 45, btnHeight, "+", THEME_SECONDARY);
  drawRoundButton(290, y, 45, btnHeight, "-", THEME_SECONDARY);
  
  y += btnHeight + spacing + 5;
  
  // BPM Control
  tft.drawString("BPM:", 10, y + 12, 1);
  tft.drawString(String(arp.bpm), 50, y + 12, 1);
  drawRoundButton(75, y, 45, btnHeight, "-", THEME_SECONDARY);
  drawRoundButton(125, y, 45, btnHeight, "+", THEME_SECONDARY);
  
  // Current status (compact)
  if (arp.isPlaying && arp.triggeredKey != -1) {
    tft.setTextColor(THEME_PRIMARY, THEME_BG);
    String keyName = getNoteNameFromMIDI(arp.triggeredKey);
    tft.drawString(keyName + " " + chordTypeNames[arp.chordType], 180, y + 12, 1);
  }
  
  y += btnHeight + spacing;
  
  // Piano octave controls
  int pianoOctY = y - 8;  // Slight adjustment for better visual alignment
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawString("Piano Oct:", 10, y, 1);
  tft.drawString(String(pianoOctave), 80, y, 1);
  drawRoundButton(100, pianoOctY, 45, btnHeight, "-", THEME_SECONDARY);
  drawRoundButton(150, pianoOctY, 45, btnHeight, "+", THEME_SECONDARY);
  
  // Current note display
  if (arp.currentNote != -1) {
    tft.setTextColor(THEME_ACCENT, THEME_BG);
    String currentNoteName = getNoteNameFromMIDI(arp.currentNote);
    tft.drawString("♪ " + currentNoteName, 210, y, 2);
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
  
  if (touch.justPressed) {
    int y = 55;
    int btnHeight = 45;
    int spacing = 5;
    
    // Pattern controls (first line)
    if (isButtonPressed(140, y, 45, btnHeight)) {
      arp.pattern = (arp.pattern - 1 + 5) % 5;
      drawArpControls();
      return;
    }
    if (isButtonPressed(190, y, 45, btnHeight)) {
      arp.pattern = (arp.pattern + 1) % 5;
      drawArpControls();
      return;
    }
    
    // Chord type control (first line)
    if (isButtonPressed(285, y, 60, btnHeight)) {
      arp.chordType = (arp.chordType + 1) % 3;
      drawArpControls();
      return;
    }
    
    y += btnHeight + spacing + 5;
    
    // Octave controls
    if (isButtonPressed(60, y, 45, btnHeight)) {
      arp.octaves = max(1, arp.octaves - 1);
      drawArpControls();
      return;
    }
    if (isButtonPressed(110, y, 45, btnHeight)) {
      arp.octaves = min(4, arp.octaves + 1);
      drawArpControls();
      return;
    }
    
    // Speed controls (+ = faster, - = slower)
    if (isButtonPressed(240, y, 45, btnHeight)) {
      // Faster (+ button)
      if (arp.speed == 32) arp.speed = 16;
      else if (arp.speed == 16) arp.speed = 8;
      else if (arp.speed == 8) arp.speed = 4;
      calculateStepInterval();
      drawArpControls();
      return;
    }
    if (isButtonPressed(290, y, 45, btnHeight)) {
      // Slower (- button)
      if (arp.speed == 4) arp.speed = 8;
      else if (arp.speed == 8) arp.speed = 16;
      else if (arp.speed == 16) arp.speed = 32;
      calculateStepInterval();
      drawArpControls();
      return;
    }
    
    y += btnHeight + spacing + 5;
    
    // BPM controls
    if (isButtonPressed(75, y, 45, btnHeight)) {
      arp.bpm = max(60, arp.bpm - 5);
      calculateStepInterval();
      drawArpControls();
      return;
    }
    if (isButtonPressed(125, y, 45, btnHeight)) {
      arp.bpm = min(200, arp.bpm + 5);
      calculateStepInterval();
      drawArpControls();
      return;
    }
    
    y += btnHeight + spacing;
    
    // Piano octave controls
    int pianoOctY = y - 8;  // Slight adjustment for better visual alignment
    if (isButtonPressed(100, pianoOctY, 45, btnHeight)) {
      pianoOctave = max(1, pianoOctave - 1);
      drawPianoKeys();
      drawArpControls();
      return;
    }
    if (isButtonPressed(150, pianoOctY, 45, btnHeight)) {
      pianoOctave = min(7, pianoOctave + 1);
      drawPianoKeys();
      drawArpControls();
      return;
    }
    
    // Piano key handling
    int keyY = 160;
    int keyWidth = SCREEN_WIDTH / NUM_PIANO_KEYS;
    int keyHeight = 50;
    
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
