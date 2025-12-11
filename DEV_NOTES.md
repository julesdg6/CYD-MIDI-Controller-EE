# Developer Notes - Creating New Modes

This guide explains how to add new interactive modes to the CYD MIDI Controller. Each mode is a self-contained module that provides a unique way to generate or control MIDI output.

## Table of Contents

- [Architecture Overview](#architecture-overview)
- [Step-by-Step Guide](#step-by-step-guide)
- [Code Examples](#code-examples)
- [Common Patterns](#common-patterns)
- [Testing Your Mode](#testing-your-mode)
- [Best Practices](#best-practices)

---

## Architecture Overview

### Project Structure

```
src/
├── CYD-MIDI-Controller.ino    # Main controller - menu, mode switching
├── common_definitions.h        # Shared types, colors, global state
├── ui_elements.h               # UI helpers (buttons, headers, etc.)
├── midi_utils.h                # MIDI communication functions
├── thread_manager.cpp          # Threading system for touch/MIDI
└── *_mode.h / *_mode.cpp       # Individual mode implementations
```

### Key Components

- **AppMode enum**: Defines all available modes (in `common_definitions.h`)
- **Mode files**: Header (.h) and optional implementation (.cpp) for each mode
- **Global state**: Shared BPM, MIDI channel, touch state
- **Threading**: Touch input and MIDI output run on separate threads

---

## Step-by-Step Guide

Follow these steps to add a new mode to the controller:

### Step 1: Add Mode to AppMode Enum

**File**: `src/common_definitions.h`

Add your new mode to the `AppMode` enum:

```cpp
enum AppMode {
  MENU,
  KEYBOARD,
  SEQUENCER,
  // ... existing modes ...
  YOUR_NEW_MODE  // Add your mode here
};
```

### Step 2: Create Mode Header File

**File**: `src/your_new_mode.h`

Create a header file with this structure:

```cpp
#ifndef YOUR_NEW_MODE_H
#define YOUR_NEW_MODE_H

#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"

// Mode state structure
struct YourModeState {
  // Add your mode-specific variables here
  int someParameter;
  bool isActive;
  // ... etc
};

// Global state instance
extern YourModeState yourModeState;

// Required function declarations
void initializeYourNewMode();  // Setup mode state
void drawYourNewMode();        // Draw UI
void handleYourNewMode();      // Handle touch & updates

// Optional helper functions
void yourHelperFunction();

#endif // YOUR_NEW_MODE_H
```

### Step 3: Implement Mode Functions

**Option A**: Simple mode - implement in header file (`.h`)

For simple modes with minimal logic, implement functions directly in the header:

```cpp
// After declarations, add implementations

void initializeYourNewMode() {
  yourModeState.someParameter = 0;
  yourModeState.isActive = false;
}

void drawYourNewMode() {
  tft.fillScreen(THEME_BG);
  drawHeader("YOUR MODE", "Subtitle");
  
  // Draw your UI elements
  drawRoundButton(10, 100, 100, 50, "BUTTON", THEME_PRIMARY);
}

void handleYourNewMode() {
  // Handle touch input
  if (touch.justPressed) {
    if (isButtonPressed(10, 100, 100, 50)) {
      // Handle button press
    }
    
    // Check for back button (typically top-right)
    if (isButtonPressed(425, 5, 50, 35)) {
      exitToMenu();
      return;
    }
  }
  
  // Update mode logic (animations, sequencing, etc.)
}
```

**Option B**: Complex mode - use separate implementation file (`.cpp`)

For complex modes, create `src/your_new_mode.cpp`:

```cpp
#include "your_new_mode.h"

YourModeState yourModeState;

void initializeYourNewMode() {
  // Initialization code
}

void drawYourNewMode() {
  // Drawing code
}

void handleYourNewMode() {
  // Handler code
}
```

### Step 4: Include Mode in Main File

**File**: `src/CYD-MIDI-Controller.ino`

Add include at the top (around line 22-36):

```cpp
// Include mode files
#include "keyboard_mode.h"
#include "sequencer_mode.h"
// ... existing includes ...
#include "your_new_mode.h"  // Add your include here
```

### Step 5: Add to Initialization

**File**: `src/CYD-MIDI-Controller.ino`

Add initialization call in `setup()` function (around line 746-758):

```cpp
void setup() {
  // ... existing setup code ...
  
  // Initialize mode systems
  initializeKeyboardMode();
  initializeSequencerMode();
  // ... existing initializations ...
  initializeYourNewMode();  // Add your initialization here
  
  drawMenu();
  updateStatus();
}
```

### Step 6: Add to Loop Handler

**File**: `src/CYD-MIDI-Controller.ino`

Add case to the switch statement in `loop()` function (around line 796-878):

```cpp
void loop() {
  // ... existing loop code ...
  
  switch (currentMode) {
    case MENU:
      // ... menu handling ...
      break;
    case KEYBOARD:
      handleKeyboardMode();
      break;
    // ... existing cases ...
    case YOUR_NEW_MODE:
      handleYourNewMode();  // Add your handler here
      break;
  }
}
```

### Step 7: Add to Mode Entry Handler

**File**: `src/CYD-MIDI-Controller.ino`

Add case to `enterMode()` function (around line 1204-1253):

```cpp
void enterMode(AppMode mode) {
  currentMode = mode;
  switch (mode) {
    case KEYBOARD:
      drawKeyboardMode();
      break;
    // ... existing cases ...
    case YOUR_NEW_MODE:
      drawYourNewMode();  // Add your draw call here
      break;
  }
  updateStatus();
}
```

### Step 8: Add Menu Icon

**File**: `src/CYD-MIDI-Controller.ino`

Add entry to the `apps[]` array (around line 109-125):

```cpp
AppIcon apps[] = {
  {"KEYS", "♪", 0x1A3D, KEYBOARD},
  // ... existing entries ...
  {"YOUR", "☆", 0xF800, YOUR_NEW_MODE}  // Add your icon
};
```

**Icon Configuration:**
- **name**: Short label (max ~6 chars for best display)
- **symbol**: Unicode emoji/symbol to display
- **color**: RGB565 color code (use online converter or existing colors)
- **mode**: Your AppMode enum value

### Step 9: Add Icon Graphics

**File**: `src/CYD-MIDI-Controller.ino`

Add case to `drawAppGraphics()` function (around line 927-1137):

```cpp
void drawAppGraphics(AppMode mode, int x, int y, int iconSize) {
  int topHalfY = y + iconSize/4;
  
  switch (mode) {
    case KEYBOARD:
      // ... existing graphics ...
      break;
    // ... other cases ...
    
    case YOUR_NEW_MODE:
      {
        int centerX = x + iconSize/2;
        // Draw your icon graphics here
        // Example: simple star
        tft.fillCircle(centerX, topHalfY, 8, THEME_BG);
        for (int i = 0; i < 5; i++) {
          float angle = (i * TWO_PI / 5) - HALF_PI;
          int px = centerX + cos(angle) * 12;
          int py = topHalfY + sin(angle) * 12;
          tft.drawLine(centerX, topHalfY, px, py, THEME_BG);
        }
      }
      break;
  }
}
```

### Step 10: Update App Count

**File**: `src/CYD-MIDI-Controller.ino`

Update the `numApps` variable (around line 127):

```cpp
int numApps = 16;  // Increment from 15 to 16 (or current count + 1)
```

---

## Code Examples

### Example 1: Simple Button-Based Mode

```cpp
#ifndef SIMPLE_MODE_H
#define SIMPLE_MODE_H

#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"

struct SimpleState {
  int currentNote;
  bool isPlaying;
};

SimpleState simpleState;

void initializeSimpleMode() {
  simpleState.currentNote = 60;  // Middle C
  simpleState.isPlaying = false;
}

void drawSimpleMode() {
  tft.fillScreen(THEME_BG);
  drawHeader("SIMPLE", "Example Mode");
  
  // Play button
  uint16_t playColor = simpleState.isPlaying ? THEME_ERROR : THEME_SUCCESS;
  String playText = simpleState.isPlaying ? "STOP" : "PLAY";
  drawRoundButton(190, 140, 100, 50, playText, playColor);
  
  // Back button
  drawRoundButton(425, 5, 50, 35, "BACK", THEME_PRIMARY);
}

void handleSimpleMode() {
  if (touch.justPressed) {
    // Play/Stop button
    if (isButtonPressed(190, 140, 100, 50)) {
      simpleState.isPlaying = !simpleState.isPlaying;
      if (simpleState.isPlaying) {
        sendNoteOn(simpleState.currentNote, 100);
      } else {
        sendNoteOff(simpleState.currentNote);
      }
      drawSimpleMode();
    }
    
    // Back button
    if (isButtonPressed(425, 5, 50, 35)) {
      if (simpleState.isPlaying) {
        sendNoteOff(simpleState.currentNote);
      }
      exitToMenu();
      return;
    }
  }
}

#endif
```

### Example 2: Animated Mode with Timer

```cpp
#ifndef ANIMATED_MODE_H
#define ANIMATED_MODE_H

#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"

struct AnimatedState {
  float ballX, ballY;
  float velocityX, velocityY;
  unsigned long lastUpdate;
};

AnimatedState animState;

void initializeAnimatedMode() {
  animState.ballX = SCREEN_WIDTH / 2;
  animState.ballY = SCREEN_HEIGHT / 2;
  animState.velocityX = 2.0;
  animState.velocityY = 1.5;
  animState.lastUpdate = millis();
}

void drawAnimatedMode() {
  tft.fillScreen(THEME_BG);
  drawHeader("ANIMATED", "Bouncing Ball");
  
  // Draw ball
  tft.fillCircle(animState.ballX, animState.ballY, 10, THEME_PRIMARY);
  
  // Back button
  drawRoundButton(425, 5, 50, 35, "BACK", THEME_PRIMARY);
}

void updateAnimation() {
  unsigned long now = millis();
  if (now - animState.lastUpdate < 16) return;  // ~60 FPS
  
  animState.lastUpdate = now;
  
  // Update position
  animState.ballX += animState.velocityX;
  animState.ballY += animState.velocityY;
  
  // Bounce off walls
  if (animState.ballX < 10 || animState.ballX > SCREEN_WIDTH - 10) {
    animState.velocityX = -animState.velocityX;
    // Play note on bounce
    sendNoteOn(60 + random(12), 80);
    sendNoteOff(60 + random(12));
  }
  
  if (animState.ballY < 50 || animState.ballY > SCREEN_HEIGHT - 10) {
    animState.velocityY = -animState.velocityY;
    sendNoteOn(72 + random(12), 80);
    sendNoteOff(72 + random(12));
  }
  
  // Redraw
  drawAnimatedMode();
}

void handleAnimatedMode() {
  if (touch.justPressed && isButtonPressed(425, 5, 50, 35)) {
    exitToMenu();
    return;
  }
  
  updateAnimation();
}

#endif
```

### Example 3: Sequencer-Style Mode

See `src/euclidean_mode.cpp` for a complete example of a sequencer mode with:
- Step-based timing
- Play/stop controls
- BPM synchronization
- Visual feedback
- Pattern generation

---

## Common Patterns

### Touch Input Handling

```cpp
void handleYourMode() {
  if (touch.justPressed) {
    int x = touch.x;
    int y = touch.y;
    
    // Check button presses
    if (isButtonPressed(x_pos, y_pos, width, height)) {
      // Handle button press
    }
    
    // Always include back button
    if (isButtonPressed(425, 5, 50, 35)) {
      // Cleanup before exiting
      exitToMenu();
      return;
    }
  }
}
```

### MIDI Output

```cpp
// Send Note On
sendNoteOn(midiNote, velocity);  // velocity: 0-127

// Send Note Off
sendNoteOff(midiNote);

// Send CC (Control Change)
sendCC(controllerNumber, value);  // both 0-127

// Send Pitch Bend
sendPitchBend(value);  // -8192 to +8191
```

### BPM-Based Timing

```cpp
// Access global BPM
float currentBPM = globalState.bpm;

// Calculate timing
unsigned long quarterNoteMs = (60000.0 / currentBPM);
unsigned long sixteenthNoteMs = quarterNoteMs / 4;

// Update on timer
unsigned long now = millis();
if (now - lastStepTime >= sixteenthNoteMs) {
  lastStepTime = now;
  // Trigger step
}
```

### UI Drawing

```cpp
// Fill screen
tft.fillScreen(THEME_BG);

// Draw header
drawHeader("TITLE", "Subtitle");

// Draw button
drawRoundButton(x, y, width, height, "LABEL", color);

// Draw text
tft.setTextColor(THEME_TEXT, THEME_BG);
tft.setTextSize(2);
tft.drawString("Text", x, y, fontNum);

// Draw shapes
tft.fillCircle(x, y, radius, color);
tft.fillRect(x, y, width, height, color);
tft.drawLine(x1, y1, x2, y2, color);
```

### State Persistence

If your mode needs to maintain state between activations:

```cpp
// Declare state at file scope (not in functions)
YourModeState yourState;

void initializeYourNewMode() {
  // Only set defaults on first initialization
  if (yourState.firstRun) {
    yourState.parameter = defaultValue;
    yourState.firstRun = false;
  }
}
```

---

## Testing Your Mode

### 1. Build and Upload

```bash
# Using PlatformIO
pio run -t upload -e cyd35

# Or use VS Code PlatformIO extension
# Click Upload button in bottom toolbar
```

### 2. Test Checklist

- [ ] Mode appears in main menu with correct icon
- [ ] Tapping icon enters the mode correctly
- [ ] UI elements are visible and properly positioned
- [ ] Touch targets are accurate (minimum 40x35 pixels)
- [ ] Back button returns to menu
- [ ] MIDI output works as expected
- [ ] No memory leaks or crashes
- [ ] Mode cleans up properly when exiting
- [ ] Works with both BLE connected and disconnected

### 3. Debug Output

Add serial debugging:

```cpp
Serial.println("Your mode initialized");
Serial.printf("Touch at (%d, %d)\n", touch.x, touch.y);
Serial.printf("Sending note %d\n", noteNumber);
```

View output:

```bash
pio device monitor --baud 115200
```

---

## Best Practices

### Design Guidelines

1. **Touch Targets**: Minimum 40x35 pixels for reliable touch detection
2. **Spacing**: Leave at least 5-10 pixels between interactive elements
3. **Back Button**: Always place at top-right (425, 5, 50, 35)
4. **Header**: Use `drawHeader()` for consistent title area
5. **Colors**: Use theme colors from `common_definitions.h`

### Performance

1. **Efficient Redraws**: Only redraw when necessary, not every loop
2. **Timing**: Use `millis()` for animations, not `delay()`
3. **MIDI Throttling**: Don't send MIDI faster than ~100Hz
4. **Memory**: Avoid large allocations in loop functions

### Code Organization

1. **Header Guards**: Always use `#ifndef` / `#define` / `#endif`
2. **Naming**: Use descriptive names: `yourModeName`, not `mode1`
3. **Comments**: Document complex algorithms or non-obvious behavior
4. **Const**: Use `const` for fixed values and lookup tables

### MIDI Best Practices

1. **Note Off**: Always send Note Off for every Note On
2. **Cleanup**: Send All Notes Off when exiting mode
3. **Channel**: Respect `globalState.currentMidiChannel`
4. **Velocity**: Use meaningful velocity values (40-127)

### Common Pitfalls

❌ **Don't**:
- Use `delay()` in main loop (blocks everything)
- Forget to handle back button
- Draw entire UI every loop iteration
- Send MIDI without checking BLE connection
- Use global variables without `extern` declaration

✅ **Do**:
- Use non-blocking timing with `millis()`
- Always include back button exit path
- Redraw only when state changes
- Check `globalState.bleConnected` before MIDI
- Properly declare state structures

---

## Reference: Existing Modes

Study these modes for examples:

- **keyboard_mode.h**: Basic button grid
- **euclidean_mode.cpp**: Sequencer with timing
- **bouncing_ball_mode.h**: Animation and collision
- **xy_pad_mode.h**: Continuous touch tracking
- **sequencer_mode.h**: Step sequencer pattern

---

## Questions or Issues?

- Check existing mode implementations for patterns
- Review `common_definitions.h` for available globals
- See `ui_elements.h` for UI helper functions
- Consult `midi_utils.h` for MIDI functions

Happy coding! 🎵
