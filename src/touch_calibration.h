#ifndef TOUCH_CALIBRATION_H
#define TOUCH_CALIBRATION_H

#include <EEPROM.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

#define EEPROM_SIZE 64
#define CALIBRATION_MAGIC 0xCAFE
#define EEPROM_ADDR_MAGIC 0
#define EEPROM_ADDR_X_MIN 2
#define EEPROM_ADDR_X_MAX 4
#define EEPROM_ADDR_Y_MIN 6
#define EEPROM_ADDR_Y_MAX 8
#define EEPROM_ADDR_SWAP_XY 10
#define EEPROM_ADDR_ROTATION 11

struct TouchCalibration {
  uint16_t magic;
  uint16_t x_min;
  uint16_t x_max;
  uint16_t y_min;
  uint16_t y_max;
  bool swap_xy;
  uint8_t rotation;  // 0, 1, 2, or 3 for 0°, 90°, 180°, 270°
  bool valid;
};

extern TFT_eSPI tft;
extern XPT2046_Touchscreen ts;

static TouchCalibration calibration;

#include "common_definitions.h"

inline void drawCalibrationCrosshair(int x, int y, uint16_t color) {
  int size = 20;
  tft.drawLine(x - size, y, x + size, y, color);
  tft.drawLine(x, y - size, x, y + size, color);
  tft.drawCircle(x, y, 5, color);
  tft.drawCircle(x, y, 10, color);
}

inline bool waitForTouch(int targetX, int targetY, uint16_t &rawX, uint16_t &rawY) {
  // Draw instruction
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("Touch the crosshair", SCREEN_WIDTH/2, SCREEN_HEIGHT - 40, 4);
  
  unsigned long timeout = millis() + 30000; // 30 second timeout
  bool touched = false;
  
  // Wait for touch
  while (millis() < timeout && !touched) {
    if (ts.tirqTouched() && ts.touched()) {
      delay(50); // Debounce
      if (ts.touched()) {
        TS_Point p = ts.getPoint();
        rawX = p.x;
        rawY = p.y;
        touched = true;
        
        // Visual feedback
        drawCalibrationCrosshair(targetX, targetY, TFT_GREEN);
        tft.fillRect(0, SCREEN_HEIGHT - 50, SCREEN_WIDTH, 50, TFT_BLACK);
        tft.drawCentreString("Got it!", SCREEN_WIDTH/2, SCREEN_HEIGHT - 30, 4);
        delay(500);
        
        // Wait for release
        while (ts.touched()) {
          delay(10);
        }
        delay(200);
      }
    }
    delay(10);
  }
  
  return touched;
}

