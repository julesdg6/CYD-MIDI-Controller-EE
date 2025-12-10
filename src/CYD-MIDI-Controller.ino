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
#include <WiFi.h>
#include <WebServer.h>

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
#include "tb3po_mode.h"
#include "grids_mode.h"
#include "raga_mode.h"
#include "euclidean_mode.h"
#include "morph_mode.h"
#include "web_server.h"
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

#define MAX_APPS 15  // Can easily expand to 3x5 grid
// RGB565 optimized colors - 5 bits red, 6 bits green, 5 bits blue
AppIcon apps[] = {
  {"KEYS", "♪", 0x1A3D, KEYBOARD},     // Deep navy (row 1)
  {"BEATS", "♫", 0x3C1F, SEQUENCER},   // Royal blue
  {"ZEN", "●", 0x54BF, BOUNCING_BALL}, // Medium blue
  {"DROP", "⬇", 0x5EDF, PHYSICS_DROP}, // Light blue
  {"RNG", "※", 0x07FF, RANDOM_GENERATOR}, // Bright cyan
  {"XY PAD", "◈", 0x1C83, XY_PAD},     // Dark green (row 2)
  {"ARP", "↗", 0x2DC5, ARPEGGIATOR},   // Green
  {"GRID", "▣", 0x8E88, GRID_PIANO},   // Lime green
  {"CHORD", "⚘", 0xCF8A, AUTO_CHORD},  // Yellow-green
  {"LFO", "", 0xFFE0, LFO},            // Yellow
  {"TB3PO", "😊", 0xFD60, TB3PO},       // Amber (row 3) - acid smiley
  {"GRIDS", "◉", 0xFB20, GRIDS},       // Orange - MI Grids
  {"RAGA", "🎵", 0xF8A5, RAGA},         // Red-orange - Indian classical
  {"EUCLID", "◯", 0xF800, EUCLIDEAN},  // Pure red - Euclidean rhythm
  {"MORPH", "∞", 0x8800, MORPH}        // Dark red - Gesture morphing
};

int numApps = 15;

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
      // Restart advertising to allow reconnection
      delay(500); // Brief delay before restarting advertising
      BLEDevice::startAdvertising();
      Serial.println("BLE disconnected - advertising restarted for reconnection");
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

// Icon drawing functions moved to ui_elements.h for consistency

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
  
  // Initialize web server for file management
  Serial.println("\n=== WiFi Web Server Initialization ===");
  initializeWebServer();
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
  
  // Ensure SD card is unmounted first to avoid VFS registration errors
  SD.end();
  delay(100);
  
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
                     AUTO_CHORD, LFO, TB3PO, GRIDS, RAGA, EUCLIDEAN, MORPH};
  String modeNames[] = {"KEYBOARD", "SEQUENCER", "BOUNCING BALL", "PHYSICS DROP",
                        "RANDOM GEN", "XY PAD", "ARPEGGIATOR", "GRID PIANO",
                        "AUTO CHORD", "LFO", "TB3PO", "GRIDS", "RAGA", "EUCLIDEAN", "MORPH"};
  
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
  
  // Get WiFi IP address for URL display
  String ipAddr = WiFi.localIP().toString();
  
  // Now cycle through all 15 modes
  String fileNames[] = {"03_keyboard", "04_sequencer", "05_bouncing_ball", "06_physics_drop",
                        "07_random_gen", "08_xy_pad", "09_arpeggiator", "10_grid_piano",
                        "11_auto_chord", "12_lfo", "13_tb3po", "14_grids", "15_raga", 
                        "16_euclidean", "17_morph"};
  
  for (int i = 0; i < 15; i++) {
    // Show mode name and URL
    tft.fillRect(0, 120, 480, 60, THEME_SURFACE);
    tft.setTextColor(THEME_PRIMARY, THEME_SURFACE);
    tft.drawCentreString("Mode " + String(i+1) + "/15: " + modeNames[i], 240, 125, 4);
    tft.setTextColor(THEME_TEXT_DIM, THEME_SURFACE);
    String url = "http://" + ipAddr + "/download?file=/screenshots/" + fileNames[i] + ".bmp";
    tft.drawCentreString(url, 240, 160, 1);
    
    // Also print to serial for easy copying
    Serial.println(url);
    
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
  tft.drawCentreString("18 BMP files in /screenshots/", 240, 160, 2);
  tft.setTextColor(0x0000, THEME_SUCCESS);
  tft.drawCentreString("Menu + Settings + BLE + 15 Modes", 240, 190, 1);
  tft.drawCentreString("Download via web interface", 240, 210, 1);
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
  
  // Initialize global state
  globalState.bpm = 120.0;
  globalState.isPlaying = false;
  globalState.currentMidiChannel = 1;
  
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
  
  // Initialize thread managers (TODO: Complete migration to threaded architecture)
  // Serial.println("Starting Touch Thread...");
  // TouchThread::begin();
  // Serial.println("Starting MIDI Thread...");
  // MIDIThread::begin();
  // Serial.println("Thread managers initialized");
  
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
  initializeGridsMode();
  initializeRagaMode();
  initializeEuclideanMode();
  initializeMorphMode();
  
  drawMenu();
  updateStatus();
  Serial.println("MIDI Controller ready!");
  Serial.println("Touch settings cog in top-left to access configuration");
}

