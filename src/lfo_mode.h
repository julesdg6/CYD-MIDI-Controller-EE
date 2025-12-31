#ifndef LFO_MODE_H
#define LFO_MODE_H

#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"

// LFO mode variables
struct LFOParams {
  float rate = 1.0;      // Hz (0.1 - 10.0)
  int amount = 64;       // 0-127
  int ccTarget = 1;      // CC number (0-127) or -1 for pitchwheel
  bool isRunning = false;
  float phase = 0.0;     // Current phase (0-2π)
  int waveform = 0;      // 0=Sine, 1=Triangle, 2=Square, 3=Sawtooth
  unsigned long lastUpdate = 0;
  int lastValue = 64;    // Last sent value
  bool pitchWheelMode = false; // Special mode for pitchwheel
};

LFOParams lfo;
String waveNames[] = {"SINE", "TRI", "SQR", "SAW"};

// Function declarations
void initializeLFOMode();
void drawLFOMode();
void handleLFOMode();
void drawLFOControls();
void updateLFO();
float calculateLFOValue();
void sendLFOValue(int value);
void drawWaveform();

// Implementations
void initializeLFOMode() {
  lfo.rate = 1.0;
  lfo.amount = 64;
  lfo.ccTarget = 1; // Modulation wheel by default
  lfo.isRunning = false;
  lfo.phase = 0.0;
  lfo.waveform = 0;
  lfo.lastUpdate = 0;
  lfo.lastValue = 64;
  lfo.pitchWheelMode = false;
  
  drawLFOMode();
}

void drawLFOMode() {
  tft.fillScreen(THEME_BG);
  drawModuleHeader("LFO MOD");
  
  drawLFOControls();
  drawWaveform();
}

void drawLFOControls() {
  int y = CONTENT_TOP + 10;
  int btnHeight = 45;
  int spacing = 5;
  
  // Play/Stop and Rate
  drawRoundButton(15, y, 80, btnHeight, lfo.isRunning ? "STOP" : "START", 
                 lfo.isRunning ? THEME_ERROR : THEME_SUCCESS);
  
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawString("Rate:", 110, y + 15, 2);
  tft.drawString(String(lfo.rate, 1) + "Hz", 180, y + 15, 2);
  drawRoundButton(260, y, 50, btnHeight, "-", THEME_SECONDARY);
  drawRoundButton(320, y, 50, btnHeight, "+", THEME_SECONDARY);
  
  // Waveform selector
  drawRoundButton(380, y, 90, btnHeight, waveNames[lfo.waveform], THEME_ACCENT);
  
  y += btnHeight + spacing + 5;
  
  // Amount
  tft.drawString("Amount:", 10, y + 15, 1);
  tft.drawString(String(lfo.amount), 60, y + 15, 1);
  drawRoundButton(85, y, 45, btnHeight, "-", THEME_SECONDARY);
  drawRoundButton(135, y, 45, btnHeight, "+", THEME_SECONDARY);
  
  // Amount bar
  int barW = 100;
  int barX = 190;
  tft.drawRect(barX, y + 18, barW, 10, THEME_TEXT_DIM);
  int fillW = (barW * lfo.amount) / 127;
  tft.fillRect(barX + 1, y + 19, fillW, 8, THEME_PRIMARY);
  
  y += btnHeight + spacing + 5;
  
  // Target selection
  tft.drawString("Target:", 10, y + 15, 1);
  if (lfo.pitchWheelMode) {
    tft.drawString("PITCH", 60, y + 15, 1);
  } else {
    tft.drawString("CC" + String(lfo.ccTarget), 60, y + 15, 1);
  }
  
  drawRoundButton(110, y, 45, btnHeight, "-", THEME_SECONDARY);
  drawRoundButton(160, y, 45, btnHeight, "+", THEME_SECONDARY);
  drawRoundButton(210, y, 80, btnHeight, "PITCH", lfo.pitchWheelMode ? THEME_PRIMARY : THEME_WARNING);
  
  y += btnHeight + spacing + 5;
  
  // Current value display
  tft.setTextColor(THEME_PRIMARY, THEME_BG);
  tft.drawString("Value: ", 10, y + 5, 1);
  tft.setTextColor(THEME_ACCENT, THEME_BG);
  tft.drawString(String(lfo.lastValue), 60, y + 5, 2);
  
  // Status indicator
  int indicatorX = SCREEN_WIDTH - 30;
  if (lfo.isRunning) {
    tft.fillCircle(250, y + 15, 8, THEME_SUCCESS);
    tft.drawCircle(250, y + 15, 8, THEME_TEXT);
  } else {
    tft.drawCircle(250, y + 15, 8, THEME_TEXT_DIM);
  }
}