inline bool performCalibration() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawCentreString("TOUCHSCREEN CALIBRATION", SCREEN_WIDTH/2, 20, 4);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("Touch each crosshair", SCREEN_WIDTH/2, 60, 2);
  tft.drawCentreString("as accurately as possible", SCREEN_WIDTH/2, 85, 2);
  
  delay(2000);
  
  // Define calibration points (screen coordinates)
  struct CalPoint {
    int screenX, screenY;
    uint16_t rawX, rawY;
  };
  
  CalPoint points[3] = {
    {40, 40, 0, 0},                              // Top-left
    {SCREEN_WIDTH - 40, SCREEN_HEIGHT/2, 0, 0},  // Right-center
    {40, SCREEN_HEIGHT - 40, 0, 0}               // Bottom-left
  };
  
  // Collect touch data for each point
  for (int i = 0; i < 3; i++) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    char msg[32];
    sprintf(msg, "Point %d of 3", i + 1);
    tft.drawCentreString(msg, SCREEN_WIDTH/2, 20, 4);
    
    drawCalibrationCrosshair(points[i].screenX, points[i].screenY, TFT_RED);
    
    if (!waitForTouch(points[i].screenX, points[i].screenY, 
                      points[i].rawX, points[i].rawY)) {
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.drawCentreString("CALIBRATION TIMEOUT", SCREEN_WIDTH/2, SCREEN_HEIGHT/2, 4);
      delay(2000);
      return false;
    }
  }
  
  // Calculate calibration values
  calibration.x_min = points[0].rawX;
  calibration.x_max = points[1].rawX;
  calibration.y_min = points[0].rawY;
  calibration.y_max = points[2].rawY;
  
  // Determine if we need to swap X and Y
  int xRange = abs((int)calibration.x_max - (int)calibration.x_min);
  int yRange = abs((int)calibration.y_max - (int)calibration.y_min);
  calibration.swap_xy = (xRange < yRange);
  
  if (calibration.swap_xy) {
    uint16_t temp = calibration.x_min;
    calibration.x_min = calibration.y_min;
    calibration.y_min = temp;
    temp = calibration.x_max;
    calibration.x_max = calibration.y_max;
    calibration.y_max = temp;
  }
  
  // Ensure min < max
  if (calibration.x_min > calibration.x_max) {
    uint16_t temp = calibration.x_min;
    calibration.x_min = calibration.x_max;
    calibration.x_max = temp;
  }
  if (calibration.y_min > calibration.y_max) {
    uint16_t temp = calibration.y_min;
    calibration.y_min = calibration.y_max;
    calibration.y_max = temp;
  }
  
  // Detect rotation by testing a fourth point
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawCentreString("Testing rotation...", SCREEN_WIDTH/2, 20, 4);
  tft.drawCentreString("Touch the crosshair", SCREEN_WIDTH/2, 60, 2);
  
  // Test point at bottom-right (opposite of top-left)
  uint16_t testRawX, testRawY;
  int testScreenX = SCREEN_WIDTH - 40;
  int testScreenY = SCREEN_HEIGHT - 40;
  drawCalibrationCrosshair(testScreenX, testScreenY, TFT_YELLOW);
  
  if (waitForTouch(testScreenX, testScreenY, testRawX, testRawY)) {
    // Map test point with current calibration
    int testX = map(testRawX, calibration.x_min, calibration.x_max, 0, SCREEN_WIDTH);
    int testY = map(testRawY, calibration.y_min, calibration.y_max, 0, SCREEN_HEIGHT);
    
    // Determine rotation based on where the test point landed
    // Expected: bottom-right quadrant
    // If inverted: top-left quadrant
    
    if (testX < SCREEN_WIDTH/2 && testY < SCREEN_HEIGHT/2) {
      // Touch landed in top-left quadrant instead of bottom-right - 180° rotation
      calibration.rotation = 2;
    } else if (testX < SCREEN_WIDTH/2 && testY > SCREEN_HEIGHT/2) {
      // Touch landed in bottom-left quadrant instead of bottom-right - possible 90° rotation
      calibration.rotation = 3;
    } else if (testX > SCREEN_WIDTH/2 && testY < SCREEN_HEIGHT/2) {
      // Touch landed in top-right quadrant instead of bottom-right - possible 270° rotation
      calibration.rotation = 1;
    } else {
      // Touch landed in correct quadrant
      calibration.rotation = 0;
    }
  } else {
    // Timeout on test point, assume no rotation
    calibration.rotation = 0;
  }
  
  calibration.magic = CALIBRATION_MAGIC;
  calibration.valid = true;
  
  // Show results
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawCentreString("CALIBRATION COMPLETE", SCREEN_WIDTH/2, SCREEN_HEIGHT/2 - 60, 4);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  char buffer[64];
  sprintf(buffer, "X: %d - %d", calibration.x_min, calibration.x_max);
  tft.drawCentreString(buffer, SCREEN_WIDTH/2, SCREEN_HEIGHT/2 - 20, 2);
  sprintf(buffer, "Y: %d - %d", calibration.y_min, calibration.y_max);
  tft.drawCentreString(buffer, SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 5, 2);
  sprintf(buffer, "Swap XY: %s", calibration.swap_xy ? "Yes" : "No");
  tft.drawCentreString(buffer, SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 30, 2);
  sprintf(buffer, "Rotation: %d deg", calibration.rotation * 90);
  tft.drawCentreString(buffer, SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 55, 2);
  tft.drawCentreString("Saving to memory...", SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 90, 2);
  
  delay(2000);
  
  return true;
}

inline void saveCalibration() {
  EEPROM.writeUShort(EEPROM_ADDR_MAGIC, calibration.magic);
  EEPROM.writeUShort(EEPROM_ADDR_X_MIN, calibration.x_min);
  EEPROM.writeUShort(EEPROM_ADDR_X_MAX, calibration.x_max);
  EEPROM.writeUShort(EEPROM_ADDR_Y_MIN, calibration.y_min);
  EEPROM.writeUShort(EEPROM_ADDR_Y_MAX, calibration.y_max);
  EEPROM.writeBool(EEPROM_ADDR_SWAP_XY, calibration.swap_xy);
  EEPROM.writeByte(EEPROM_ADDR_ROTATION, calibration.rotation);
  EEPROM.commit();
}