void loop() {
  // Update touch state (using existing calibration logic)
  updateTouch();
  
  // TODO: Switch to threaded touch input
  // touch = TouchThread::getState();
  
  // Handle web server requests
  handleWebServer();
  
  // Sync global state with MIDI clock (compatibility layer)
  if (midiClock.isReceiving) {
    globalState.bpm = midiClock.calculatedBPM;
    globalState.isPlaying = midiClock.isPlaying;
  }
  
  // Check MIDI clock timeout (stop receiving if no clock for 2 seconds)
  if (midiClock.isReceiving && (millis() - midiClock.lastBPMUpdate > 2000)) {
    midiClock.isReceiving = false;
    midiClock.isPlaying = false;
    globalState.isPlaying = false;
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
    case TB3PO:
      handleTB3POMode();
      break;
    case GRIDS:
      handleGridsMode();
      break;
    case RAGA:
      handleRagaMode();
      break;
    case EUCLIDEAN:
      handleEuclideanMode();
      break;
    case MORPH:
      handleMorphMode();
      break;
  }
  
  delay(20);
}

void drawMenu() {
  tft.fillScreen(THEME_BG);
  
  // Use unified header (settings icon, not back button)
  drawModuleHeader("MIDI CONTROLLER", false);
  
  // Subtitle under header
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawCentreString("Cheap Yellow Display", 240, 52, 2);
  
  // Dynamic grid layout - 5 icons per row with bigger graphics
  int iconSize = 85;   // Button size stays the same
  int spacing = 5;     // Reduced spacing between columns
  int rowSpacing = 2;  // Minimal spacing between rows to fit all 3 rows
  int cols = 5;        // 5 icons per row
  int startX = (480 - (cols * iconSize + (cols-1) * spacing)) / 2;
  int startY = 58;     // Start slightly higher to fit 3 rows
  
  // Draw all apps (now includes TB3PO as 11th app)
  for (int i = 0; i < numApps; i++) {
    int col = i % cols;
    int row = i / cols;
    int x = startX + col * (iconSize + spacing);
    int y = startY + row * (iconSize + rowSpacing);
    
    // App icon background
    uint16_t iconColor = apps[i].color;
    
    tft.fillRoundRect(x, y, iconSize, iconSize, 8, iconColor);
    tft.drawRoundRect(x, y, iconSize, iconSize, 8, THEME_TEXT);
    
    // Draw app-specific graphics in TOP half of button
    drawAppGraphics(apps[i].mode, x, y, iconSize);
    
    // Icon symbol in TOP half
    tft.setTextColor(TFT_BLACK, iconColor);
    tft.drawCentreString(apps[i].symbol, x + iconSize/2, y + iconSize/4, 2);
    
    // App name in BOTTOM half
    tft.setTextColor(TFT_BLACK, iconColor);
    tft.drawCentreString(apps[i].name, x + iconSize/2, y + (3 * iconSize/4) - 4, 2);
  }
}

