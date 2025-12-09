/*******************************************************************
 MIDI Controller Main Launcher for ESP32 Cheap Yellow Display
 Main file - handles setup, menu, and mode switching
 *******************************************************************/

#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <TFT_eSPI.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <SD.h>
#include <FS.h>

// Include calibration first (needs tft and ts)
#include "touch_calibration.h"

// Include mode files
#include "keyboard_mode.h"
#include "sequencer_mode.h"
#include "bouncing_ball_mode.h"
#include "physics_drop_mode.h"
#include "random_generator_mode.h"
#include "xy_pad_mode.h"
#include "arpeggiator_mode.h"
#include "grid_piano_mode.h"
#include "auto_chord_mode.h"
#include "lfo_mode.h"
#include "ui_elements.h"
#include "midi_utils.h"

// Hardware setup
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33
#define SD_CS 5    // SD card CS pin
#define SD_MOSI 23 // SD card uses different SPI pins than display
#define SD_MISO 19
#define SD_SCK 18

// SD card globals
bool sdCardAvailable = false;
uint64_t sdCardSize = 0;
uint64_t sdCardUsed = 0;

// Settings
uint8_t midiChannel = 1;  // MIDI channel 1-16
bool bleEnabled = true;

// Press tracking for 3-second hold
unsigned long pressStartTime = 0;
bool longPressTriggered = false;

// Screenshot counter
int screenshotCount = 0;

// Global objects
SPIClass mySpi = SPIClass(VSPI);  // Touch uses VSPI
SPIClass sdSPI = SPIClass(HSPI);  // SD card uses HSPI
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
TFT_eSPI tft = TFT_eSPI();

// BLE MIDI globals
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
uint8_t midiPacket[] = {0x80, 0x80, 0x00, 0x60, 0x7F};

// MIDI Clock sync
MIDIClockSync midiClock;

// Touch state
TouchState touch;

// App state
AppMode currentMode = MENU;

// Forward declarations
void drawMenu();
void showSettingsMenu(bool interactive = true);

// Scalable App Icon System
// To add new apps:
// 1. Add new mode to AppMode enum in common_definitions.h
// 2. Create mode header file (e.g., new_mode.h)
// 3. Include header in this file
// 4. Add to initialization, loop, and enterMode switch statements
// 5. Add entry to apps[] array below
// 6. Add graphics case to drawAppGraphics() function
// 7. Increment numApps
struct AppIcon {
  String name;
  String symbol;
  uint16_t color;
  AppMode mode;
};

#define MAX_APPS 12  // Can easily expand to 3x4 grid
AppIcon apps[] = {
  {"KEYS", "♪", 0xF800, KEYBOARD},     // Red
  {"BEATS", "♫", 0xFD00, SEQUENCER},   // Orange
  {"ZEN", "●", 0xFFE0, BOUNCING_BALL}, // Yellow
  {"DROP", "⬇", 0x07E0, PHYSICS_DROP}, // Green
  {"RNG", "※", 0x001F, RANDOM_GENERATOR}, // Blue
  {"XY PAD", "◈", 0x781F, XY_PAD},     // Purple
  {"ARP", "↗", 0xF81F, ARPEGGIATOR},   // Magenta
  {"GRID", "▣", 0x07FF, GRID_PIANO},   // Cyan
  {"CHORD", "⚘", 0xFBE0, AUTO_CHORD},  // Light Orange
  {"LFO", "", 0xAFE5, LFO}             // Light Green
};

int numApps = 10;

class MIDICallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      if (currentMode == MENU) {
        drawMenu(); // Redraw menu to clear "BLE WAITING..."
      }
      updateStatus();
    }
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      if (currentMode == MENU) {
        drawMenu(); // Redraw menu to show "BLE WAITING..."
      }
      updateStatus();
      // Stop all notes
      for (int i = 0; i < 128; i++) {
        sendMIDI(0x80, i, 0);
      }
    }
};

class MIDICharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      
      if (value.length() >= 3) {
        // BLE MIDI packet format: timestamp(2 bytes) + MIDI status + data...
        uint8_t status = value[2];
        
        // MIDI Clock (0xF8) - 24 ppqn (24 pulses per quarter note)
        if (status == 0xF8) {
          unsigned long now = millis();
          
          if (midiClock.lastClockTime > 0) {
            midiClock.clockInterval = now - midiClock.lastClockTime;
            
            // Calculate BPM: 24 clocks per quarter note
            // BPM = 60000 / (interval_ms * 24)
            if (midiClock.clockInterval > 0) {
              float bpm = 60000.0 / (midiClock.clockInterval * 24.0);
              
              // Smooth BPM calculation with averaging
              if (midiClock.clockCount > 24) {
                midiClock.calculatedBPM = (midiClock.calculatedBPM * 0.9) + (bpm * 0.1);
              } else {
                midiClock.calculatedBPM = bpm;
              }
            }
          }
          
          midiClock.lastClockTime = now;
          midiClock.clockCount++;
          midiClock.isReceiving = true;
          midiClock.lastBPMUpdate = now;
        }
        // MIDI Start (0xFA)
        else if (status == 0xFA) {
          midiClock.isPlaying = true;
          midiClock.clockCount = 0;
          Serial.println("MIDI Start received");
        }
        // MIDI Stop (0xFC)
        else if (status == 0xFC) {
          midiClock.isPlaying = false;
          Serial.println("MIDI Stop received");
        }
        // MIDI Continue (0xFB)
        else if (status == 0xFB) {
          midiClock.isPlaying = true;
          Serial.println("MIDI Continue received");
        }
      }
    }
};