inline bool loadCalibration() {
  uint16_t magic = EEPROM.readUShort(EEPROM_ADDR_MAGIC);
  
  if (magic != CALIBRATION_MAGIC) {
    calibration.valid = false;
    return false;
  }
  
  calibration.magic = magic;
  calibration.x_min = EEPROM.readUShort(EEPROM_ADDR_X_MIN);
  calibration.x_max = EEPROM.readUShort(EEPROM_ADDR_X_MAX);
  calibration.y_min = EEPROM.readUShort(EEPROM_ADDR_Y_MIN);
  calibration.y_max = EEPROM.readUShort(EEPROM_ADDR_Y_MAX);
  calibration.swap_xy = EEPROM.readBool(EEPROM_ADDR_SWAP_XY);
  
  // Read rotation, default to 0 if garbage value
  uint8_t rot = EEPROM.readByte(EEPROM_ADDR_ROTATION);
  calibration.rotation = (rot <= 3) ? rot : 0;
  
  calibration.valid = true;
  
  Serial.printf("[DEBUG] Raw EEPROM rotation byte: %d, using: %d\n", rot, calibration.rotation);
  
  return true;
}

inline void initTouchCalibration() {
  EEPROM.begin(EEPROM_SIZE);
  
  if (!loadCalibration()) {
    Serial.println("No calibration found, starting calibration...");
    if (performCalibration()) {
      saveCalibration();
      Serial.println("Calibration saved!");
    } else {
      Serial.println("Calibration failed, using defaults");
      // Set reasonable defaults for 3.5" CYD
      calibration.x_min = 300;
      calibration.x_max = 3700;
      calibration.y_min = 280;
      calibration.y_max = 3800;
      calibration.swap_xy = false;
      calibration.rotation = 0;
      calibration.valid = true;
    }
  } else {
    Serial.println("Loaded calibration from EEPROM");
    Serial.printf("X: %d - %d, Y: %d - %d, Swap: %d, Rotation: %d\n", 
                  calibration.x_min, calibration.x_max,
                  calibration.y_min, calibration.y_max,
                  calibration.swap_xy, calibration.rotation);
    Serial.printf("[DEBUG] Calibration valid: %d\n", calibration.valid);
  }
}

inline void resetCalibration() {
  EEPROM.writeUShort(EEPROM_ADDR_MAGIC, 0);
  EEPROM.commit();
  Serial.println("Calibration reset! Reboot to recalibrate.");
}

// Test calibration by showing touch points
inline void testCalibration() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("TOUCH TEST MODE", SCREEN_WIDTH/2, 20, 4);
  tft.drawCentreString("Touch anywhere to test", SCREEN_WIDTH/2, 60, 2);
  tft.drawCentreString("Long press to exit", SCREEN_WIDTH/2, 85, 2);
  
  unsigned long lastTouch = 0;
  unsigned long touchStart = 0;
  bool wasTouched = false;
  
  while (true) {
    if (ts.tirqTouched() && ts.touched()) {
      if (!wasTouched) {
        touchStart = millis();
        wasTouched = true;
      }
      
      // Long press to exit (2 seconds)
      if (millis() - touchStart > 2000) {
        tft.fillScreen(TFT_BLACK);
        tft.drawCentreString("Exiting test mode...", SCREEN_WIDTH/2, SCREEN_HEIGHT/2, 4);
        delay(1000);
        return;
      }
      
      TS_Point p = ts.getPoint();
      uint16_t rawX = p.x;
      uint16_t rawY = p.y;
      
      if (calibration.swap_xy) {
        uint16_t temp = rawX;
        rawX = rawY;
        rawY = temp;
      }
      
      int mappedX = map(rawX, calibration.x_min, calibration.x_max, 0, SCREEN_WIDTH);
      int mappedY = map(rawY, calibration.y_min, calibration.y_max, 0, SCREEN_HEIGHT);
      
      // Constrain to screen bounds
      mappedX = constrain(mappedX, 0, SCREEN_WIDTH - 1);
      mappedY = constrain(mappedY, 0, SCREEN_HEIGHT - 1);
      
      // Draw crosshair at touch point
      tft.fillCircle(mappedX, mappedY, 3, TFT_RED);
      
      // Show coordinates
      tft.fillRect(0, 110, SCREEN_WIDTH, 60, TFT_BLACK);
      tft.setTextColor(TFT_CYAN, TFT_BLACK);
      char buffer[64];
      sprintf(buffer, "Raw: %d, %d", p.x, p.y);
      tft.drawCentreString(buffer, SCREEN_WIDTH/2, 120, 2);
      sprintf(buffer, "Mapped: %d, %d", mappedX, mappedY);
      tft.drawCentreString(buffer, SCREEN_WIDTH/2, 145, 2);
      
      lastTouch = millis();
    } else {
      wasTouched = false;
    }
    
    delay(10);
  }
}

#endif // TOUCH_CALIBRATION_H