void drawWaveform() {
  // Draw a mini waveform visualization - use available width
  int waveX = 10;
  int waveY = SCREEN_HEIGHT - 80;
  int waveW = SCREEN_WIDTH - 20;
  int waveH = 50;
  
  tft.drawRect(waveX, waveY, waveW, waveH, THEME_TEXT_DIM);
  
  // Draw waveform based on type
  for (int x = 0; x < waveW - 2; x++) {
    float phase = (x / (float)(waveW - 2)) * 2 * PI;
    float value = 0;
    
    switch (lfo.waveform) {
      case 0: // Sine
        value = sin(phase);
        break;
      case 1: // Triangle
        value = (phase <= PI) ? (2 * phase / PI - 1) : (3 - 2 * phase / PI);
        break;
      case 2: // Square
        value = (phase <= PI) ? 1 : -1;
        break;
      case 3: // Sawtooth
        value = 2 * phase / (2 * PI) - 1;
        break;
    }
    
    int y = waveY + waveH/2 - (value * waveH/4);
    tft.drawPixel(waveX + 1 + x, y, THEME_PRIMARY);
  }
  
  // Phase indicator removed per user request
}

void handleLFOMode() {
  // Back button - larger touch area
  if (touch.justPressed && isButtonPressed(BACK_BTN_X, BACK_BTN_Y, BTN_BACK_W, BTN_BACK_H)) {
    lfo.isRunning = false;
    exitToMenu();
    return;
  }
  
  // Calculate button positions for press feedback
  int y1 = 55;
  int y2 = y1 + 30;
  int y3 = y2 + 30;
  
  // Check button press states
  bool startPressed = touch.isPressed && isButtonPressed(15, y1, 80, 30);
  bool rateDownPressed = touch.isPressed && isButtonPressed(260, y1, 35, 30);
  bool rateUpPressed = touch.isPressed && isButtonPressed(305, y1, 35, 30);
  bool wavePressed = touch.isPressed && isButtonPressed(360, y1, 90, 30);
  
  bool amtDownPressed = touch.isPressed && isButtonPressed(85, y2, 25, 25);
  bool amtUpPressed = touch.isPressed && isButtonPressed(115, y2, 25, 25);
  
  bool tgtDownPressed = touch.isPressed && isButtonPressed(110, y3, 25, 25);
  bool tgtUpPressed = touch.isPressed && isButtonPressed(140, y3, 25, 25);
  bool pitchPressed = touch.isPressed && isButtonPressed(180, y3, 70, 25);
  
  // Only redraw buttons when pressed
  if (startPressed || rateDownPressed || rateUpPressed || wavePressed || amtDownPressed || amtUpPressed || tgtDownPressed || tgtUpPressed || pitchPressed) {
    drawRoundButton(15, y1, 80, 30, lfo.isRunning ? "STOP" : "START", 
                   lfo.isRunning ? THEME_ERROR : THEME_SUCCESS, startPressed);
    drawRoundButton(260, y1, 35, 30, "-", THEME_SECONDARY, rateDownPressed);
    drawRoundButton(305, y1, 35, 30, "+", THEME_SECONDARY, rateUpPressed);
    drawRoundButton(360, y1, 90, 30, waveNames[lfo.waveform], THEME_ACCENT, wavePressed);
    
    drawRoundButton(85, y2, 25, 25, "-", THEME_SECONDARY, amtDownPressed);
    drawRoundButton(115, y2, 25, 25, "+", THEME_SECONDARY, amtUpPressed);
    
    drawRoundButton(110, y3, 25, 25, "-", THEME_SECONDARY, tgtDownPressed);
    drawRoundButton(140, y3, 25, 25, "+", THEME_SECONDARY, tgtUpPressed);
    drawRoundButton(180, y3, 70, 25, "PITCH", lfo.pitchWheelMode ? THEME_PRIMARY : THEME_WARNING, pitchPressed);
  }
  
  if (touch.justPressed) {
    int y = 55;
    int btnHeight = 45;
    int spacing = 5;
    
    // Start/Stop
    if (isButtonPressed(15, y, 80, btnHeight)) {
      lfo.isRunning = !lfo.isRunning;
      if (lfo.isRunning) {
        lfo.phase = 0.0;
        lfo.lastUpdate = millis();
      }
      drawLFOMode();
      return;
    }
    
    // Rate controls
    if (isButtonPressed(260, y, 50, btnHeight)) {
      lfo.rate = max(0.1, lfo.rate - 0.1);
      drawLFOControls();
      return;
    }
    if (isButtonPressed(320, y, 50, btnHeight)) {
      lfo.rate = min(10.0, lfo.rate + 0.1);
      drawLFOControls();
      return;
    }
    
    // Waveform selector
    if (isButtonPressed(380, y, 90, btnHeight)) {
      lfo.waveform = (lfo.waveform + 1) % 4;
      drawLFOMode();
      return;
    }
    
    y += btnHeight + spacing + 5;
    
    // Amount controls
    if (isButtonPressed(85, y, 45, btnHeight)) {
      lfo.amount = max(0, lfo.amount - 5);
      drawLFOControls();
      return;
    }
    if (isButtonPressed(135, y, 45, btnHeight)) {
      lfo.amount = min(127, lfo.amount + 5);
      drawLFOControls();
      return;
    }
    
    y += btnHeight + spacing + 5;
    
    // Target controls
    if (isButtonPressed(110, y, 45, btnHeight)) {
      if (lfo.pitchWheelMode) {
        lfo.pitchWheelMode = false;
        lfo.ccTarget = 1; // Back to modulation wheel
      } else {
        lfo.ccTarget = max(0, lfo.ccTarget - 1);
      }
      drawLFOMode();
      return;
    }
    if (isButtonPressed(160, y, 45, btnHeight)) {
      if (!lfo.pitchWheelMode) {
        lfo.ccTarget = min(127, lfo.ccTarget + 1);
      }
      drawLFOMode();
      return;
    }
    
    // Pitchwheel mode toggle
    if (isButtonPressed(210, y, 80, btnHeight)) {
      lfo.pitchWheelMode = !lfo.pitchWheelMode;
      drawLFOMode();
      return;
    }
  }
  
  // Update LFO
  updateLFO();
}