void drawSettingsIcon(int x, int y) {
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

void drawSDCardIcon(int x, int y) {
  uint16_t color = sdCardAvailable ? THEME_SUCCESS : THEME_TEXT_DIM;
  tft.fillRoundRect(x, y, 20, 20, 2, color);
  tft.fillRect(x + 2, y + 2, 16, 6, THEME_SURFACE);
  tft.fillRect(x + 4, y + 12, 12, 6, THEME_BG);
  tft.fillRect(x + 16, y + 4, 2, 4, THEME_BG);
  if (!sdCardAvailable) {
    tft.setTextColor(THEME_ERROR, THEME_SURFACE);
    tft.drawString("NO", x - 15, y + 6, 1);
  }
}

void drawBluetoothIcon(int x, int y) {
  // Classic Bluetooth icon - blue when connected, gray when disconnected
  uint16_t color = deviceConnected ? 0x001F : THEME_TEXT_DIM; // Blue or gray
  int cx = x + 10;
  int cy = y + 10;
  
  // Classic Bluetooth symbol shape (more accurate)
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

void initSDCard() {
  Serial.println("\n=== SD Card Initialization ===");
  Serial.printf("SD pins - CS:%d MOSI:%d MISO:%d SCK:%d\n", SD_CS, SD_MOSI, SD_MISO, SD_SCK);
  
  // SD card uses HSPI with its own pins (separate from touch VSPI and display)
  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  
  if (!SD.begin(SD_CS, sdSPI, 1000000)) {
    Serial.println("SD.begin() failed");
    sdCardAvailable = false;
    return;
  }
  
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card detected");
    SD.end();
    sdCardAvailable = false;
    return;
  }
  
  Serial.print("Card Type: ");
  if (cardType == CARD_MMC) Serial.println("MMC");
  else if (cardType == CARD_SD) Serial.println("SDSC");
  else if (cardType == CARD_SDHC) Serial.println("SDHC");
  else Serial.println("UNKNOWN");
  
  sdCardSize = SD.cardSize() / (1024 * 1024);
  sdCardUsed = SD.usedBytes() / (1024 * 1024);
  
  Serial.printf("Size: %lluMB, Used: %lluMB\n", sdCardSize, sdCardUsed);
  Serial.println("SD Card ready!\n");
  
  sdCardAvailable = true;
  SD.end(); // Release SPI bus for display
}

void showSDCardInfo() {
  tft.fillScreen(THEME_BG);
  tft.setTextColor(THEME_PRIMARY, THEME_BG);
  tft.drawCentreString("SD CARD INFO", 240, 20, 4);
  
  int y = 70;
  int lineHeight = 25;
  
  if (!sdCardAvailable) {
    tft.setTextColor(THEME_ERROR, THEME_BG);
    tft.drawCentreString("NO SD CARD DETECTED", 240, y, 2);
    tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
    y += lineHeight * 2;
    tft.drawCentreString("Check card is inserted", 240, y, 2);
  } else {
    // Remount to get fresh stats
    SD.begin(SD_CS);
    uint64_t totalBytes = SD.totalBytes() / (1024 * 1024);
    uint64_t usedBytes = SD.usedBytes() / (1024 * 1024);
    uint64_t freeBytes = totalBytes - usedBytes;
    
    tft.setTextColor(THEME_TEXT, THEME_BG);
    String cardType = "UNKNOWN";
    uint8_t type = SD.cardType();
    if (type == CARD_MMC) cardType = "MMC";
    else if (type == CARD_SD) cardType = "SDSC";
    else if (type == CARD_SDHC) cardType = "SDHC";
    
    tft.drawString("Card Type:", 60, y, 2);
    tft.setTextColor(THEME_PRIMARY, THEME_BG);
    tft.drawString(cardType, 260, y, 2);
    y += lineHeight;
    
    tft.setTextColor(THEME_TEXT, THEME_BG);
    tft.drawString("Total Size:", 60, y, 2);
    tft.setTextColor(THEME_PRIMARY, THEME_BG);
    tft.drawString(String(totalBytes) + " MB", 260, y, 2);
    y += lineHeight;
    
    tft.setTextColor(THEME_TEXT, THEME_BG);
    tft.drawString("Used Space:", 60, y, 2);
    tft.setTextColor(THEME_WARNING, THEME_BG);
    tft.drawString(String(usedBytes) + " MB", 260, y, 2);
    y += lineHeight;
    
    tft.setTextColor(THEME_TEXT, THEME_BG);
    tft.drawString("Free Space:", 60, y, 2);
    tft.setTextColor(THEME_SUCCESS, THEME_BG);
    tft.drawString(String(freeBytes) + " MB", 260, y, 2);
    y += lineHeight * 1.5;
    
    // Usage bar
    int barWidth = 360;
    int barHeight = 20;
    int barX = (480 - barWidth) / 2;
    float usagePercent = (float)usedBytes / (float)totalBytes;
    
    tft.drawRect(barX, y, barWidth, barHeight, THEME_PRIMARY);
    int fillWidth = (int)(barWidth * usagePercent);
    uint16_t barColor = usagePercent > 0.9 ? THEME_ERROR : 
                       (usagePercent > 0.7 ? THEME_WARNING : THEME_SUCCESS);
    if (fillWidth > 2) tft.fillRect(barX + 1, y + 1, fillWidth - 2, barHeight - 2, barColor);
    
    y += barHeight + 10;
    tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
    tft.drawCentreString(String((int)(usagePercent * 100)) + "% used", 240, y, 2);
    
    SD.end(); // Release SPI
  }
  
  // Back button
  drawRoundButton(190, 260, 100, 35, "BACK", THEME_PRIMARY);
  
  // Wait for back button
  while (true) {
    updateTouch();
    if (touch.justPressed && isButtonPressed(190, 260, 100, 35)) {
      drawMenu();
      return;
    }
    delay(20);
  }
}

void saveScreenshot(String filename) {
  if (!sdCardAvailable) {
    Serial.println("Cannot save screenshot: SD card not available");
    return;
  }
  
  // Mount SD card
  if (!SD.begin(SD_CS, sdSPI)) {
    Serial.println("SD card mount failed for screenshot");
    return;
  }
  
  // Create screenshots directory if it doesn't exist
  if (!SD.exists("/screenshots")) {
    SD.mkdir("/screenshots");
  }
  
  String filepath = "/screenshots/" + filename + ".bmp";
  File file = SD.open(filepath, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to create screenshot file: " + filepath);
    SD.end();
    return;
  }
  
  Serial.println("Saving screenshot: " + filepath);
  
  // BMP file format header for 24-bit RGB
  uint32_t imageSize = SCREEN_WIDTH * SCREEN_HEIGHT * 3;
  uint32_t fileSize = imageSize + 54; // 54 bytes header
  
  // BMP Header (14 bytes)
  file.write('B'); file.write('M');  // Signature
  file.write((uint8_t)(fileSize & 0xFF));
  file.write((uint8_t)((fileSize >> 8) & 0xFF));
  file.write((uint8_t)((fileSize >> 16) & 0xFF));
  file.write((uint8_t)((fileSize >> 24) & 0xFF));
  file.write(0); file.write(0);  // Reserved
  file.write(0); file.write(0);  // Reserved
  file.write(54); file.write(0); file.write(0); file.write(0);  // Pixel data offset
  
  // DIB Header (40 bytes)
  file.write(40); file.write(0); file.write(0); file.write(0);  // Header size
  file.write((uint8_t)(SCREEN_WIDTH & 0xFF));
  file.write((uint8_t)((SCREEN_WIDTH >> 8) & 0xFF));
  file.write((uint8_t)((SCREEN_WIDTH >> 16) & 0xFF));
  file.write((uint8_t)((SCREEN_WIDTH >> 24) & 0xFF));
  file.write((uint8_t)(SCREEN_HEIGHT & 0xFF));
  file.write((uint8_t)((SCREEN_HEIGHT >> 8) & 0xFF));
  file.write((uint8_t)((SCREEN_HEIGHT >> 16) & 0xFF));
  file.write((uint8_t)((SCREEN_HEIGHT >> 24) & 0xFF));
  file.write(1); file.write(0);  // Color planes
  file.write(24); file.write(0);  // Bits per pixel (24-bit RGB)
  file.write(0); file.write(0); file.write(0); file.write(0);  // Compression (none)
  file.write((uint8_t)(imageSize & 0xFF));
  file.write((uint8_t)((imageSize >> 8) & 0xFF));
  file.write((uint8_t)((imageSize >> 16) & 0xFF));
  file.write((uint8_t)((imageSize >> 24) & 0xFF));
  file.write(0); file.write(0); file.write(0); file.write(0);  // X pixels per meter
  file.write(0); file.write(0); file.write(0); file.write(0);  // Y pixels per meter
  file.write(0); file.write(0); file.write(0); file.write(0);  // Colors in palette
  file.write(0); file.write(0); file.write(0); file.write(0);  // Important colors
  
  // Write pixel data (bottom to top, BGR format)
  uint16_t pixelData;
  uint8_t r, g, b;
  
  for (int y = SCREEN_HEIGHT - 1; y >= 0; y--) {
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      pixelData = tft.readPixel(x, y);
      // Convert RGB565 to RGB888
      r = ((pixelData >> 11) & 0x1F) << 3;  // 5 bits red
      g = ((pixelData >> 5) & 0x3F) << 2;   // 6 bits green
      b = (pixelData & 0x1F) << 3;          // 5 bits blue
      // BMP uses BGR order
      file.write(b);
      file.write(g);
      file.write(r);
    }
  }
  
  file.close();
  SD.end();
  
  // Brief visual feedback
  tft.fillCircle(460, 300, 10, THEME_SUCCESS);
  delay(100);
  
  Serial.println("Screenshot saved: " + filepath + " (" + String(fileSize) + " bytes)");
}

