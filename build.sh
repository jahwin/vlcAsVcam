#!/bin/bash
set -e

# Create build directory
mkdir -p build
cd build

# Function to check for VLC plugin headers
check_headers() {
    local dir="$1"
    if [ -f "$dir/vlc_plugin.h" ] || [ -f "$dir/vlc/vlc_plugin.h" ]; then
        return 0
    fi
    return 1
}

# Attempt to find VLC headers if not set
if [ -z "$VLC_INCLUDE_DIR" ]; then
    echo "Checking for VLC plugin headers..."
    
    # Check common locations
    # Note: VLC.app/Contents/MacOS/include usually only contains LibVLC headers, not plugin headers.
    
    if [ -d "/usr/local/include/vlc/plugins" ]; then
         export VLC_INCLUDE_DIR="/usr/local/include"
         echo "Found in /usr/local/include"
    elif [ -d "/opt/homebrew/include/vlc/plugins" ]; then
         export VLC_INCLUDE_DIR="/opt/homebrew/include"
         echo "Found in /opt/homebrew/include"
    else
        echo "VLC plugin headers not found automatically."
        echo "Note: The headers inside VLC.app are usually LibVLC headers (for embedding), not Plugin headers."
        echo "To build a plugin, you generally need the VLC source code or the 'vlc-plugin' dev package."
        echo ""
        echo "Please set VLC_INCLUDE_DIR to the directory containing vlc_plugin.h"
        echo "Example: export VLC_INCLUDE_DIR=/path/to/vlc-source/include"
    fi
fi

echo "Running CMake..."
if [ -n "$VLC_INCLUDE_DIR" ]; then
    cmake .. -DVLC_INCLUDE_DIR="$VLC_INCLUDE_DIR"
else
    cmake ..
fi

echo "Building..."
make

echo "Done. Plugin is at build/libvideo_vcam_plugin.dylib"
