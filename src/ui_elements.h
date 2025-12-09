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
  tft.fillRect(0, 0, 480, 45, THEME_SURFACE);
  tft.drawFastHLine(0, 45, 480, THEME_PRIMARY);
  
  tft.setTextColor(THEME_TEXT, THEME_SURFACE);
  tft.drawCentreString(title, 240, 8, 4);
  
  if (subtitle.length() > 0) {
    tft.setTextColor(THEME_TEXT_DIM, THEME_SURFACE);
    tft.drawCentreString(subtitle, 240, 28, 2);
  }
  
  drawRoundButton(10, 5, 70, 35, "BACK", THEME_ERROR);
}

inline void updateStatus() {
  // Status bar removed - no more BLE connection alerts on every screen
}

#endif