void drawAppGraphics(AppMode mode, int x, int y, int iconSize) {
  int topHalfY = y + iconSize/4; // Center graphics in top half
  
  switch (mode) {
    case KEYBOARD: // KEYS - piano keys
      {
        int keyWidth = 8;
        int totalWidth = 5 * keyWidth + 4 * 2; // 5 keys + 4 gaps
        int startX = x + (iconSize - totalWidth) / 2;
        for (int i = 0; i < 5; i++) {
          tft.fillRect(startX + i*10, topHalfY - 12, keyWidth, 24, THEME_BG);
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
        tft.fillCircle(centerX - 16, topHalfY - 12, 4, THEME_BG);
        tft.fillCircle(centerX - 2, topHalfY - 6, 4, THEME_BG);
        tft.fillCircle(centerX + 14, topHalfY + 2, 4, THEME_BG);
        tft.fillCircle(centerX - 8, topHalfY + 12, 4, THEME_BG);
      }
      break;
    case XY_PAD: // XY PAD - crosshairs
      {
        int centerX = x + iconSize/2;
        int crossSize = 28;
        tft.drawFastHLine(centerX - crossSize/2, topHalfY, crossSize, THEME_BG);
        tft.drawFastVLine(centerX, topHalfY - crossSize/2, crossSize, THEME_BG);
        tft.fillCircle(centerX, topHalfY, 6, THEME_BG);
      }
      break;
    case ARPEGGIATOR: // ARP - ascending notes
      {
        int centerX = x + iconSize/2;
        for (int i = 0; i < 4; i++) {
          tft.fillCircle(centerX - 14 + i*10, topHalfY + 10 - i*6, 4, THEME_BG);
        }
      }
      break;
    case GRID_PIANO: // GRID - grid pattern
      {
        int cellW = 10, cellH = 8, gapX = 2, gapY = 4;
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
        int lineWidth = 28;
        tft.fillRect(centerX - lineWidth/2, topHalfY + 8, lineWidth, 4, THEME_BG);
        tft.fillRect(centerX - lineWidth/2, topHalfY, lineWidth, 4, THEME_BG);
        tft.fillRect(centerX - lineWidth/2, topHalfY - 8, lineWidth, 4, THEME_BG);
      }
      break;
    case LFO: // LFO - simple sine wave line
      {
        int centerX = x + iconSize/2;
        
        // Draw sine wave as connected line segments
        int lastX = centerX - 30;
        int lastY = topHalfY;
        
        for (int i = 1; i <= 15; i++) {
          int px = centerX - 30 + i * 4;
          float angle = (i * 3.14159) / 4.0; // One and a half cycles
          int py = topHalfY + (int)(12 * sin(angle));
          
          // Draw line from last point to current point
          tft.drawLine(lastX, lastY, px, py, THEME_BG);
          
          lastX = px;
          lastY = py;
        }
      }
      break;
    case TB3PO: // TB3PO - Acid smiley face (transparent outline)
      {
        int centerX = x + iconSize/2;
        // Draw circle face outline (2 pixels thick for visibility)
        tft.drawCircle(centerX, topHalfY, 18, THEME_BG);
        tft.drawCircle(centerX, topHalfY, 17, THEME_BG);
        // Eyes (filled dots)
        tft.fillCircle(centerX - 8, topHalfY - 5, 3, THEME_BG);
        tft.fillCircle(centerX + 8, topHalfY - 5, 3, THEME_BG);
        // Acid smiley mouth (wide smile - curves upward, thicker line)
        for (int i = -10; i <= 10; i++) {
          int y = topHalfY + 8 - (abs(i) * abs(i)) / 20;
          tft.drawPixel(centerX + i, y, THEME_BG);
          tft.drawPixel(centerX + i, y + 1, THEME_BG); // Make it thicker
        }
      }
      break;
    case GRIDS: // GRIDS - MI Grids drum pattern (concentric circles)
      {
        int centerX = x + iconSize/2;
        // Draw concentric circles representing drum pattern map
        tft.drawCircle(centerX, topHalfY, 16, THEME_BG);
        tft.drawCircle(centerX, topHalfY, 10, THEME_BG);
        tft.drawCircle(centerX, topHalfY, 4, THEME_BG);
        // Draw dots representing triggers in different positions
        tft.fillCircle(centerX, topHalfY - 10, 2, THEME_BG);
        tft.fillCircle(centerX + 8, topHalfY + 6, 2, THEME_BG);
        tft.fillCircle(centerX - 8, topHalfY + 6, 2, THEME_BG);
      }
      break;
    case RAGA: // RAGA - Indian classical music (sitar/tanpura shape)
      {
        int centerX = x + iconSize/2;
        // Draw sitar/tanpura body (gourd shape)
        tft.fillCircle(centerX, topHalfY + 6, 12, THEME_BG);
        // Neck
        tft.fillRect(centerX - 2, topHalfY - 18, 4, 24, THEME_BG);
        // Tuning pegs
        tft.drawLine(centerX - 2, topHalfY - 16, centerX - 8, topHalfY - 18, THEME_BG);
        tft.drawLine(centerX + 2, topHalfY - 16, centerX + 8, topHalfY - 18, THEME_BG);
        // Strings (vertical lines)
        tft.drawFastVLine(centerX - 4, topHalfY - 10, 16, THEME_BG);
        tft.drawFastVLine(centerX, topHalfY - 10, 16, THEME_BG);
        tft.drawFastVLine(centerX + 4, topHalfY - 10, 16, THEME_BG);
        // Bridge
        tft.drawFastHLine(centerX - 8, topHalfY + 10, 16, THEME_BG);
      }
      break;
    case EUCLIDEAN: // EUCLIDEAN - Euclidean rhythm (multi-ring circular pattern)
      {
        int centerX = x + iconSize/2;
        // Draw 4 concentric circles with dots (representing Euclidean patterns)
        tft.drawCircle(centerX, topHalfY, 18, THEME_BG);
        tft.drawCircle(centerX, topHalfY, 14, THEME_BG);
        tft.drawCircle(centerX, topHalfY, 10, THEME_BG);
        tft.drawCircle(centerX, topHalfY, 6, THEME_BG);
        // Event markers at different positions on each ring
        // Outer ring - 4 events
        for (int i = 0; i < 4; i++) {
          float angle = (i * TWO_PI / 4) - HALF_PI;
          tft.fillCircle(centerX + cos(angle) * 18, topHalfY + sin(angle) * 18, 2, THEME_BG);
        }
        // Mid-outer ring - 3 events
        for (int i = 0; i < 3; i++) {
          float angle = (i * TWO_PI / 3) - HALF_PI + 0.5;
          tft.fillCircle(centerX + cos(angle) * 14, topHalfY + sin(angle) * 14, 2, THEME_BG);
        }
        // Center dot
        tft.fillCircle(centerX, topHalfY, 2, THEME_BG);
      }
      break;
    case MORPH: // MORPH - Gesture morphing (infinity symbol with trail)
      {
        int centerX = x + iconSize/2;
        // Draw infinity symbol (lemniscate)
        for (float t = 0; t < TWO_PI; t += 0.1) {
          float scale = 16.0f;
          float ix = scale * cos(t) / (1 + sin(t) * sin(t));
          float iy = scale * sin(t) * cos(t) / (1 + sin(t) * sin(t));
          tft.drawPixel(centerX + (int)ix, topHalfY + (int)iy, THEME_BG);
        }
        // Add flowing particles
        for (int i = 0; i < 3; i++) {
          float t = (millis() / 500.0f + i * TWO_PI / 3.0f);
          float scale = 16.0f;
          float px = scale * cos(t) / (1 + sin(t) * sin(t));
          float py = scale * sin(t) * cos(t) / (1 + sin(t) * sin(t));
          tft.fillCircle(centerX + (int)px, topHalfY + (int)py, 2, THEME_BG);
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
  int spacing = 5;    // Match drawMenu spacing (reduced)
  int rowSpacing = 2; // Match drawMenu row spacing
  int cols = 5;       // 5 icons per row
  int startX = (480 - (cols * iconSize + (cols-1) * spacing)) / 2;
  int startY = 58;    // Match drawMenu startY
  
  for (int i = 0; i < numApps; i++) {
    int col = i % cols;
    int row = i / cols;
    int x = startX + col * (iconSize + spacing);
    int y = startY + row * (iconSize + rowSpacing);
    
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
    case TB3PO:
      initializeTB3POMode();
      break;
    case GRIDS:
      initializeGridsMode();
      break;
    case RAGA:
      initializeRagaMode();
      break;
    case EUCLIDEAN:
      initializeEuclideanMode();
      break;
    case MORPH:
      initializeMorphMode();
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