void cycleModesForScreenshots() {
  // Cycle through all modes for visual inspection and screenshot capture
  tft.fillScreen(THEME_BG);
  tft.setTextColor(THEME_PRIMARY, THEME_BG);
  tft.drawCentreString("CYCLING MODES", 240, 20, 4);
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawCentreString("Saving screenshots to SD", 240, 60, 2);
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawCentreString("Hold 3 seconds to skip", 240, 90, 2);
  
  AppMode modes[] = {KEYBOARD, SEQUENCER, BOUNCING_BALL, PHYSICS_DROP, 
                     RANDOM_GENERATOR, XY_PAD, ARPEGGIATOR, GRID_PIANO, 
                     AUTO_CHORD, LFO};
  String modeNames[] = {"KEYBOARD", "SEQUENCER", "BOUNCING BALL", "PHYSICS DROP",
                        "RANDOM GEN", "XY PAD", "ARPEGGIATOR", "GRID PIANO",
                        "AUTO CHORD", "LFO"};
  
  // First capture main menu
  tft.fillRect(0, 120, 480, 40, THEME_SURFACE);
  tft.setTextColor(THEME_PRIMARY, THEME_SURFACE);
  tft.drawCentreString("Capturing: MAIN MENU", 240, 130, 4);
  delay(1000);
  drawMenu();
  delay(1000);
  saveScreenshot("00_main_menu");
  delay(500);
  
  // Capture settings menu
  tft.fillRect(0, 120, 480, 40, THEME_SURFACE);
  tft.setTextColor(THEME_PRIMARY, THEME_SURFACE);
  tft.drawCentreString("Capturing: SETTINGS", 240, 130, 4);
  delay(1000);
  showSettingsMenu(false);
  delay(1000);
  saveScreenshot("01_settings_menu");
  delay(500);
  
  // Capture bluetooth menu
  tft.fillRect(0, 120, 480, 40, THEME_SURFACE);
  tft.setTextColor(THEME_PRIMARY, THEME_SURFACE);
  tft.drawCentreString("Capturing: BLUETOOTH", 240, 130, 4);
  delay(1000);
  tft.fillScreen(THEME_BG);
  tft.setTextColor(THEME_PRIMARY, THEME_BG);
  tft.drawCentreString("BLUETOOTH STATUS", 240, 60, 4);
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawCentreString(deviceConnected ? "Connected" : "Waiting for connection", 240, 120, 2);
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawCentreString("Device: CYD MIDI", 240, 160, 2);
  String mac = BLEDevice::getAddress().toString().c_str();
  tft.drawCentreString("MAC: " + mac, 240, 190, 2);
  drawRoundButton(190, 240, 100, 35, "BACK", THEME_PRIMARY);
  delay(1000);
  saveScreenshot("02_bluetooth_status");
  delay(500);
  
  // Now cycle through all 10 modes
  String fileNames[] = {"03_keyboard", "04_sequencer", "05_bouncing_ball", "06_physics_drop",
                        "07_random_gen", "08_xy_pad", "09_arpeggiator", "10_grid_piano",
                        "11_auto_chord", "12_lfo"};
  
  for (int i = 0; i < 10; i++) {
    // Show mode name
    tft.fillRect(0, 120, 480, 40, THEME_SURFACE);
    tft.setTextColor(THEME_PRIMARY, THEME_SURFACE);
    tft.drawCentreString("Mode " + String(i+1) + "/10: " + modeNames[i], 240, 130, 4);
    
    delay(1500);
    
    // Enter mode
    enterMode(modes[i]);
    
    // Wait 2 seconds for mode to fully render, then capture
    delay(2000);
    saveScreenshot(fileNames[i]);
    
    // Wait additional time or until touch held to skip
    unsigned long startTime = millis();
    unsigned long touchStartTime = 0;
    bool skipMode = false;
    bool wasTouching = false;
    while (millis() - startTime < 3000 && !skipMode) {
      updateTouch();
      if (touch.isPressed) {
        if (!wasTouching) {
          touchStartTime = millis();
          wasTouching = true;
        } else if (millis() - touchStartTime > 3000) {
          skipMode = true;
        }
      } else {
        wasTouching = false;
      }
      delay(50);
    }
    
    // Give visual feedback if skipped
    if (skipMode) {
      tft.fillScreen(THEME_WARNING);
      tft.setTextColor(THEME_BG, THEME_WARNING);
      tft.drawCentreString("SKIPPED", 240, 150, 4);
      delay(500);
    }
  }
  
  // Done
  tft.fillScreen(THEME_SUCCESS);
  tft.setTextColor(THEME_BG, THEME_SUCCESS);
  tft.drawCentreString("SCREENSHOTS SAVED", 240, 120, 4);
  tft.drawCentreString("13 BMP files in /screenshots/", 240, 160, 2);
  tft.setTextColor(0x0000, THEME_SUCCESS);
  tft.drawCentreString("Menu + Settings + BLE + 10 Modes", 240, 190, 1);
  tft.drawCentreString("Ready to view on your computer", 240, 210, 1);
  delay(3000);
  
  // Return to menu
  exitToMenu();
}

