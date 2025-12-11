# UI Scaling System Documentation

## Overview

This project implements a dynamic UI scaling system that automatically adjusts button sizes, spacing, and layout positions based on the target display's resolution. This ensures consistent usability across all supported CYD (Cheap Yellow Display) devices.

## Supported Displays

- **CYD 3.5"** (ESP32-3248S035): 480×320 ILI9488 - Reference display (scale factor 1.0)
- **CYD 2.8"** (ESP32-2432S028R): 320×240 ILI9341 - Scaled (0.67×0.75)
- **CYD 2.4"** (ESP32-2432S024): 320×240 ILI9341 - Scaled (0.67×0.75)

## How It Works

### 1. Screen Dimension Detection

The system automatically detects screen dimensions from the TFT library build flags:

```cpp
// From common_definitions.h
#ifndef SCREEN_WIDTH
  #ifdef TFT_HEIGHT
    #define SCREEN_WIDTH TFT_HEIGHT  // Landscape orientation
  #else
    #define SCREEN_WIDTH 480         // Default fallback
  #endif
#endif
```

### 2. Scale Factor Calculation

Scale factors are computed at compile time based on the ratio to the reference display (480×320):

```cpp
#define BASE_SCREEN_WIDTH   480
#define BASE_SCREEN_HEIGHT  320
#define SCALE_FACTOR_W  ((float)SCREEN_WIDTH / (float)BASE_SCREEN_WIDTH)
#define SCALE_FACTOR_H  ((float)SCREEN_HEIGHT / (float)BASE_SCREEN_HEIGHT)
```

**Examples:**
- CYD 3.5" (480×320): `SCALE_FACTOR_W = 1.0`, `SCALE_FACTOR_H = 1.0`
- CYD 2.8" (320×240): `SCALE_FACTOR_W = 0.67`, `SCALE_FACTOR_H = 0.75`

### 3. Dimension Scaling Macros

All UI dimensions should use these macros:

```cpp
#define SCALED_W(w)  ((int)((w) * SCALE_FACTOR_W))
#define SCALED_H(h)  ((int)((h) * SCALE_FACTOR_H))
```

**Example usage:**
```cpp
// Old hardcoded approach (DON'T USE):
drawRoundButton(10, 50, 60, 35, "BACK", THEME_PRIMARY);

// New scaled approach (CORRECT):
drawRoundButton(SCALED_W(10), SCALED_H(50), BTN_SMALL_W, BTN_SMALL_H, "BACK", THEME_PRIMARY);
```

## Predefined Button Sizes

For consistency, use these predefined button size constants:

| Constant | Base Size (480×320) | Scaled (320×240) | Use Case |
|----------|---------------------|------------------|----------|
| `BTN_SMALL_W` | 60px | 40px | Compact buttons (OCT+/-) |
| `BTN_SMALL_H` | 35px | 26px | Compact button height |
| `BTN_MEDIUM_W` | 80px | 53px | Standard buttons (SCALE) |
| `BTN_MEDIUM_H` | 45px | 34px | Standard button height |
| `BTN_LARGE_W` | 100px | 67px | Primary actions |
| `BTN_LARGE_H` | 45px | 34px | Primary action height |
| `BTN_BACK_W` | 70px | 47px | Back button width |
| `BTN_BACK_H` | 35px | 26px | Back button height |

## Predefined Spacing

Use these spacing constants for consistent layouts:

```cpp
#define SPACING_SMALL   SCALED_W(5)   // 5px → 3px
#define SPACING_MEDIUM  SCALED_W(10)  // 10px → 7px
#define SPACING_LARGE   SCALED_W(20)  // 20px → 13px
```

## Common Patterns

### Back Button (Standardized Position)

All modes use consistent back button positioning:

```cpp
// Constants (automatically scaled)
#define BACK_BTN_X  SCALED_W(10)
#define BACK_BTN_Y  SCALED_H(5)

// Usage in mode handlers
if (touch.justPressed && isButtonPressed(BACK_BTN_X, BACK_BTN_Y, BTN_BACK_W, BTN_BACK_H)) {
  exitToMenu();
  return;
}
```

### Control Button Layout

