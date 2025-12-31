#ifndef KEYBOARD_MODE_H
#define KEYBOARD_MODE_H

#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"

// Keyboard mode variables
#define NUM_KEYS 10  // More keys per row
#define NUM_ROWS 2   // Two rows
int keyboardOctave = 4;
int keyboardScale = 0;
int keyboardKey = 0;  // Key signature (C=0, C#=1, D=2, etc.)
int lastKey = -1;
int lastRow = -1;

// Control buttons
Button keyboardBtnOctDown;
Button keyboardBtnOctUp;
Button keyboardBtnScale;
Button keyboardBtnKeyDown;
Button keyboardBtnKeyUp;
Button keyboardBtnMenu;

// Function declarations
void initializeKeyboardMode();
void drawKeyboardMode();
void handleKeyboardMode();
void drawKeyboardKey(int row, int keyIndex, bool pressed);
void playKeyboardNote(int row, int keyIndex, bool on);

// Implementations
void initializeKeyboardMode() {
  keyboardOctave = 4;
  keyboardScale = 0;
  keyboardKey = 0;
  lastKey = -1;
  lastRow = -1;
  
  // Calculate button layout from screen dimensions
  int btnY = SCREEN_HEIGHT - 60;
  int btnH = 45;
  int btnSpacing = 10;
  int totalBtnWidth = SCREEN_WIDTH - (2 * btnSpacing);
  int btn1W = (totalBtnWidth - (5 * btnSpacing)) / 6;
  int btn2W = btn1W;
  int btn3W = btn1W + 20;
  
  // Initialize control buttons with calculated positioning
  keyboardBtnOctDown.setBounds(btnSpacing, btnY, btn1W, btnH);
  keyboardBtnOctDown.setText("OCT-");
  keyboardBtnOctDown.setColor(THEME_SECONDARY);
  
  keyboardBtnOctUp.setBounds(btnSpacing * 2 + btn1W, btnY, btn1W, btnH);
  keyboardBtnOctUp.setText("OCT+");
  keyboardBtnOctUp.setColor(THEME_SECONDARY);
  
  keyboardBtnScale.setBounds(btnSpacing * 3 + btn1W * 2, btnY, btn3W, btnH);
  keyboardBtnScale.setText("SCALE");
  keyboardBtnScale.setColor(THEME_ACCENT);
  
  keyboardBtnKeyDown.setBounds(btnSpacing * 4 + btn1W * 2 + btn3W, btnY, btn1W, btnH);
  keyboardBtnKeyDown.setText("KEY-");
  keyboardBtnKeyDown.setColor(THEME_WARNING);
  
  keyboardBtnKeyUp.setBounds(btnSpacing * 5 + btn1W * 3 + btn3W, btnY, btn1W, btnH);
  keyboardBtnKeyUp.setText("KEY+");
  keyboardBtnKeyUp.setColor(THEME_WARNING);
  
  keyboardBtnMenu.setBounds(btnSpacing * 6 + btn1W * 4 + btn3W, btnY, btn1W + 20, btnH);
  keyboardBtnMenu.setText("MENU");
  keyboardBtnMenu.setColor(THEME_ERROR);
  
  drawKeyboardMode();
}