void showSettingsMenu(bool interactive) {
  tft.fillScreen(THEME_BG);
  tft.setTextColor(THEME_PRIMARY, THEME_BG);
  tft.drawCentreString("SETTINGS", 240, 12, 4);
  
  int btnY = 50;
  int btnH = 45;  // Good size for touch without being too large
  int btnW = 440;
  int btnX = 20;
  int spacing = 6;
  
  // Calibrate Touch button
  drawRoundButton(btnX, btnY, btnW, btnH, "CALIBRATE TOUCH", THEME_PRIMARY);
  btnY += btnH + spacing;
  
  // MIDI Channel setting
  drawRoundButton(btnX, btnY, 140, btnH, "CH -", THEME_WARNING);
  drawRoundButton(btnX + 150, btnY, 140, btnH, "CH: " + String(midiChannel), THEME_SUCCESS);
  drawRoundButton(btnX + 300, btnY, 140, btnH, "CH +", THEME_WARNING);
  btnY += btnH + spacing;
  
  // BLE Enable/Disable
  String bleText = bleEnabled ? "BLE: ON" : "BLE: OFF";
  uint16_t bleColor = bleEnabled ? THEME_SUCCESS : THEME_ERROR;
  drawRoundButton(btnX, btnY, btnW, btnH, bleText, bleColor);
  btnY += btnH + spacing;
  
  // Screenshot Mode Cycling
  drawRoundButton(btnX, btnY, btnW, btnH, "SCREENSHOT ALL MODES", 0x07FF);
  btnY += btnH + spacing;
  
  // Back button - centered at bottom
  drawRoundButton(180, 270, 120, 45, "BACK", THEME_PRIMARY);
  
  // If not interactive (screenshot mode), just return
  if (!interactive) return;
  
  // Handle touch
  while (true) {
    updateTouch();
    if (!touch.justPressed) {
      delay(20);
      continue;
    }
    
    // Calibrate Touch (y=50)
    if (isButtonPressed(btnX, 50, btnW, btnH)) {
      resetCalibration();
      if (performCalibration()) {
        saveCalibration();
      }
      showSettingsMenu();
      return;
    }
    
    // MIDI Channel - (y=101: 50+45+6)
    if (isButtonPressed(btnX, 101, 140, btnH)) {
      if (midiChannel > 1) midiChannel--;
      showSettingsMenu();
      return;
    }
    
    // MIDI Channel + (y=101: same row)
    if (isButtonPressed(btnX + 300, 101, 140, btnH)) {
      if (midiChannel < 16) midiChannel++;
      showSettingsMenu();
      return;
    }
    
    // BLE Toggle (y=152: 101+45+6)
    if (isButtonPressed(btnX, 152, btnW, btnH)) {
      bleEnabled = !bleEnabled;
      if (bleEnabled) {
        BLEDevice::startAdvertising();
        Serial.println("BLE advertising enabled");
      } else {
        BLEDevice::stopAdvertising();
        Serial.println("BLE advertising disabled");
      }
      showSettingsMenu();
      return;
    }
    
    // Screenshot Mode Cycling (y=203: 152+45+6)
    if (isButtonPressed(btnX, 203, btnW, btnH)) {
      cycleModesForScreenshots();
      showSettingsMenu();
      return;
    }
    
    // Back button
    if (isButtonPressed(180, 270, 120, 45)) {
      drawMenu();
      return;
    }
    
    delay(20);
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n\nCYD MIDI Controller Starting...");
  
  // Touch setup
  mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin();
  ts.setRotation(1);
  
  // Display setup
  tft.init();
  tft.setRotation(1);
  pinMode(27, OUTPUT);
  digitalWrite(27, HIGH);
  
  // Initialize touch calibration (will auto-calibrate if needed)
  initTouchCalibration();
  
  // Initialize SD card (after display is ready)
  initSDCard();
  
  // Re-initialize touch SPI after SD card to ensure it still works
  mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin();
  Serial.println("Touch re-initialized after SD card");
  
  // BLE MIDI Setup
  Serial.println("Initializing BLE MIDI...");
  BLEDevice::init("CYD MIDI");
  Serial.println("BLE Device initialized");
  
  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new MIDICallbacks());
  Serial.println("BLE Server created");
  
  BLEService *service = server->createService(BLEUUID(SERVICE_UUID));
  Serial.println("BLE Service created");
  
  pCharacteristic = service->createCharacteristic(
    BLEUUID(CHARACTERISTIC_UUID),
    BLECharacteristic::PROPERTY_READ | 
    BLECharacteristic::PROPERTY_WRITE | 
    BLECharacteristic::PROPERTY_NOTIFY | 
    BLECharacteristic::PROPERTY_WRITE_NR
  );
  
  pCharacteristic->setCallbacks(new MIDICharacteristicCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());
  service->start();
  Serial.println("BLE Service started - MIDI Clock sync enabled");
  
  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(BLEUUID(SERVICE_UUID));
  advertising->setScanResponse(true);
  advertising->setMinPreferred(0x06);  // Functions that help with iPhone connections issue
  advertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("BLE Advertising started - Device discoverable as 'CYD MIDI'");
  Serial.printf("BLE MAC Address: %s\n", BLEDevice::getAddress().toString().c_str());
  
  // Initialize mode systems
  initializeKeyboardMode();
  initializeSequencerMode();
  initializeBouncingBallMode();
  initializeRandomGeneratorMode();
  initializeXYPadMode();
  initializeArpeggiatorMode();
  initializeGridPianoMode();
  initializeAutoChordMode();
  initializeLFOMode();
  
  drawMenu();
  updateStatus();
  Serial.println("MIDI Controller ready!");
  Serial.println("Touch settings cog in top-left to access configuration");
}

