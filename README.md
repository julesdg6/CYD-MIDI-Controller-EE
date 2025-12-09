# CYD MIDI Controller - Enhanced Edition

Advanced touchscreen Bluetooth MIDI controller for the ESP32-2432S028R "Cheap Yellow Display" (CYD).

> **Note**: This is a heavily modified fork of [NickCulbertson/CYD-MIDI-Controller](https://github.com/NickCulbertson/CYD-MIDI-Controller). We've significantly enhanced the UI/UX with larger touch targets, improved layouts, and better touch accuracy for capacitive touchscreens.

## Screenshots

> **Note**: Screenshots are captured as BMP files (compatible with SD card storage on the device). GitHub doesn't render BMP files, but you can view them by cloning the repo or checking the [assets/screenshots](assets/screenshots/) folder.

<div align="center">
  <img src="assets/screenshots/00_main_menu.bmp" width="400" alt="Main Menu">
  <img src="assets/screenshots/03_keyboard.bmp" width="400" alt="Keyboard Mode">
</div>

<details>
<summary>📸 View All Mode Screenshots</summary>

<div align="center">
  <img src="assets/screenshots/01_settings_menu.bmp" width="400" alt="Settings Menu"><br>
  <em>Settings Menu</em>
  
  <img src="assets/screenshots/04_sequencer.bmp" width="400" alt="Sequencer"><br>
  <em>16-Step Sequencer</em>
  
  <img src="assets/screenshots/08_xy_pad.bmp" width="400" alt="XY Pad"><br>
  <em>XY Pad Controller</em>
  
  <img src="assets/screenshots/09_arpeggiator.bmp" width="400" alt="Arpeggiator"><br>
  <em>Arpeggiator</em>
  
  <img src="assets/screenshots/10_grid_piano.bmp" width="400" alt="Grid Piano"><br>
  <em>Grid Piano (4ths Layout)</em>
  
  <img src="assets/screenshots/11_auto_chord.bmp" width="400" alt="Auto Chord"><br>
  <em>Auto Chord Mode</em>
  
  <img src="assets/screenshots/12_lfo.bmp" width="400" alt="LFO"><br>
  <em>LFO Modulator</em>
</div>

</details>

## Credits

- **Original Project**: [NickCulbertson/CYD-MIDI-Controller](https://github.com/NickCulbertson/CYD-MIDI-Controller)
- **Hardware Resources**: Brian Lough's [ESP32-Cheap-Yellow-Display](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display) repo

## Features

### 10 Interactive Modes

- **KEYS** - Virtual piano keyboard with scale and key controls
- **BEATS** - 16-step sequencer with 4 tracks and tempo control
- **ZEN** - Ambient bouncing ball mode for generative music
- **DROP** - Physics-based ball drop with customizable platforms
- **RNG** - Random music generator for creative exploration
- **XY PAD** - Touch-controlled X/Y pad for real-time parameter control
- **ARP** - Arpeggiator with chord-based patterns
- **GRID** - Grid piano with 4ths layout for unique playing style
- **CHORD** - Auto-chord mode with diatonic chord progressions
- **LFO** - Low-frequency oscillator for modulation effects

### Core Features

- **Bluetooth MIDI** - Wireless connection to DAWs and music software
- **Enhanced Touch UI** - Enlarged buttons (60-80px) and optimized layouts for capacitive touchscreens
- **Accurate Touch Detection** - Fixed coordinate mismatches between visual and touch layers
- **Real-time Control** - Low-latency MIDI output with configurable MIDI channels
- **Visual Feedback** - Responsive graphics with status icons (BLE, SD card, BPM display)
## What You Need

### Supported Hardware

- **CYD 3.5" (ESP32-3248S035)** - 480×320 ILI9488 - *Recommended* ✓
- **CYD 2.8" (ESP32-2432S028R)** - 320×240 ILI9341
- **CYD 2.4" (ESP32-2432S024)** - 320×240 ILI9341

All boards available for ~$15 from AliExpress/Amazon

### Additional

- **MicroSD Card** (optional) - For screenshot capture and future features
- **PlatformIO** or **Arduino IDE** with ESP32 support

- ✅ **Larger touch targets** - Minimum 45px buttons for better accuracy
- ✅ **Fixed touch detection** - Resolved Y-coordinate mismatches in all modes
- ✅ **Improved layouts** - Settings menu, Grid Piano (45×32 cells), Auto Chord, Arpeggiator
- ✅ **Better spacing** - No overlapping buttons, optimized for 480×320 display
- ✅ **Header icons** - BLE status, SD card indicator, and BPM display on main menu
- ✅ **MIDI channel support** - Configurable per-session MIDI channels (1-16)

## Installation

### Option A: PlatformIO (Recommended)

1. Install [PlatformIO](https://platformio.org/)
2. Clone this repo:
   ```bash
   git clone https://github.com/julesdg6/CYD-MIDI-Controller.git
   cd CYD-MIDI-Controller
   ```
3. Open in VS Code with PlatformIO extension
4. Select your board in `platformio.ini` (default: `cyd35`)
5. Click "Upload"

### Option B: Arduino IDE

#### 1. Add ESP32 Board Support

1. Go to `File` → `Preferences`
2. Add to "Additional Boards Manager URLs":
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
3. Go to `Tools` → `Board` → `Boards Manager`
4. Search "ESP32" and install "ESP32 by Espressif Systems"

#### 2. Install Libraries
In Arduino IDE Library Manager, install:
- `TFT_eSPI` by Bodmer
- `XPT2046_Touchscreen` by Paul Stoffregen

#### 3. Configure TFT_eSPI

Replace the `libraries/TFT_eSPI/User_Setup.h` with the `User_Setup.h` from the repo.

#### 4. Upload Code
1. Clone this repo and open `CYD_MIDI_Controller.ino`
2. Select board: `ESP32 Dev Module`
3. Connect CYD and upload
(Lower Upload Speed to `115200` if the sketch isn't uploading)

## Usage

### First Run - Touch Calibration

### Settings Menu

Access via the cog icon in the main menu:
- **Calibrate Touch** - Recalibrate touchscreen
- **MIDI Channel** - Change MIDI channel (1-16)
- **BLE Toggle** - Enable/disable Bluetooth advertising
- **Screenshot Mode** - Cycle through all modes and save screenshots to SD card

## Troubleshooting

- **Touch accuracy issues**: Run touchscreen calibration from Settings menu
- **Upload fails**: Lower upload speed to `115200` in PlatformIO or Arduino IDE
- **Blank screen**: Check TFT_eSPI pin configuration matches `User_Setup.h`
- **Touch not responding**: Verify XPT2046_Touchscreen library is installed
- **Bluetooth not pairing**: Toggle BLE off/on in Settings, restart device
- **SD card not detected**: Ensure FAT32 formatted card is properly inserted

## Development

### Building from Source

```bash
# Using PlatformIO
pio run -e cyd35

# Upload to device
pio run -t upload -e cyd35
```

### Project Structure

```
src/
├── CYD-MIDI-Controller.ino  # Main file
├── common_definitions.h      # Shared constants
├── ui_elements.h             # UI helper functions
├── midi_utils.h              # MIDI communication
└── *_mode.h                  # Individual mode implementations
```

## Contributing

Contributions welcome! This project has diverged significantly from the original, focusing on improved UX for capacitive touchscreens.

## Licenseting to Your DAW
1. Pair "CYD MIDI" via Bluetooth
2. Select as MIDI input in your DAW

## Troubleshooting

- **Upload Speed**: Lower it to `115200` if the sketch isn't uploading
- **Blank screen**: Check TFT_eSPI pin configuration
- **No touch**: Verify touchscreen library installation
- **No Bluetooth**: Restart device and re-pair

## License

Open source - see MIT license file for details.