void drawKeyboardMode() {
  tft.fillScreen(THEME_BG);
  drawModuleHeader("KEYS");
  
  // Show scale and key info under header
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  String info = scales[keyboardScale].name + " - Key " + getNoteNameFromMIDI(keyboardKey);
  tft.drawCentreString(info, SCREEN_WIDTH/2, CONTENT_TOP + 2, 2);
  
  // Draw keys - two rows
  for (int row = 0; row < NUM_ROWS; row++) {
    for (int i = 0; i < NUM_KEYS; i++) {
      drawKeyboardKey(row, i, false);
    }
  }
  
  // Control layout - larger buttons at bottom, spanning full width
  int btnY = SCALED_H(240);  // Lower position
  int btnH = BTN_MEDIUM_H;   // Taller buttons
  int margin = SCALED_W(5);
  int spacing = SCALED_W(5);
  
  // 6 buttons spanning full width
  int totalW = SCREEN_WIDTH - (2 * margin) - (5 * spacing);
  int btnW = totalW / 6;
  
  int x1 = margin;
  int x2 = x1 + btnW + spacing;
  int x3 = x2 + btnW + spacing;
  int x4 = x3 + btnW + spacing;
  int x5 = x4 + btnW + spacing;
  int x6 = x5 + btnW + spacing;
  
  drawRoundButton(x1, btnY, btnW, btnH, "OCT-", THEME_SECONDARY, false);
  drawRoundButton(x2, btnY, btnW, btnH, "OCT+", THEME_SECONDARY, false);
  drawRoundButton(x3, btnY, btnW, btnH, "SCALE", THEME_ACCENT, false);
  drawRoundButton(x4, btnY, btnW, btnH, "KEY-", THEME_WARNING, false);
  drawRoundButton(x5, btnY, btnW, btnH, "KEY+", THEME_WARNING, false);
  drawRoundButton(x6, btnY, btnW, btnH, "MENU", THEME_PRIMARY, false);
  
  // Status display
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  String status = "Oct " + String(keyboardOctave) + " | " + 
               scales[keyboardScale].name + " | Key: " + getNoteNameFromMIDI(keyboardKey);
  tft.drawString(status, 10, SCREEN_HEIGHT - 15, 2);
}

void drawKeyboardKey(int row, int keyIndex, bool pressed) {
  int keyWidth = SCREEN_WIDTH / NUM_KEYS;
  int keySpacing = 5;
  int keyHeight = (SCREEN_HEIGHT - CONTENT_TOP - 80 - 20) / NUM_ROWS; // Fill available space
  int keyY = CONTENT_TOP + 20 + (row * (keyHeight + keySpacing));
  int x = keyIndex * keyWidth;
  
  uint16_t bgColor = pressed ? THEME_PRIMARY : THEME_SURFACE;
  uint16_t borderColor = THEME_PRIMARY;
  uint16_t textColor = pressed ? THEME_BG : THEME_TEXT;
  
  tft.fillRect(x + 1, keyY + 1, keyWidth - 2, keyHeight - 2, bgColor);
  tft.drawRect(x, keyY, keyWidth, keyHeight, borderColor);
  
  // Row 0 = base octave, Row 1 = octave higher
  // Apply key signature transpose
  int note = getNoteInScale(keyboardScale, keyIndex, keyboardOctave + row) + keyboardKey;
  String noteName = getNoteNameFromMIDI(note);
  
  tft.setTextColor(textColor, bgColor);
  tft.drawCentreString(noteName, x + keyWidth/2, keyY + keyHeight/2 - 6, 1);
}