void loop() {
  updateTouch();
  
  // Check MIDI clock timeout (stop receiving if no clock for 2 seconds)
  if (midiClock.isReceiving && (millis() - midiClock.lastBPMUpdate > 2000)) {
    midiClock.isReceiving = false;
    midiClock.isPlaying = false;
    Serial.println("MIDI Clock timeout");
  }
  
  switch (currentMode) {
    case MENU:
      // Handle taps first (including cog icon for settings)
      if (touch.justPressed) {
        handleMenuTouch();
        // Reset long press tracking when a tap is handled
        longPressTriggered = false;
      }
      // Check for 3-second hold in top-left corner for calibration (backup method)
      else if (touch.isPressed && touch.x < 40 && touch.y < 40 && !longPressTriggered) {
        if (pressStartTime == 0) {
          pressStartTime = millis();
        } else if (millis() - pressStartTime > 3000) {
          // Trigger calibration after 3 seconds of holding
          longPressTriggered = true;
          tft.fillScreen(THEME_WARNING);
          tft.setTextColor(THEME_BG, THEME_WARNING);
          tft.drawCentreString("CALIBRATION MODE", 240, 140, 4);
          delay(500);
          resetCalibration();
          if (performCalibration()) {
            saveCalibration();
          }
          drawMenu();
          // Wait for release
          while (touch.isPressed) {
            updateTouch();
            delay(20);
          }
          pressStartTime = 0;
        }
      } else if (!touch.isPressed) {
        // Reset on release
        pressStartTime = 0;
        longPressTriggered = false;
      }
      break;
    case KEYBOARD:
      handleKeyboardMode();
      break;
    case SEQUENCER:
      handleSequencerMode();
      break;
    case BOUNCING_BALL:
      handleBouncingBallMode();
      break;
    case PHYSICS_DROP:
      handlePhysicsDropMode();
      break;
    case RANDOM_GENERATOR:
      handleRandomGeneratorMode();
      break;
    case XY_PAD:
      handleXYPadMode();
      break;
    case ARPEGGIATOR:
      handleArpeggiatorMode();
      break;
    case GRID_PIANO:
      handleGridPianoMode();
      break;
    case AUTO_CHORD:
      handleAutoChordMode();
      break;
    case LFO:
      handleLFOMode();
      break;
  }
  
  delay(20);
}

