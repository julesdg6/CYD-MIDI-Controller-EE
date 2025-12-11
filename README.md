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
<summary>📸 View All Mode Screenshots (15 Modes)</summary>

### Menu & System
<div align="center">
  <img src="assets/screenshots/01_settings_menu.bmp" width="400" alt="Settings Menu"><br>
  <em>Settings Menu - Calibration, MIDI, BLE, Screenshots</em>
  
  <img src="assets/screenshots/02_bluetooth_status.bmp" width="400" alt="Bluetooth Status"><br>
  <em>Bluetooth Status - Connection & MAC Address</em>
</div>

### Original 10 Modes
<div align="center">
  <img src="assets/screenshots/03_keyboard.bmp" width="400" alt="Keyboard"><br>
  <em>Virtual Keyboard - Scales & Key Selection</em>
  
  <img src="assets/screenshots/04_sequencer.bmp" width="400" alt="Sequencer"><br>
  <em>16-Step Sequencer - 4 Tracks</em>
  
  <img src="assets/screenshots/05_bouncing_ball.bmp" width="400" alt="Bouncing Ball"><br>
  <em>Bouncing Ball - Generative Ambient</em>
  
  <img src="assets/screenshots/06_physics_drop.bmp" width="400" alt="Physics Drop"><br>
  <em>Physics Drop - Gravity Simulation</em>
  
  <img src="assets/screenshots/07_random_gen.bmp" width="400" alt="Random Generator"><br>
  <em>Random Generator - Controlled Chaos</em>
  
  <img src="assets/screenshots/08_xy_pad.bmp" width="400" alt="XY Pad"><br>
  <em>XY Pad - Real-time Expression</em>
  
  <img src="assets/screenshots/09_arpeggiator.bmp" width="400" alt="Arpeggiator"><br>
  <em>Arpeggiator - Chord Patterns</em>
  
  <img src="assets/screenshots/10_grid_piano.bmp" width="400" alt="Grid Piano"><br>
  <em>Grid Piano - 4ths Layout</em>
  
  <img src="assets/screenshots/11_auto_chord.bmp" width="400" alt="Auto Chord"><br>
  <em>Auto Chord - Diatonic Progressions</em>
  
  <img src="assets/screenshots/12_lfo.bmp" width="400" alt="LFO"><br>
  <em>LFO Modulator - Waveform Control</em>
</div>

### New Advanced Modes
<div align="center">
  <img src="assets/screenshots/13_tb3po.bmp" width="400" alt="TB3PO"><br>
  <em>TB3PO 😊 - Acid Bassline Generator</em>
  
  <img src="assets/screenshots/14_grids.bmp" width="400" alt="Grids"><br>
  <em>GRIDS ◉ - Euclidean Drum Sequencer</em>
  
  <img src="assets/screenshots/15_raga.bmp" width="400" alt="Raga"><br>
  <em>RAGA 🎵 - Indian Classical Music</em>
  
  <img src="assets/screenshots/16_euclidean.bmp" width="400" alt="Euclidean"><br>
  <em>EUCLID ◯ - Mathematical Rhythms</em>
  
  <img src="assets/screenshots/17_morph.bmp" width="400" alt="Morph"><br>
  <em>MORPH ∞ - Gesture Synthesis</em>
</div>

</details>

## Credits

