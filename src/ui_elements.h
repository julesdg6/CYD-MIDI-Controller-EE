#ifndef UI_ELEMENTS_H
#define UI_ELEMENTS_H

#include "common_definitions.h"
#include "touch_calibration.h"

// UI function declarations
void updateTouch();
void updateStatus();
bool isButtonPressed(int x, int y, int w, int h);
void drawRoundButton(int x, int y, int w, int h, String text, uint16_t color, bool pressed = false);
void drawHeader(String title, String subtitle = "");
void drawModuleHeader(String title, bool showBackButton = true);
void drawSettingsIcon(int x, int y);
void drawBackIcon(int x, int y);
void drawBluetoothIcon(int x, int y);
void drawSDCardIcon(int x, int y);
void drawBPMIndicator(int x, int y);
void exitToMenu();

// UI implementations
inline void updateTouch() {
  extern TouchCalibration calibration;
  
  touch.wasPressed = touch.isPressed;
  touch.isPressed = ts.tirqTouched() && ts.touched();
  touch.justPressed = touch.isPressed && !touch.wasPressed;
  touch.justReleased = !touch.isPressed && touch.wasPressed;
  
  if (touch.isPressed) {
    TS_Point p = ts.getPoint();
    uint16_t rawX = p.x;
    uint16_t rawY = p.y;
    
    // Debug: print raw values even if calibration invalid
    static unsigned long lastDebug = 0;
    if (millis() - lastDebug > 500) {
      Serial.printf("[TOUCH] Raw: (%d,%d) CalibValid: %d\n", rawX, rawY, calibration.valid);
      lastDebug = millis();
    }
    
    if (!calibration.valid) return;
    
    // Apply XY swap if calibrated that way
    if (calibration.swap_xy) {
      uint16_t temp = rawX;
      rawX = rawY;
      rawY = temp;
    }
    
    // Map using calibrated values
    int mappedX = map(rawX, calibration.x_min, calibration.x_max, 0, 480);
    int mappedY = map(rawY, calibration.y_min, calibration.y_max, 0, 320);
    
    // Apply rotation (default to 0 if not set)
    uint8_t rot = calibration.rotation;
    if (rot > 3) rot = 0;  // Safety check
    
    switch (rot) {
      case 0:  // No rotation
        touch.x = mappedX;
        touch.y = mappedY;
        break;
      case 1:  // 90° clockwise
        touch.x = 320 - mappedY;
        touch.y = mappedX;
        break;
      case 2:  // 180°
        touch.x = 480 - mappedX;
        touch.y = 320 - mappedY;
        break;
      case 3:  // 270° clockwise (90° counter-clockwise)
        touch.x = mappedY;
        touch.y = 480 - mappedX;
        break;
    }
    
    // Constrain to screen bounds
    touch.x = constrain(touch.x, 0, 479);
    touch.y = constrain(touch.y, 0, 319);
  }
}

inline bool isButtonPressed(int x, int y, int w, int h) {
  return touch.x >= x && touch.x <= x + w && touch.y >= y && touch.y <= y + h;
}

inline void drawRoundButton(int x, int y, int w, int h, String text, uint16_t color, bool pressed) {
  uint16_t bgColor = pressed ? color : THEME_SURFACE;
  uint16_t borderColor = color;
  uint16_t textColor = pressed ? THEME_BG : color;
  
  tft.fillRoundRect(x, y, w, h, 8, bgColor);
  tft.drawRoundRect(x, y, w, h, 8, borderColor);
  tft.drawRoundRect(x+1, y+1, w-2, h-2, 7, borderColor);
  
  tft.setTextColor(textColor, bgColor);
  tft.drawCentreString(text, x + w/2, y + h/2 - 8, 2);
}

inline void drawHeader(String title, String subtitle) {
  tft.fillRect(0, 0, SCREEN_WIDTH, SCALED_H(45), THEME_SURFACE);
  tft.drawFastHLine(0, SCALED_H(45), SCREEN_WIDTH, THEME_PRIMARY);
  
  tft.setTextColor(THEME_TEXT, THEME_SURFACE);
  tft.drawCentreString(title, SCREEN_WIDTH/2, SCALED_H(8), 4);
  
  if (subtitle.length() > 0) {
    tft.setTextColor(THEME_TEXT_DIM, THEME_SURFACE);
    tft.drawCentreString(subtitle, SCREEN_WIDTH/2, SCALED_H(28), 2);
  }
  
  drawRoundButton(SCALED_W(10), SCALED_H(5), BTN_BACK_W, BTN_BACK_H, "BACK", THEME_ERROR);
}

inline void updateStatus() {
  // Status bar removed - no more BLE connection alerts on every screen
}