void drawMenu() {
  tft.fillScreen(THEME_BG);
  
  // Header
  tft.fillRect(0, 0, 480, 50, THEME_SURFACE);
  tft.drawFastHLine(0, 50, 480, THEME_PRIMARY);
  tft.setTextColor(THEME_PRIMARY, THEME_SURFACE);
  
  // Title closer to left (moved from center to make space)
  tft.drawString("MIDI CONTROLLER", 50, 12, 4);
  
  // Show BPM if receiving MIDI clock
  if (midiClock.isReceiving) {
    tft.setTextColor(THEME_SUCCESS, THEME_SURFACE);
    String bpmText = String((int)midiClock.calculatedBPM) + " BPM";
    tft.drawString(bpmText, 290, 17, 4);
  }
  
  tft.setTextColor(THEME_TEXT_DIM, THEME_SURFACE);
  tft.drawString("Cheap Yellow Display", 50, 32, 2);
  
  // Settings cog icon in top left (matches touch area at 10,10)
  drawSettingsIcon(10, 10);
  
  // Bluetooth icon next to SD card
  drawBluetoothIcon(410, 10);
  
  // SD Card icon in top right
  drawSDCardIcon(445, 10);
  
  // Dynamic grid layout - 5 icons per row with larger touch targets
  int iconSize = 85;  // Increased from 70 for easier tapping
  int spacing = 10;   // Reduced spacing to fit larger icons
  int cols = 5;  // 5 icons per row
  int rows = 3;  // Fixed to 3 rows now (2 active + 1 blank)
  int startX = (480 - (cols * iconSize + (cols-1) * spacing)) / 2;
  int startY = 60;  // Start slightly higher
  
  // Draw existing apps in first 2 rows
  for (int i = 0; i < numApps; i++) {
    int col = i % cols;
    int row = i / cols;
    int x = startX + col * (iconSize + spacing);
    int y = startY + row * (iconSize + spacing);
    
    // App icon background
    uint16_t iconColor = apps[i].color;
    
    tft.fillRoundRect(x, y, iconSize, iconSize, 8, iconColor);
    tft.drawRoundRect(x, y, iconSize, iconSize, 8, THEME_TEXT);
    
    // Draw app-specific graphics in TOP half of button
    drawAppGraphics(apps[i].mode, x, y, iconSize);
    
    // Icon symbol in TOP half
    tft.setTextColor(THEME_BG, iconColor);
    tft.drawCentreString(apps[i].symbol, x + iconSize/2, y + iconSize/4, 2);
    
    // App name in BOTTOM half
    tft.setTextColor(THEME_TEXT, iconColor);
    tft.drawCentreString(apps[i].name, x + iconSize/2, y + (3 * iconSize/4) - 4, 2);
  }
  
  // Draw blank buttons in third row (black with white outline)
  int blankRow = 2;
  for (int col = 0; col < cols; col++) {
    int x = startX + col * (iconSize + spacing);
    int y = startY + blankRow * (iconSize + spacing);
    
    tft.fillRoundRect(x, y, iconSize, iconSize, 8, TFT_BLACK);
    tft.drawRoundRect(x, y, iconSize, iconSize, 8, TFT_WHITE);
  }
}

