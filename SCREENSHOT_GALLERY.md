# Screenshot Gallery & Download Feature

The CYD MIDI Controller now includes a web-based screenshot gallery and automated download tools for managing screenshots captured on the device.

## Features

### Web Interface Gallery

The web interface now includes a new **Screenshots Gallery** section that displays:
- All BMP screenshots stored on the device's SD card
- Grid-based gallery view with thumbnail placeholders
- Individual download buttons for each screenshot
- Bulk download feature ("Download All" button)
- Delete functionality for removing unwanted screenshots
- Auto-refresh after taking new screenshots

### Automated Download Tools

Two scripts are provided for downloading all screenshots:

#### Python Script (Recommended)

```bash
# Install dependencies (one time)
pip install requests

# Download all screenshots (default to 192.168.4.1)
python3 download_screenshots.py

# Download from specific IP
python3 download_screenshots.py --device http://192.168.1.100

# Download to custom directory
python3 download_screenshots.py --output my_screenshots

# Verbose output
python3 download_screenshots.py -v
```

**Features:**
- Cross-platform (Linux, macOS, Windows with Python)
- Automatic directory creation
- Detailed error handling
- Progress display
- Bandwidth-aware (delays between downloads)

#### Bash Script (Quick & Simple)

```bash
# Download all screenshots (default to 192.168.4.1)
./download_screenshots.sh

# Download from specific IP
./download_screenshots.sh http://192.168.1.100

# Download to custom directory
./download_screenshots.sh http://192.168.1.100 my_screenshots
```

**Requirements:**
- `curl` (usually pre-installed)
- `jq` (JSON processor)

### Web API Endpoints

New endpoints added to the web server:

#### List All Screenshots
```
GET /screenshots
Returns: JSON array of screenshot objects
[
  {"name": "00_main_menu.bmp", "path": "00_main_menu.bmp", "size": 307200},
  {"name": "03_keyboard.bmp", "path": "03_keyboard.bmp", "size": 307200},
  ...
]
```

#### Download Screenshot
```
GET /screenshot?file={filename}
Returns: BMP binary data
```

#### Delete Screenshot
```
DELETE /screenshot?file={filename}
Returns: Success/error message
```

#### Capture New Screenshot
```
GET /screenshot (no parameters)
Captures the current screen and returns BMP data
```

## Usage Examples

### Via Web Interface

1. Open device IP in browser (http://192.168.4.1 or configured IP)
2. Scroll to "Screenshots Gallery" section
3. Click on individual screenshot to download
4. Click "Download All" to bulk download all screenshots
5. Click delete icon to remove unwanted screenshots

### Via Command Line

**Python (Recommended for automated workflows):**
```bash
# Setup
pip install requests

# Download all screenshots daily
python3 download_screenshots.py --device http://192.168.1.100 --output ~/Downloads/cyd_screenshots_$(date +%Y%m%d)
```

**Bash (Quick one-liner):**
```bash
# Download and backup
./download_screenshots.sh http://192.168.1.100 ./backup_$(date +%s)
```

## Connection Methods

### Device Access Point Mode (Default)
- URL: `http://192.168.4.1`
- SSID: "CYD-MIDI"
- Password: "midi1234"

### Station Mode (Connected to WiFi)
- URL: `http://{device-ip}` (shown on serial monitor)
- Example: `http://192.168.1.100`

## Screenshot Storage

Screenshots are stored as 480×320 BMP files on the device's SD card:
- Format: 16-bit RGB565 uncompressed
- File size: ~307 KB per screenshot
- Location: Root directory of SD card
- Naming: Automatically generated with mode/menu names

## Web Interface Features

### Screenshot Gallery Section
```html
📸 Screenshots Gallery
[Refresh Gallery] [⬇️ Download All]

[🖼️]  [🖼️]  [🖼️]
00_m.. 03_k.. 05_b..
[⬇️][🗑️] [⬇️][🗑️] [⬇️][🗑️]
```

### Gallery HTML Structure
- Responsive grid layout (auto-fill columns)
- Mobile-friendly design
- Visual feedback for downloads
- One-click delete with confirmation

## Technical Details

### Backend Implementation
- **Handler**: `handleScreenshots()` lists all BMP files
- **Handler**: `handleScreenshot()` handles GET/DELETE for individual screenshots
- **Storage**: Scans SD card root for `.bmp` files
- **Format**: JSON response with filename, path, and size

### Frontend Implementation
- **Gallery Function**: `loadScreenshots()` fetches and displays gallery
- **Download**: `downloadAllScreenshots()` with timed delays between requests
- **Delete**: `delScreenshot()` sends DELETE request and refreshes gallery
- **UI**: CSS Grid layout with automatic column sizing

## Troubleshooting

### "No screenshots found"
- Check if any screenshots have been captured
- Verify SD card is accessible
- Check device serial output for SD card mount status

### Download fails with Python script
- Verify device IP address is correct
- Ensure `requests` library is installed: `pip install requests`
- Check device is powered on and WiFi is connected
- Try verbose mode: `python3 download_screenshots.py -v`

### Download fails with Bash script
- Verify `curl` and `jq` are installed
- On macOS: `brew install curl jq`
- On Ubuntu: `apt install curl jq`
- Check device connectivity

### Web interface shows blank gallery
- Device may be starting up
- Try clicking "Refresh Gallery" button
- Check serial monitor for errors
- Verify web interface loads other sections properly

## Performance Notes

- BMP files are 307 KB each (uncompressed)
- Download time depends on WiFi speed (typically 1-3 seconds per screenshot)
- Bash script includes 200ms delays between downloads to avoid network issues
- Python script includes configurable delays

## Future Enhancements

Potential improvements for future versions:
- Thumbnail generation (reduce file sizes)
- Screenshot compression (JPEG/PNG export)
- Batch operations (multi-select delete)
- Scheduled backups
- Cloud upload integration
- Screenshot comparison tool

## See Also

- [Web Server Documentation](./README.md) - Web interface features
- [BUILD.md](./BUILD.md) - Build and hardware configuration
- [DEV_NOTES.md](./DEV_NOTES.md) - Development guide

---

Last updated: 2024
Device: CYD MIDI Controller - Enhanced Edition