inline void drawSettingsIcon(int x, int y) {
  // Draw a cog/gear icon
  uint16_t color = THEME_PRIMARY;
  tft.fillCircle(x + 10, y + 10, 8, color);
  tft.fillCircle(x + 10, y + 10, 4, THEME_SURFACE);
  // Cog teeth
  for (int i = 0; i < 8; i++) {
    float angle = i * PI / 4;
    int x1 = x + 10 + cos(angle) * 6;
    int y1 = y + 10 + sin(angle) * 6;
    int x2 = x + 10 + cos(angle) * 10;
    int y2 = y + 10 + sin(angle) * 10;
    tft.drawLine(x1, y1, x2, y2, color);
  }
}

inline void drawBackIcon(int x, int y) {
  // Draw a back arrow icon
  uint16_t color = THEME_ERROR;
  int cx = x + 10;
  int cy = y + 10;
  
  // Arrow pointing left
  tft.fillTriangle(cx - 5, cy, cx + 3, cy - 6, cx + 3, cy + 6, color);
  tft.fillRect(cx + 2, cy - 2, 8, 4, color);
}

inline void drawBluetoothIcon(int x, int y) {
  uint16_t color = globalState.bleConnected ? 0x001F : THEME_TEXT_DIM;
  int cx = x + 10;
  int cy = y + 10;
  
  // Vertical line
  tft.fillRect(cx, cy - 8, 2, 17, color);
  
  // Upper triangle
  tft.drawLine(cx, cy - 8, cx + 7, cy - 1, color);
  tft.drawLine(cx, cy - 8, cx + 8, cy - 1, color);
  tft.drawLine(cx + 7, cy - 1, cx, cy, color);
  tft.drawLine(cx + 8, cy - 1, cx, cy, color);
  
  // Lower triangle
  tft.drawLine(cx, cy, cx + 7, cy + 8, color);
  tft.drawLine(cx, cy, cx + 8, cy + 8, color);
  tft.drawLine(cx, cy + 8, cx + 7, cy + 8, color);
  tft.drawLine(cx, cy + 8, cx + 8, cy + 8, color);
  
  // Cross lines
  tft.drawLine(cx - 5, cy - 5, cx + 7, cy + 5, color);
  tft.drawLine(cx - 5, cy + 5, cx + 7, cy - 5, color);
}

inline void drawSDCardIcon(int x, int y) {
  extern bool sdCardAvailable;
  uint16_t color = sdCardAvailable ? THEME_SUCCESS : THEME_TEXT_DIM;
  tft.fillRoundRect(x, y, 20, 20, 2, color);
  tft.fillRect(x + 2, y + 2, 16, 6, THEME_SURFACE);
  tft.fillRect(x + 4, y + 12, 12, 6, THEME_BG);
  tft.fillRect(x + 16, y + 4, 2, 4, THEME_BG);
}

inline void drawBPMIndicator(int x, int y) {
  extern GlobalState globalState;
  extern MIDIClockSync midiClock;
  
  // Use MIDI clock BPM if receiving, otherwise use global BPM
  float displayBPM = midiClock.isReceiving ? midiClock.calculatedBPM : globalState.bpm;
  String bpmText = String((int)displayBPM);
  
  if (midiClock.isReceiving) {
    bpmText += " [EXT]";
    tft.setTextColor(THEME_WARNING, THEME_SURFACE);
  } else {
    tft.setTextColor(THEME_TEXT_DIM, THEME_SURFACE);
  }
  
  tft.drawString(bpmText, x, y, 2);
}

inline void drawModuleHeader(String title, bool showBackButton) {
  // Draw header bar
  tft.fillRect(0, 0, SCREEN_WIDTH, SCALED_H(45), THEME_SURFACE);
  tft.drawFastHLine(0, SCALED_H(45), SCREEN_WIDTH, THEME_PRIMARY);
  
  // Draw title centered
  tft.setTextColor(THEME_TEXT, THEME_SURFACE);
  tft.drawCentreString(title, SCREEN_WIDTH/2, SCALED_H(13), 4);
  
  // Draw left icon (back button or settings on main menu)
  if (showBackButton) {
    drawBackIcon(SCALED_W(8), SCALED_H(8));
  } else {
    drawSettingsIcon(SCALED_W(8), SCALED_H(8));
  }
  
  // Draw right-side status icons
  drawBluetoothIcon(SCREEN_WIDTH - SCALED_W(70), SCALED_H(8));  // Bluetooth at far right
  drawSDCardIcon(SCREEN_WIDTH - SCALED_W(100), SCALED_H(10));     // SD card next to Bluetooth
  drawBPMIndicator(SCREEN_WIDTH - SCALED_W(180), SCALED_H(17));   // BPM indicator on the left side
}

#endif