void drawAppGraphics(AppMode mode, int x, int y, int iconSize) {
  int topHalfY = y + iconSize/4; // Center graphics in top half
  
  switch (mode) {
    case KEYBOARD: // KEYS - piano keys
      {
        int keyWidth = 4;
        int totalWidth = 5 * keyWidth + 4 * 1; // 5 keys + 4 gaps
        int startX = x + (iconSize - totalWidth) / 2;
        for (int i = 0; i < 5; i++) {
          tft.fillRect(startX + i*5, topHalfY - 6, keyWidth, 12, THEME_BG);
        }
      }
      break;
    case SEQUENCER: // BEATS - grid pattern
      {
        int gridW = 4, gridH = 4, gapX = 2, gapY = 2;
        int totalW = 4 * gridW + 3 * gapX;
        int totalH = 3 * gridH + 2 * gapY;
        int startX = x + (iconSize - totalW) / 2;
        int startYPos = topHalfY - totalH / 2;
        for (int r = 0; r < 3; r++) {
          for (int c = 0; c < 4; c++) {
            tft.fillRect(startX + c*(gridW+gapX), startYPos + r*(gridH+gapY), gridW, gridH, THEME_BG);
          }
        }
      }
      break;
    case BOUNCING_BALL: // ZEN - circle with dots
      {
        int centerX = x + iconSize/2;
        tft.drawCircle(centerX, topHalfY, 12, THEME_BG);
        tft.fillCircle(centerX - 6, topHalfY - 4, 2, THEME_BG);
        tft.fillCircle(centerX + 5, topHalfY + 2, 2, THEME_BG);
        tft.fillCircle(centerX - 2, topHalfY + 6, 2, THEME_BG);
      }
      break;
    case PHYSICS_DROP: // DROP - balls falling on platforms
      {
        int centerX = x + iconSize/2;
        // Draw platforms
        tft.fillRect(centerX - 10, topHalfY + 8, 8, 2, THEME_BG);
        tft.fillRect(centerX + 4, topHalfY + 4, 6, 2, THEME_BG);
        // Draw falling balls
        tft.fillCircle(centerX - 6, topHalfY - 8, 2, THEME_BG);
        tft.fillCircle(centerX + 2, topHalfY - 4, 2, THEME_BG);
        tft.fillCircle(centerX + 8, topHalfY, 2, THEME_BG);
      }
      break;
    case RANDOM_GENERATOR: // RNG - random dots
      {
        int centerX = x + iconSize/2;
        tft.fillCircle(centerX - 8, topHalfY - 6, 2, THEME_BG);
        tft.fillCircle(centerX - 1, topHalfY - 3, 2, THEME_BG);
        tft.fillCircle(centerX + 7, topHalfY + 1, 2, THEME_BG);
        tft.fillCircle(centerX - 4, topHalfY + 6, 2, THEME_BG);
      }
      break;
    case XY_PAD: // XY PAD - crosshairs
      {
        int centerX = x + iconSize/2;
        int crossSize = 14;
        tft.drawFastHLine(centerX - crossSize/2, topHalfY, crossSize, THEME_BG);
        tft.drawFastVLine(centerX, topHalfY - crossSize/2, crossSize, THEME_BG);
        tft.fillCircle(centerX, topHalfY, 3, THEME_BG);
      }
      break;
    case ARPEGGIATOR: // ARP - ascending notes
      {
        int centerX = x + iconSize/2;
        for (int i = 0; i < 4; i++) {
          tft.fillCircle(centerX - 7 + i*5, topHalfY + 5 - i*3, 2, THEME_BG);
        }
      }
      break;
    case GRID_PIANO: // GRID - grid pattern
      {
        int cellW = 5, cellH = 4, gapX = 1, gapY = 2;
        int totalW = 4 * cellW + 3 * gapX;
        int totalH = 3 * cellH + 2 * gapY;
        int startX = x + (iconSize - totalW) / 2;
        int startYPos = topHalfY - totalH / 2;
        for (int r = 0; r < 3; r++) {
          for (int c = 0; c < 4; c++) {
            tft.drawRect(startX + c*(cellW+gapX), startYPos + r*(cellH+gapY), cellW, cellH, THEME_BG);
          }
        }
      }
      break;
    case AUTO_CHORD: // CHORD - stacked notes
      {
        int centerX = x + iconSize/2;
        int lineWidth = 14;
        tft.fillRect(centerX - lineWidth/2, topHalfY + 4, lineWidth, 2, THEME_BG);
        tft.fillRect(centerX - lineWidth/2, topHalfY, lineWidth, 2, THEME_BG);
        tft.fillRect(centerX - lineWidth/2, topHalfY - 4, lineWidth, 2, THEME_BG);
      }
      break;
    case LFO: // LFO - simple sine wave line
      {
        int centerX = x + iconSize/2;
        
        // Draw sine wave as connected line segments
        int lastX = centerX - 15;
        int lastY = topHalfY;
        
        for (int i = 1; i <= 15; i++) {
          int px = centerX - 15 + i * 2;
          float angle = (i * 3.14159) / 4.0; // One and a half cycles
          int py = topHalfY + (int)(6 * sin(angle));
          
          // Draw line from last point to current point
          tft.drawLine(lastX, lastY, px, py, THEME_BG);
          
          lastX = px;
          lastY = py;
        }
      }
      break;
  }
}