- **Original Project**: [NickCulbertson/CYD-MIDI-Controller](https://github.com/NickCulbertson/CYD-MIDI-Controller)
- **Hardware Resources**: Brian Lough's [ESP32-Cheap-Yellow-Display](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display) repo

## Features

### 15 Interactive Modes

#### Original Modes
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

#### New Advanced Modes
- **TB3PO** 😊 - Acid bassline generator inspired by the TB-303 with probabilistic sequencing
- **GRIDS** ◉ - Euclidean drum sequencer based on Mutable Instruments Grids topology
- **RAGA** 🎵 - Indian classical music mode with authentic ragas and microtonal support
- **EUCLID** ◯ - Pure Euclidean rhythm generator with mathematical pattern distribution
- **MORPH** ∞ - Gesture-based morphing synthesizer for expressive performance

### Core Features

- **Bluetooth MIDI** - Wireless connection to DAWs and music software
- **WiFi Web Server** - Remote file management and screenshot capture via web browser
- **Enhanced Touch UI** - Enlarged buttons (60-80px) and optimized layouts for capacitive touchscreens
- **Accurate Touch Detection** - Fixed coordinate mismatches between visual and touch layers
- **Real-time Control** - Low-latency MIDI output with configurable MIDI channels
- **Visual Feedback** - Responsive graphics with status icons (BLE, SD card, BPM display)
- **Screenshot Capture** - Save all mode screens to SD card or download via web interface
- **Persistent WiFi Config** - Store WiFi credentials on SD card for automatic connection
## What You Need

### Supported Hardware

- **CYD 3.5" (ESP32-3248S035)** - 480×320 ILI9488 - *Recommended* ✓
- **CYD 2.8" (ESP32-2432S028R)** - 320×240 ILI9341
- **CYD 2.4" (ESP32-2432S024)** - 320×240 ILI9341

All boards available for ~$15 from AliExpress/Amazon

> **📌 Pin Map:** Complete pin assignments for display, touch, SD card, and available expansion pins are documented in [BUILD.md - Pin Map Reference](BUILD.md#pin-map-reference)

### Additional

- **MicroSD Card** (optional) - For screenshot capture, WiFi config storage, and web file management
- **PlatformIO** or **Arduino IDE** with ESP32 support
- **WiFi Network** (optional) - For web server and remote control features
- **Serial MIDI (future)** - Available GPIOs for M5 MIDI breakout or DIN-5 connector (see pin map)

### Recent Enhancements

- ✅ **Multi-resolution support** - Automatic screen scaling for 480×320, 320×240 displays
- ✅ **Larger touch targets** - Minimum 45px buttons for better accuracy
- ✅ **Fixed touch detection** - Resolved Y-coordinate mismatches in all modes
- ✅ **Improved layouts** - Settings menu, Grid Piano (45×32 cells), Auto Chord, Arpeggiator
- ✅ **Better spacing** - No overlapping buttons, optimized layouts
- ✅ **Header icons** - BLE status, SD card indicator, and BPM display on main menu
- ✅ **MIDI channel support** - Configurable per-session MIDI channels (1-16)
- ✅ **Expanded flash memory** - 3.1MB app partition for advanced features
- ✅ **5 new advanced modes** - TB3PO, Grids, Raga, Euclidean, Morph
- ✅ **Web server** - Remote file management and screenshot downloads
- ✅ **WiFi persistence** - Store network credentials on SD card

## Installation

### Option A: PlatformIO (Recommended)

1. Install [PlatformIO](https://platformio.org/)
2. Clone this repo:
   ```bash
   git clone https://github.com/julesdg6/CYD-MIDI-Controller-EE.git
   cd CYD-MIDI-Controller-EE
   ```
3. Open in VS Code with PlatformIO extension
4. **Select your CYD board** in `platformio.ini`:
   - Edit line 3 to uncomment the environment for your display:
     - `default_envs = cyd35` for 3.5" (480×320) - *Default*
     - `default_envs = cyd28` for 2.8" (320×240)
     - `default_envs = cyd24` for 2.4" (320×240)
5. Click "Upload"

**Note**: Screen dimensions are automatically configured based on your board selection. All UI elements scale to match your display size.

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

On first boot, the device will automatically start touch calibration. Follow the on-screen prompts to touch each crosshair accurately. **Important**: If you change display size (e.g., from 3.5" to 2.8"), you must re-run calibration from the Settings menu for accurate touch detection.

### Settings Menu

Access via the cog icon in the main menu:
- **Calibrate Touch** - Recalibrate touchscreen coordinates
- **MIDI Channel** - Change MIDI channel (1-16)
- **BLE Toggle** - Enable/disable Bluetooth advertising
- **Screenshot Mode** - Cycle through all 15 modes and save screenshots to SD card
- **Web Server** - Automatically starts on WiFi connection (configurable via SD card)

### Web Server Interface

Access at `http://[device-ip]` when connected to WiFi:
- **File Browser** - Navigate SD card directories, upload/download/delete files
- **Screenshot Capture** - Take instant screenshots via `/screenshot` endpoint
- **WiFi Configuration** - Save network credentials to `/wifi_config.txt` for automatic connection
- **Directory Navigation** - Full filesystem access with breadcrumb navigation

## Troubleshooting

- **Touch accuracy issues**: Run touchscreen calibration from Settings menu
- **Upload fails**: Lower upload speed to `115200` in PlatformIO or Arduino IDE
- **Blank screen**: Check TFT_eSPI pin configuration matches `User_Setup.h`
- **Touch not responding**: Verify XPT2046_Touchscreen library is installed
- **Bluetooth not pairing**: Toggle BLE off/on in Settings, restart device
- **SD card not detected**: Ensure FAT32 formatted card is properly inserted
- **WiFi not connecting**: Create `/wifi_config.txt` on SD card with SSID on line 1, password on line 2
- **Web server not accessible**: Check device IP address on serial monitor or connect to AP mode "CYD-MIDI" (password: midi1234)
- **Screenshots failing**: Ensure SD card has sufficient space and `/screenshots/` directory exists
- **Hardware modifications**: Check [BUILD.md - Pin Map Reference](BUILD.md#pin-map-reference) for complete pin assignments before adding external hardware

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

## Connecting to Your DAW

1. Pair "CYD MIDI" via Bluetooth
2. Select as MIDI input in your DAW (e.g., Ableton Live, FL Studio, Logic Pro)
3. All MIDI modes send on the configured channel (default: Channel 1)
4. Use Settings menu to change MIDI channel if needed

## Contributing

Contributions welcome! This project has diverged significantly from the original, focusing on improved UX for capacitive touchscreens.

### For Developers

Want to add a new mode? See **[DEV_NOTES.md](DEV_NOTES.md)** for a complete step-by-step guide on creating new interactive modes.

## License

Open source - see MIT license file for details.
