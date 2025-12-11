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

### Prerequisites: USB Drivers
Before uploading, ensure you have installed the appropriate USB-to-serial drivers:
- **CP210x (Silicon Labs)**: [Download here](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)
- **CH340/CH341 (WCH)**: [Download here](http://www.wch-ic.com/downloads/CH341SER_EXE.html)

Most CYD boards use the CP210x chip. Install the driver for your operating system and restart if needed.

### Detecting Your Serial Port

1. Connect your ESP32 board via USB
2. Find the serial port:
   ```bash
   # macOS
   ls /dev/cu.*
   # Look for: /dev/cu.usbserial-* or /dev/cu.SLAB_USBtoUART
   
   # Linux
   ls /dev/ttyUSB* /dev/ttyACM*
   # Look for: /dev/ttyUSB0 or /dev/ttyACM0
   
   # Windows (PowerShell)
   [System.IO.Ports.SerialPort]::getportnames()
   # Look for: COM3, COM4, etc.
   ```

### Using VS Code
1. Click the right arrow (→) icon in the bottom toolbar to upload
2. Or use **PlatformIO: Upload** from the command palette (Cmd+Shift+P / Ctrl+Shift+P)

### Using Command Line
```bash
# Upload (PlatformIO auto-detects port)
pio run -t upload -e cyd35

# Or specify the port explicitly
pio run -t upload -e cyd35 --upload-port /dev/cu.SLAB_USBtoUART

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

## Troubleshooting

### Build Errors
- **Library not found**: Run `pio lib install` to fetch dependencies automatically
- **TFT display errors**: Verify pin definitions in `platformio.ini` match your board
- **BLE errors**: Ensure you're using ESP32 (not ESP8266)
- **Partition table errors**: The project uses `huge_app.csv` for 3.1MB app space - this is correct

### Upload Errors

**Common Issues:**
- **Serial port not found**: 
  - Check USB cable (must be data cable, not charge-only)
  - Install CP210x or CH340 drivers (see [Prerequisites](#prerequisites-usb-drivers))
  - Try different USB port (USB 2.0 often more reliable)
  - Manually specify port: `pio run -t upload --upload-port /dev/cu.XXX`

- **"Failed to connect to ESP32" or "Timed out waiting for packet header"**:
  - Hold **BOOT** button during upload (release when "Writing at..." appears)
  - Reduce upload speed in `platformio.ini`: `upload_speed = 115200`
  - Try different reset method: `upload_resetmethod = nodemcu`
  - Close other programs using serial port (Arduino IDE, screen, minicom)

- **Permission denied**: 
  - **macOS**: Grant Terminal/VS Code permissions in System Preferences → Security & Privacy → Privacy → Full Disk Access
  - **Linux**: Add user to `dialout` group:
    ```bash
    sudo usermod -a -G dialout $USER
    # Log out and back in for changes to take effect
    ```

**ESP32-S3 Boards:**
- May have two USB ports - try both
- Requires manual boot mode:
  1. Hold **BOOT** button
  2. Press **RST** briefly
  3. Release **BOOT** after 1-2 seconds
  4. Upload immediately
- Add to `platformio.ini`:
  ```ini
  board = esp32-s3-devkitc-1
  upload_speed = 115200
  ```

### Serial Monitor
```bash
# Open serial monitor
pio device monitor --baud 115200

# Monitor with timestamp
pio device monitor --baud 115200 --filter time

# Exit: Ctrl+C
```

### Clean Build
If encountering persistent errors, try a clean build:
```bash
pio run --target clean
pio run -e cyd35
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
