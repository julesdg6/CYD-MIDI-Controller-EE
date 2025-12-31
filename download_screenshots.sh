#!/bin/bash

# CYD MIDI Controller - Screenshot Downloader
# Downloads all BMP screenshots from the device via the web interface
# Usage: ./download_screenshots.sh [device_url] [output_dir]
# Examples:
#   ./download_screenshots.sh                          # Uses default http://192.168.4.1
#   ./download_screenshots.sh http://192.168.1.100
#   ./download_screenshots.sh http://192.168.1.100 my_screenshots

DEVICE_URL="${1:-http://192.168.4.1}"
OUTPUT_DIR="${2:-./screenshots}"

# Remove trailing slash from URL
DEVICE_URL="${DEVICE_URL%/}"

# Create output directory
mkdir -p "$OUTPUT_DIR"

echo "🎹 CYD Screenshot Downloader"
echo "Device: $DEVICE_URL"
echo "Output: $OUTPUT_DIR"
echo ""

# Get list of screenshots
RESPONSE=$(curl -s "$DEVICE_URL/screenshots" 2>/dev/null)

if [ $? -ne 0 ]; then
    echo "❌ Error: Could not connect to device at $DEVICE_URL"
    echo "Make sure the device is powered on and connected to WiFi."
    exit 1
fi

# Check if response is valid JSON
if ! echo "$RESPONSE" | jq empty 2>/dev/null; then
    echo "❌ Error: Invalid response from device (expected JSON)"
    exit 1
fi

# Get number of screenshots
COUNT=$(echo "$RESPONSE" | jq '. | length')

if [ "$COUNT" -eq 0 ]; then
    echo "ℹ️  No screenshots found on device."
    exit 0
fi

echo "Found $COUNT screenshot(s)"
echo ""

# Download each screenshot
echo "$RESPONSE" | jq -c '.[]' | while IFS= read -r line; do
    # Parse JSON line
    FILENAME=$(echo "$line" | jq -r '.name')
    FILEPATH=$(echo "$line" | jq -r '.path')
    
    OUTPUT_PATH="$OUTPUT_DIR/$FILENAME"
    DOWNLOAD_URL="$DEVICE_URL/screenshot?file=$FILEPATH"
    
    echo -n "⬇️  Downloading $FILENAME... "
    
    if curl -s "$DOWNLOAD_URL" -o "$OUTPUT_PATH" 2>/dev/null; then
        FILE_SIZE=$(du -h "$OUTPUT_PATH" | cut -f1)
        echo "✓ ($FILE_SIZE)"
    else
        echo "✗ Failed"
        rm -f "$OUTPUT_PATH"
    fi
done

# Get final count
ACTUAL_COUNT=$(ls -1 "$OUTPUT_DIR"/*.bmp 2>/dev/null | wc -l)

echo ""
echo "✅ Download complete: $ACTUAL_COUNT screenshot(s) downloaded"
echo "📁 Location: $(cd "$OUTPUT_DIR" 2>/dev/null && pwd || echo "$OUTPUT_DIR")"

if [ $ACTUAL_COUNT -gt 0 ]; then
    exit 0
else
    exit 1
fi