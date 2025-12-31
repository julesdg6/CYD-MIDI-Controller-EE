#!/usr/bin/env python3
"""
CYD MIDI Controller - Screenshot Downloader
Downloads all BMP screenshots from the device via the web interface.
"""

import requests
import json
import os
import sys
import time
from pathlib import Path
from urllib.parse import urljoin
import argparse

def download_screenshots(device_url, output_dir=None, verbose=False):
    """
    Download all screenshots from the device.
    
    Args:
        device_url (str): Base URL of the device (e.g., http://192.168.1.100 or http://192.168.4.1)
        output_dir (str): Directory to save screenshots (default: ./screenshots)
        verbose (bool): Print verbose output
    """
    
    if output_dir is None:
        output_dir = "./screenshots"
    
    # Create output directory if it doesn't exist
    Path(output_dir).mkdir(parents=True, exist_ok=True)
    
    # Ensure device URL doesn't have trailing slash
    device_url = device_url.rstrip('/')
    
    # Get list of screenshots
    try:
        if verbose:
            print(f"Connecting to device at {device_url}...")
        
        response = requests.get(f"{device_url}/screenshots", timeout=10)
        response.raise_for_status()
        
        screenshots = response.json()
        
        if not screenshots:
            print("No screenshots found on device.")
            return True
        
        if verbose:
            print(f"Found {len(screenshots)} screenshot(s)")
        
        # Download each screenshot
        downloaded = 0
        failed = 0
        
        for i, screenshot in enumerate(screenshots, 1):
            filename = screenshot['name']
            filepath = os.path.join(output_dir, filename)
            
            try:
                if verbose:
                    print(f"[{i}/{len(screenshots)}] Downloading {filename}...", end=" ", flush=True)
                
                # Download the screenshot
                download_url = f"{device_url}/screenshot?file={screenshot['path']}"
                response = requests.get(download_url, timeout=30)
                response.raise_for_status()
                
                # Save to file
                with open(filepath, 'wb') as f:
                    f.write(response.content)
                
                file_size = os.path.getsize(filepath)
                
                if verbose:
                    print(f"✓ ({file_size} bytes)")
                else:
                    print(f"✓ {filename} ({file_size} bytes)")
                
                downloaded += 1
                
                # Small delay between downloads
                if i < len(screenshots):
                    time.sleep(0.2)
                    
            except Exception as e:
                if verbose:
                    print(f"✗ Error: {e}")
                else:
                    print(f"✗ {filename} - Error: {e}")
                failed += 1
        
        print(f"\nDownload complete: {downloaded} successful, {failed} failed")
        print(f"Screenshots saved to: {os.path.abspath(output_dir)}")
        
        return failed == 0
        
    except requests.exceptions.ConnectionError:
        print(f"Error: Could not connect to device at {device_url}")
        print("Make sure the device is powered on and connected to WiFi")
        return False
    except requests.exceptions.Timeout:
        print(f"Error: Connection to device timed out")
        return False
    except json.JSONDecodeError:
        print("Error: Invalid response from device (expected JSON)")
        return False
    except Exception as e:
        print(f"Error: {e}")
        return False

def main():
    parser = argparse.ArgumentParser(
        description="Download screenshots from CYD MIDI Controller",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Download from default AP mode address
  python3 download_screenshots.py
  
  # Download from custom IP address
  python3 download_screenshots.py --device http://192.168.1.100
  
  # Download to specific directory
  python3 download_screenshots.py --output my_screenshots
  
  # Verbose output
  python3 download_screenshots.py -v
        """
    )
    
    parser.add_argument(
        '--device', '-d',
        default='http://192.168.4.1',
        help='Device URL (default: http://192.168.4.1 for AP mode)'
    )
    parser.add_argument(
        '--output', '-o',
        default='./screenshots',
        help='Output directory (default: ./screenshots)'
    )
    parser.add_argument(
        '--verbose', '-v',
        action='store_true',
        help='Verbose output'
    )
    
    args = parser.parse_args()
    
    success = download_screenshots(
        device_url=args.device,
        output_dir=args.output,
        verbose=args.verbose
    )
    
    sys.exit(0 if success else 1)

if __name__ == '__main__':
    main()
