# Build & Upload Instructions

This project has been migrated to PlatformIO for easier compilation and dependency management.

## Prerequisites

### Option 1: VS Code (Recommended)
1. Install [Visual Studio Code](https://code.visualstudio.com/)
2. Install the **PlatformIO IDE** extension from the Extensions marketplace

### Option 2: Command Line
```bash
# Install PlatformIO Core (macOS)
brew install platformio
```

## Project Structure

```
CYD-MIDI-Controller/
├── platformio.ini           # PlatformIO configuration
├── src/                     # Source code
│   ├── CYD-MIDI-Controller.ino  # Main sketch
│   ├── User_Setup.h         # TFT_eSPI display configuration
│   ├── *.h                  # Mode headers (keyboard, sequencer, etc.)
│   └── ...
└── lib/                     # Optional library overrides
    └── TFT_eSPI/
        └── User_Setup.h     # TFT configuration backup
```

## Building the Project

### Using VS Code
1. Open the project folder in VS Code
2. PlatformIO will automatically detect the `platformio.ini`
3. Click the checkmark (✓) icon in the bottom toolbar to build
4. Or use **PlatformIO: Build** from the command palette (Cmd+Shift+P)

### Using Command Line
```bash
cd /path/to/CYD-MIDI-Controller

# Build the project
pio run

# Or use the full path if pio is not in PATH
~/.platformio/penv/bin/platformio run
```

## Uploading to Board

1. Connect your ESP32 board via USB
2. Find the serial port:
   ```bash
   # macOS
   ls /dev/cu.*
   # Look for something like /dev/cu.usbserial-* or /dev/cu.SLAB_USBtoUART
   ```

### Using VS Code
1. Click the right arrow (→) icon in the bottom toolbar to upload
2. Or use **PlatformIO: Upload** from the command palette

### Using Command Line
```bash
# Upload (PlatformIO auto-detects port)
pio run -t upload

# Or specify the port explicitly
pio run -t upload --upload-port /dev/cu.SLAB_USBtoUART

# Monitor serial output after upload
pio device monitor --baud 115200
```

## Configuration

### Board Selection
The project is configured for `esp32dev` (generic ESP32). If you have a specific board:
1. Edit `platformio.ini`
2. Change the `board` line to match your board:
   ```ini
   board = esp32doit-devkit-v1  # For DevKit V1
   board = ttgo-t-display       # For TTGO T-Display
   # etc.
   ```
3. Find available boards: `pio boards esp32`

### Display Configuration
Display pins are configured in `platformio.ini` under `build_flags`:
- `TFT_MISO`, `TFT_MOSI`, `TFT_SCLK` - SPI pins
- `TFT_CS`, `TFT_DC`, `TFT_RST` - Control pins
- `TFT_BL` - Backlight pin

If your display uses different pins, update these values in `platformio.ini`.

### Touch Controller
Touch controller pins are defined in the main sketch:
```cpp
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33
```

## Pin Map Reference

This section provides complete pin assignments for all CYD board variants. This information is important when planning hardware expansions like serial MIDI interfaces (e.g., M5 MIDI breakout).

### ESP32 Pin Usage Overview

All CYD boards use the ESP32-WROOM-32 module with the following peripheral assignments:

| Peripheral | SPI Bus | Pins Used | Notes |
|------------|---------|-----------|-------|
| TFT Display | VSPI | 12, 13, 14, 15, 2, 21/27 | Main display controller |
| Touch Controller (XPT2046) | VSPI (custom) | 25, 32, 33, 36, 39 | Resistive touch digitizer |
| SD Card Reader | HSPI | 5, 18, 19, 23 | MicroSD card slot |
| Bluetooth/BLE | Internal | N/A | Built-in ESP32 radio |
| WiFi | Internal | N/A | Built-in ESP32 radio |
| Serial/USB | UART0 | 1 (TX), 3 (RX) | Programming & debug console |

### Detailed Pin Assignments by Board

#### CYD 3.5" (ESP32-3248S035) - 480×320 ILI9488

**TFT Display (VSPI)**
| Pin # | Function | Description |
|-------|----------|-------------|
| GPIO12 | TFT_MISO | SPI Master In Slave Out |
| GPIO13 | TFT_MOSI | SPI Master Out Slave In |
| GPIO14 | TFT_SCLK | SPI Clock |
| GPIO15 | TFT_CS | Chip Select |
| GPIO2 | TFT_DC | Data/Command control |
| GPIO27 | TFT_BL | Backlight control (PWM capable) |
| N/A | TFT_RST | Reset (tied to ESP32 reset) |

**Touch Controller (XPT2046) - Custom SPI**
| Pin # | Function | Description |
|-------|----------|-------------|
| GPIO32 | XPT2046_MOSI | Touch SPI Data Out |
| GPIO39 | XPT2046_MISO | Touch SPI Data In (input only) |
| GPIO25 | XPT2046_CLK | Touch SPI Clock |
| GPIO33 | XPT2046_CS | Touch Chip Select |
| GPIO36 | XPT2046_IRQ | Touch Interrupt (input only) |

**SD Card (HSPI)**
| Pin # | Function | Description |
|-------|----------|-------------|
| GPIO23 | SD_MOSI | SD Card Data Out |
| GPIO19 | SD_MISO | SD Card Data In |
| GPIO18 | SD_SCK | SD Card Clock |
| GPIO5 | SD_CS | SD Card Chip Select |

**System Pins**
| Pin # | Function | Description |
|-------|----------|-------------|
| GPIO1 | TX0 | Serial transmit (USB programming) |
| GPIO3 | RX0 | Serial receive (USB programming) |
| GPIO0 | BOOT | Boot mode select (input only) |
| EN | RESET | Hardware reset |

#### CYD 2.8" (ESP32-2432S028R) & 2.4" (ESP32-2432S024) - 320×240 ILI9341

**TFT Display (VSPI)**
| Pin # | Function | Description |
|-------|----------|-------------|
| GPIO12 | TFT_MISO | SPI Master In Slave Out |
| GPIO13 | TFT_MOSI | SPI Master Out Slave In |
| GPIO14 | TFT_SCLK | SPI Clock |
| GPIO15 | TFT_CS | Chip Select |
| GPIO2 | TFT_DC | Data/Command control |
| GPIO21 | TFT_BL | Backlight control (PWM capable) |
| N/A | TFT_RST | Reset (tied to ESP32 reset) |

**Touch Controller (XPT2046) - Custom SPI**
| Pin # | Function | Description |
|-------|----------|-------------|
| GPIO32 | XPT2046_MOSI | Touch SPI Data Out |
| GPIO39 | XPT2046_MISO | Touch SPI Data In (input only) |
| GPIO25 | XPT2046_CLK | Touch SPI Clock |
| GPIO33 | XPT2046_CS | Touch Chip Select |
| GPIO36 | XPT2046_IRQ | Touch Interrupt (input only) |

**SD Card (HSPI)**
| Pin # | Function | Description |
|-------|----------|-------------|
| GPIO23 | SD_MOSI | SD Card Data Out |
| GPIO19 | SD_MISO | SD Card Data In |
| GPIO18 | SD_SCK | SD Card Clock |
| GPIO5 | SD_CS | SD Card Chip Select |

**System Pins**
| Pin # | Function | Description |
|-------|----------|-------------|
| GPIO1 | TX0 | Serial transmit (USB programming) |
| GPIO3 | RX0 | Serial receive (USB programming) |
| GPIO0 | BOOT | Boot mode select (input only) |
| EN | RESET | Hardware reset |

### Key Differences Between Board Variants

| Feature | CYD 3.5" | CYD 2.8" / 2.4" |
|---------|----------|-----------------|
| Display Driver | ILI9488 | ILI9341 |
| Resolution | 480×320 | 320×240 |
| Backlight Pin | GPIO27 | GPIO21 |
| Touch/SD/SPI | Identical | Identical |
| Form Factor | Larger | Compact |

### Available Pins for Expansion

The following ESP32 GPIO pins are **not used** by the current hardware configuration and are available for expansion projects like serial MIDI (M5 MIDI breakout):

**Available GPIOs:**
- **GPIO4** - General purpose I/O
- **GPIO16** - UART2 RX (recommended for serial MIDI IN)
- **GPIO17** - UART2 TX (recommended for serial MIDI OUT)
- **GPIO22** - I2C SCL or general I/O
- **GPIO26** - General purpose I/O / DAC output
- **GPIO34** - Input only (ADC1_CH6)
- **GPIO35** - Input only (ADC1_CH7)

**Notes on Available Pins:**
- GPIO16/17 are ideal for UART2-based serial MIDI implementation
- GPIO34-39 are input-only pins (cannot drive outputs)
- GPIO22/21 can be used for I2C devices (SDA/SCL)
- Some pins may have pull-up/down resistors on the CYD board - verify with multimeter before use

### Serial MIDI Expansion Planning

For adding a hardware MIDI interface (e.g., M5 MIDI breakout or DIN-5 connector):

**Recommended Pin Assignment:**
| Function | GPIO | Notes |
|----------|------|-------|
| MIDI OUT | GPIO17 (TX2) | Connect via optocoupler to DIN-5 pin 5 |
| MIDI IN | GPIO16 (RX2) | Connect via optocoupler from DIN-5 pin 4 |
| MIDI THRU | GPIO4 | Optional - buffer input to second output |

**Circuit Requirements:**
- MIDI OUT: 5V source + 220Ω resistor + 6N138 optocoupler
- MIDI IN: 6N138 optocoupler + 220Ω + 4.7kΩ resistors
- Standard MIDI baud rate: 31,250 bps

**Software Implementation:**
```cpp
// Example UART2 setup for serial MIDI
HardwareSerial MIDISerial(2); // UART2
void setup() {
  MIDISerial.begin(31250, SERIAL_8N1, 16, 17); // RX=16, TX=17
}
```

### Pin Constraints & Limitations

**Input-Only Pins (cannot be used as outputs):**
- GPIO34, GPIO35, GPIO36, GPIO39

**Strapping Pins (avoid if possible, or use with caution):**
- GPIO0 - Must be HIGH during boot (pulled HIGH on CYD boards)
- GPIO2 - Used by TFT_DC, affects boot mode if changed
- GPIO5 - Used by SD_CS, affects boot if pulled HIGH
- GPIO12 - Used by TFT_MISO, affects flash voltage
- GPIO15 - Used by TFT_CS, must be HIGH during boot

**Reserved Pins (do not use):**
- GPIO1/3 - UART0 TX/RX (USB serial programming)
- GPIO6-11 - Connected to internal SPI flash (do not use!)

## Troubleshooting

### Build Errors
- **Library not found**: Run `pio lib install` to fetch dependencies
- **TFT display errors**: Verify pin definitions in `platformio.ini`
- **BLE errors**: Ensure you're using ESP32 (not ESP8266)

### Upload Errors
- **Serial port not found**: 
  - Check USB connection
  - Install CH340/CP210x drivers if needed
  - Manually specify port: `--upload-port /dev/cu.XXX`
- **Permission denied**: 
  - macOS: Grant permissions in System Settings → Privacy & Security
  - Linux: Add user to `dialout` group

### Serial Monitor
```bash
# Open serial monitor
pio device monitor --baud 115200

# Exit: Ctrl+C
```

## Clean Build
If you encounter strange errors, try a clean build:
```bash
pio run --target clean
pio run
```

## Library Dependencies
Automatically managed by PlatformIO:
- **TFT_eSPI** @ 2.5.43 - Display driver
- **XPT2046_Touchscreen** - Touch controller
- **ESP32 BLE Arduino** @ 2.0.0 - Bluetooth MIDI

## Additional Resources
- [PlatformIO Documentation](https://docs.platformio.org/)
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32)
- [TFT_eSPI Library](https://github.com/Bodmer/TFT_eSPI)
