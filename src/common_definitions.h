#ifndef COMMON_DEFINITIONS_H
#define COMMON_DEFINITIONS_H

#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <BLEDevice.h>

// Color scheme
#define THEME_BG         0x0841
#define THEME_SURFACE    0x2945
#define THEME_PRIMARY    0x06FF
#define THEME_SECONDARY  0xFD20
#define THEME_ACCENT     0x07FF
#define THEME_SUCCESS    0x07E0
#define THEME_WARNING    0xFFE0
#define THEME_ERROR      0xF800
#define THEME_TEXT       0xFFFF
#define THEME_TEXT_DIM   0x8410

// Screen dimensions - use TFT library defines if available, otherwise default to 480×320
// Note: TFT_WIDTH/HEIGHT are portrait orientation, we use landscape (swap them)
#ifndef SCREEN_WIDTH
  #ifdef TFT_HEIGHT
    #define SCREEN_WIDTH TFT_HEIGHT
  #else
    #define SCREEN_WIDTH 480
  #endif
#endif

#ifndef SCREEN_HEIGHT
  #ifdef TFT_WIDTH
    #define SCREEN_HEIGHT TFT_WIDTH
  #else
    #define SCREEN_HEIGHT 320
  #endif
#endif

#define CONTENT_TOP      50  // Below header

// UI Element Size Scaling
// Base dimensions are for 480x320 display (CYD 3.5")
// Minimum touch target sizes as recommended (45px for comfortable touch)
#define BASE_SCREEN_WIDTH   480
#define BASE_SCREEN_HEIGHT  320
#define MIN_BUTTON_WIDTH    45   // Minimum comfortable button width
#define MIN_BUTTON_HEIGHT   45   // Minimum comfortable button height

// Scaling factor calculation for responsive UI
// For 320x240 displays, scale = 0.67 (320/480)
#define SCALE_FACTOR_W  ((float)SCREEN_WIDTH / (float)BASE_SCREEN_WIDTH)
#define SCALE_FACTOR_H  ((float)SCREEN_HEIGHT / (float)BASE_SCREEN_HEIGHT)

// Scaled dimension helpers - use these for all UI element sizes
#define SCALED_W(w)  ((int)((w) * SCALE_FACTOR_W))
#define SCALED_H(h)  ((int)((h) * SCALE_FACTOR_H))

// Common scaled button sizes for consistency
#define BTN_SMALL_W     SCALED_W(60)
#define BTN_SMALL_H     SCALED_H(35)
#define BTN_MEDIUM_W    SCALED_W(80)
#define BTN_MEDIUM_H    SCALED_H(45)
#define BTN_LARGE_W     SCALED_W(100)
#define BTN_LARGE_H     SCALED_H(45)
#define BTN_BACK_W      SCALED_W(65)
#define BTN_BACK_H      SCALED_H(45)

// Scaled spacing constants
#define SPACING_SMALL   SCALED_W(5)
#define SPACING_MEDIUM  SCALED_W(10)
#define SPACING_LARGE   SCALED_W(20)

// Back button position constants (top-left of header) - full header height for reliability
#define BACK_BTN_X      0
#define BACK_BTN_Y      0

// BLE MIDI UUIDs
#define SERVICE_UUID        "03b80e5a-ede8-4b33-a751-6ce34ec4c700"
#define CHARACTERISTIC_UUID "7772e5db-3868-4112-a1a9-f2669d106bf3"

// Touch handling (must be defined BEFORE TouchThread)
struct TouchState {
  bool wasPressed = false;
  bool isPressed = false;
  bool justPressed = false;
  bool justReleased = false;
  int x = 0, y = 0;
};

// Global BPM (shared across all modules)
struct GlobalState {
  float bpm = 120.0;
  bool isPlaying = false;
  unsigned long lastBeatTime = 0;
  int currentMidiChannel = 1;  // 1-16
  bool bleConnected = false;  // Centralized BLE connection state
};
extern GlobalState globalState;

// MIDI Clock sync (keeps compatibility with existing code during migration)
struct MIDIClockSync {
  bool isReceiving = false;
  unsigned long lastClockTime = 0;
  unsigned long clockInterval = 0;
  float calculatedBPM = 120.0;  // Kept for compatibility, synced with globalState.bpm
  int clockCount = 0;
  bool isPlaying = false;  // Kept for compatibility, synced with globalState.isPlaying
  unsigned long lastBPMUpdate = 0;
};
extern MIDIClockSync midiClock;

// Touch event callback type
typedef void (*TouchCallback)(int x, int y, bool pressed);

// Touch thread manager
class TouchThread {
public:
  static void begin();
  static void update();
  static void registerCallback(TouchCallback callback);
  static void unregisterCallback();
  static TouchState getState();
  
private:
  static TouchCallback activeCallback;
  static TouchState currentState;
  static SemaphoreHandle_t touchMutex;
  static void touchTask(void* parameter);
};

// MIDI thread manager
class MIDIThread {
public:
  static void begin();
  static void sendNoteOn(uint8_t note, uint8_t velocity);
  static void sendNoteOff(uint8_t note, uint8_t velocity);
  static void sendCC(uint8_t controller, uint8_t value);
  static void sendPitchBend(int16_t value);
  static void sendClock();
  static void sendStart();
  static void sendStop();
  static void setBPM(float bpm);
  static float getBPM();
  
private:
  static QueueHandle_t midiQueue;
  static SemaphoreHandle_t midiMutex;
  static void midiTask(void* parameter);
  
  struct MIDIMessage {
    enum Type { NOTE_ON, NOTE_OFF, CC, PITCH_BEND, CLOCK, START, STOP } type;
    uint8_t data1;
    uint8_t data2;
    int16_t data16;
  };
};

// App modes
enum AppMode {
  MENU,
  KEYBOARD,
  SEQUENCER,
  BOUNCING_BALL,
  PHYSICS_DROP,
  RANDOM_GENERATOR,
  XY_PAD,
  ARPEGGIATOR,
  PADS,
  AUTO_CHORD,
  LFO,
  TB3PO,
  GRIDS,
  RAGA,
  EUCLIDEAN,
  MORPH
};

// Music theory
struct Scale {
  String name;
  int intervals[12];
  int numNotes;
};

// Global objects - declared in main file
extern TFT_eSPI tft;
extern XPT2046_Touchscreen ts;
extern BLECharacteristic *pCharacteristic;
// BLE connection state now in GlobalState (globalState.bleConnected)
extern uint8_t midiPacket[];
extern TouchState touch;
extern AppMode currentMode;

#endif