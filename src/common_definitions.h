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

// Screen dimensions - automatically derived from TFT_WIDTH and TFT_HEIGHT
// These are defined in platformio.ini for each board variant:
//   - CYD 3.5" (ILI9488): TFT_WIDTH=320, TFT_HEIGHT=480 → Landscape: 480×320
//   - CYD 2.8" (ILI9341): TFT_WIDTH=240, TFT_HEIGHT=320 → Landscape: 320×240
//   - CYD 2.4" (ILI9341): TFT_WIDTH=240, TFT_HEIGHT=320 → Landscape: 320×240
// All UI elements should use SCREEN_WIDTH and SCREEN_HEIGHT for proper scaling
#if !defined(TFT_WIDTH) || !defined(TFT_HEIGHT)
  #error "TFT_WIDTH and TFT_HEIGHT must be defined in build flags (platformio.ini)"
#endif

// Convert portrait dimensions to landscape (swap if needed)
// TFT_eSPI defines WIDTH and HEIGHT in portrait mode
#if TFT_HEIGHT > TFT_WIDTH
  #define SCREEN_WIDTH   TFT_HEIGHT  // Use larger dimension for width (landscape)
  #define SCREEN_HEIGHT  TFT_WIDTH   // Use smaller dimension for height (landscape)
#else
  #define SCREEN_WIDTH   TFT_WIDTH
  #define SCREEN_HEIGHT  TFT_HEIGHT
#endif

#define CONTENT_TOP      50  // Below header

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
  GRID_PIANO,
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