void handleMenuTouch() {
  Serial.printf("Menu touch at: (%d,%d)\n", touch.x, touch.y);
  
  // Check settings icon touch (top left) - extra large touch area for easier access
  if (isButtonPressed(5, 5, 50, 50)) {
    Serial.println("Settings icon tapped!");
    showSettingsMenu();
    return;
  }
  
  // Check Bluetooth icon touch - larger touch area
  if (isButtonPressed(405, 10, 35, 35)) {
    // Show BLE status
    tft.fillScreen(THEME_BG);
    tft.setTextColor(THEME_PRIMARY, THEME_BG);
    tft.drawCentreString("BLUETOOTH STATUS", 240, 60, 4);
    tft.setTextColor(THEME_TEXT, THEME_BG);
    tft.drawCentreString(deviceConnected ? "Connected" : "Waiting for connection", 240, 120, 2);
    tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
    tft.drawCentreString("Device: CYD MIDI", 240, 160, 2);
    String mac = BLEDevice::getAddress().toString().c_str();
    tft.drawCentreString("MAC: " + mac, 240, 190, 2);
    drawRoundButton(190, 240, 100, 35, "BACK", THEME_PRIMARY);
    while (true) {
      updateTouch();
      if (touch.justPressed && isButtonPressed(190, 240, 100, 35)) {
        drawMenu();
        return;
      }
      delay(20);
    }
  }
  
  // Check SD icon touch (top right) - larger touch area
  if (isButtonPressed(440, 10, 35, 35)) {
    showSDCardInfo();
    return;
  }
  
  int iconSize = 85;  // Match drawMenu icon size (updated for better touch)
  int spacing = 10;   // Match drawMenu spacing
  int cols = 5;       // 5 icons per row
  int startX = (480 - (cols * iconSize + (cols-1) * spacing)) / 2;
  int startY = 60;    // Match drawMenu startY
  
  for (int i = 0; i < numApps; i++) {
    int col = i % cols;
    int row = i / cols;
    int x = startX + col * (iconSize + spacing);
    int y = startY + row * (iconSize + spacing);
    
    if (isButtonPressed(x, y, iconSize, iconSize)) {
      Serial.printf("Menu touch: app %d (%s) at (%d,%d) touch=(%d,%d)\n", 
                    i, apps[i].name.c_str(), x, y, touch.x, touch.y);
      enterMode(apps[i].mode);
      return;
    }
  }
  
  // Debug: show where the touch was
  Serial.printf("Menu touch missed all icons: (%d,%d)\n", touch.x, touch.y);
}

void enterMode(AppMode mode) {
  currentMode = mode;
  switch (mode) {
    case KEYBOARD:
      drawKeyboardMode();
      break;
    case SEQUENCER:
      drawSequencerMode();
      break;
    case BOUNCING_BALL:
      drawBouncingBallMode();
      break;
    case PHYSICS_DROP:
      drawPhysicsDropMode();
      break;
    case RANDOM_GENERATOR:
      drawRandomGeneratorMode();
      break;
    case XY_PAD:
      drawXYPadMode();
      break;
    case ARPEGGIATOR:
      drawArpeggiatorMode();
      break;
    case GRID_PIANO:
      drawGridPianoMode();
      break;
    case AUTO_CHORD:
      drawAutoChordMode();
      break;
    case LFO:
      drawLFOMode();
      break;
  }
  updateStatus();
}

void exitToMenu() {
  currentMode = MENU;
  stopAllModes();
  drawMenu();
  updateStatus();
}