Example from keyboard mode:

```cpp
void drawKeyboardMode() {
  int btnY = SCALED_H(240);
  drawRoundButton(SCALED_W(10), btnY, BTN_SMALL_W, BTN_MEDIUM_H, "OCT-", THEME_SECONDARY);
  drawRoundButton(SCALED_W(80), btnY, BTN_SMALL_W, BTN_MEDIUM_H, "OCT+", THEME_SECONDARY);
  drawRoundButton(SCALED_W(150), btnY, BTN_MEDIUM_W, BTN_MEDIUM_H, "SCALE", THEME_ACCENT);
}
```

### Grid/Cell Layouts

For grids that need to scale proportionally:

```cpp
void drawSequencerGrid() {
  int gridX = SCALED_W(10);
  int gridY = SCALED_H(55);
  int cellW = SCALED_W(27);
  int cellH = SCALED_H(42);
  int spacing = SCALED_W(2);
  
  for (int track = 0; track < SEQ_TRACKS; track++) {
    int y = gridY + track * (cellH + spacing + SCALED_H(3));
    // ... draw cells
  }
}
```

## Migration Guide

When updating existing code to use the scaling system:

### Step 1: Identify Hardcoded Dimensions

Look for numeric literals in:
- `drawRoundButton()` calls
- `isButtonPressed()` calls
- Position calculations (x, y coordinates)
- Size calculations (width, height)

### Step 2: Wrap X/Y Positions

```cpp
// Before
int x = 10;
int y = 240;

// After
int x = SCALED_W(10);
int y = SCALED_H(240);
```

### Step 3: Use Button Size Constants

```cpp
// Before
drawRoundButton(x, y, 60, 35, "Text", color);

// After
drawRoundButton(x, y, BTN_SMALL_W, BTN_SMALL_H, "Text", color);
```

### Step 4: Scale All Calculations

```cpp
// Before
int cellWidth = 45;
int spacing = 2;
int x = startX + col * (cellWidth + spacing);

// After
int cellWidth = SCALED_W(45);
int spacing = SCALED_W(2);
int x = startX + col * (cellWidth + spacing);
```

## Touch Target Guidelines

From UX research, the minimum comfortable touch target is ~45px. The scaling system ensures this is maintained:

- **480×320 display**: 45px (comfortable)
- **320×240 display**: ~30px (acceptable minimum)

If buttons become too small on lower resolutions, consider:
1. Reducing the number of visible elements
2. Using multi-screen navigation
3. Adjusting base button sizes

## Testing Across Displays

To test your changes on different displays:

1. Edit `platformio.ini` to select your target environment:
   ```ini
   [platformio]
   default_envs = cyd35  ; or cyd28, cyd24
   ```

2. Build for the target:
   ```bash
   pio run -e cyd35  # or cyd28, cyd24
   ```

3. Verify button sizes are usable and layouts don't overflow

## Files Updated

The scaling system is implemented in these key files:

- **`src/common_definitions.h`**: Core scaling constants and macros
- **`src/ui_elements.h`**: Scaled header and UI helper functions
- **`src/*_mode.h`**: All mode files updated with scaled dimensions
- **`src/CYD-MIDI-Controller.ino`**: Main menu and settings scaled

## Benefits

✅ **Automatic scaling**: No code changes needed for different displays  
✅ **Consistent UX**: Proportional layouts across all devices  
✅ **Maintainable**: Single source of truth for dimensions  
✅ **Type-safe**: Compile-time calculations, no runtime overhead  
✅ **Backward compatible**: 480×320 displays work identically (scale = 1.0)

## Future Enhancements

Potential improvements to the scaling system:

1. **Dynamic font sizing**: Scale font sizes based on display resolution
2. **Orientation detection**: Auto-adjust for portrait/landscape modes
3. **DPI awareness**: Consider physical screen size, not just resolution
4. **Layout presets**: Pre-calculated layouts for common screen sizes

## Questions?

For questions about the scaling system, refer to:
- This documentation
- Code comments in `src/common_definitions.h`
- Example implementations in mode files (e.g., `keyboard_mode.h`, `sequencer_mode.h`)