void handleKeyboardMode() {
  if (touch.justPressed && isButtonPressed(BACK_BTN_X, BACK_BTN_Y, BTN_BACK_W, BTN_BACK_H)) {
    exitToMenu();
    return;
  }
  
  // Calculate button layout from screen dimensions
  int btnY = SCALED_H(240);
  int btnH = BTN_MEDIUM_H;
  int margin = SCALED_W(5);
  int spacing = SCALED_W(5);
  
  // 6 buttons spanning full width
  int totalW = SCREEN_WIDTH - (2 * margin) - (5 * spacing);
  int btnW = totalW / 6;
  
  int x1 = margin;
  int x2 = x1 + btnW + spacing;
  int x3 = x2 + btnW + spacing;
  int x4 = x3 + btnW + spacing;
  int x5 = x4 + btnW + spacing;
  int x6 = x5 + btnW + spacing;
  
  // Check button presses
  bool octDownPressed = touch.isPressed && isButtonPressed(x1, btnY, btnW, btnH);
  bool octUpPressed = touch.isPressed && isButtonPressed(x2, btnY, btnW, btnH);
  bool scalePressed = touch.isPressed && isButtonPressed(x3, btnY, btnW, btnH);
  bool keyDownPressed = touch.isPressed && isButtonPressed(x4, btnY, btnW, btnH);
  bool keyUpPressed = touch.isPressed && isButtonPressed(x5, btnY, btnW, btnH);
  bool menuPressed = touch.isPressed && isButtonPressed(x6, btnY, btnW, btnH);
  
  // Draw button press feedback only if pressed
  if (octDownPressed || octUpPressed || scalePressed || keyDownPressed || keyUpPressed || menuPressed) {
    drawRoundButton(x1, btnY, btnW, btnH, "OCT-", THEME_SECONDARY, octDownPressed);
    drawRoundButton(x2, btnY, btnW, btnH, "OCT+", THEME_SECONDARY, octUpPressed);
    drawRoundButton(x3, btnY, btnW, btnH, "SCALE", THEME_ACCENT, scalePressed);
    drawRoundButton(x4, btnY, btnW, btnH, "KEY-", THEME_WARNING, keyDownPressed);
    drawRoundButton(x5, btnY, btnW, btnH, "KEY+", THEME_WARNING, keyUpPressed);
    drawRoundButton(x6, btnY, btnW, btnH, "MENU", THEME_PRIMARY, menuPressed);
  }
  
  if (touch.justPressed) {
    if (octDownPressed) {
      keyboardOctave = max(1, keyboardOctave - 1);
      drawKeyboardMode();
      return;
    }
    if (octUpPressed) {
      keyboardOctave = min(8, keyboardOctave + 1);
      drawKeyboardMode();
      return;
    }
    if (scalePressed) {
      keyboardScale = (keyboardScale + 1) % NUM_SCALES;
      drawKeyboardMode();
      return;
    }
    if (keyDownPressed) {
      keyboardKey = (keyboardKey - 1 + 12) % 12;
      drawKeyboardMode();
      return;
    }
    if (keyUpPressed) {
      keyboardKey = (keyboardKey + 1) % 12;
      drawKeyboardMode();
      return;
    }
    if (menuPressed) {
      // MENU button - return to main menu
      exitToMenu();
      return;
    }
  }
  
  // Key sliding support - handle two rows
  int key = -1;
  int row = -1;
  
  // Check which key and row is being touched - use calculated dimensions
  if (touch.isPressed) {
    int keyWidth = SCREEN_WIDTH / NUM_KEYS;
    int keySpacing = 5;
    int keyHeight = (SCREEN_HEIGHT - CONTENT_TOP - 80 - 20) / NUM_ROWS;
    
    for (int r = 0; r < NUM_ROWS; r++) {
      int keyY = CONTENT_TOP + 20 + (r * (keyHeight + keySpacing));
      if (touch.y >= keyY && touch.y < keyY + keyHeight) {
        row = r;
        key = touch.x / keyWidth;
        if (key >= NUM_KEYS) key = NUM_KEYS - 1;
        break;
      }
    }
  }
  
  if (touch.isPressed && key != -1 && row != -1) {
    if (key != lastKey || row != lastRow) {
      if (lastKey != -1 && lastRow != -1) {
        playKeyboardNote(lastRow, lastKey, false);
        drawKeyboardKey(lastRow, lastKey, false);
      }
      playKeyboardNote(row, key, true);
      drawKeyboardKey(row, key, true);
      lastKey = key;
      lastRow = row;
    }
  } else if (touch.justReleased && lastKey != -1 && lastRow != -1) {
    playKeyboardNote(lastRow, lastKey, false);
    drawKeyboardKey(lastRow, lastKey, false);
    lastKey = -1;
    lastRow = -1;
  }
}

void playKeyboardNote(int row, int keyIndex, bool on) {
  int note = getNoteInScale(keyboardScale, keyIndex, keyboardOctave + row) + keyboardKey;
  
  if (on) {
    sendNoteOn(note, 100);
  } else {
    sendNoteOff(note);
  }
  
  Serial.printf("Key R%d:%d: %s %s\n", row, keyIndex, getNoteNameFromMIDI(note).c_str(), on ? "ON" : "OFF");
}

#endif