void updateLFO() {
  if (!lfo.isRunning) return;
  
  unsigned long now = millis();
  float deltaTime = (now - lfo.lastUpdate) / 1000.0; // Convert to seconds
  lfo.lastUpdate = now;
  
  // Optionally sync rate to MIDI clock BPM (rate becomes multiplier)
  float effectiveRate = lfo.rate;
  if (midiClock.isReceiving && midiClock.calculatedBPM > 0) {
    // Sync LFO to clock - use rate as multiplier of BPM/60
    effectiveRate = (midiClock.calculatedBPM / 60.0) * lfo.rate;
  }
  
  // Update phase
  lfo.phase += 2 * PI * effectiveRate * deltaTime;
  while (lfo.phase >= 2 * PI) {
    lfo.phase -= 2 * PI;
  }
  
  // Calculate LFO value
  float lfoValue = calculateLFOValue();
  
  // Calculate output value based on target type
  int outputValue;
  if (lfo.pitchWheelMode) {
    // For pitch bend: 8192 center, scale by amount
    outputValue = 8192 + (lfoValue * lfo.amount * 64); // Scale for pitch bend range
    outputValue = constrain(outputValue, 0, 16383);
  } else {
    // For CC: 64 center, scale by amount  
    outputValue = 64 + (lfoValue * lfo.amount / 2);
    outputValue = constrain(outputValue, 0, 127);
  }
  
  // Send if value changed significantly (reduce MIDI spam)
  if (abs(outputValue - lfo.lastValue) >= 1) {
    sendLFOValue(outputValue);
    lfo.lastValue = outputValue;
    
    // Update display every few cycles
    static int displayUpdateCounter = 0;
    if (++displayUpdateCounter >= 10) {
      drawLFOControls();
      drawWaveform();
      displayUpdateCounter = 0;
    }
  }
}

float calculateLFOValue() {
  switch (lfo.waveform) {
    case 0: // Sine
      return sin(lfo.phase);
    case 1: // Triangle
      return (lfo.phase <= PI) ? (2 * lfo.phase / PI - 1) : (3 - 2 * lfo.phase / PI);
    case 2: // Square
      return (lfo.phase <= PI) ? 1 : -1;
    case 3: // Sawtooth
      return 2 * lfo.phase / (2 * PI) - 1;
    default:
      return 0;
  }
}

void sendLFOValue(int value) {
  if (!globalState.bleConnected) return;
  
  if (lfo.pitchWheelMode) {
    // Send pitchwheel (14-bit value already calculated)
    byte lsb = value & 0x7F;
    byte msb = (value >> 7) & 0x7F;
    
    // Pitchwheel: 0xE0, LSB, MSB
    midiPacket[2] = 0xE0;
    midiPacket[3] = lsb;
    midiPacket[4] = msb;
    pCharacteristic->setValue(midiPacket, 5);
    pCharacteristic->notify();
  } else {
    // Send regular CC
    sendControlChange(lfo.ccTarget, value);
  }
}

#endif