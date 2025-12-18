# VLC VCam Plugin

This project builds a VLC interface plugin that adds a "Start VCam" capability (currently simulated via logs).

## Prerequisites

To build a VLC plugin, you need the **VLC Plugin Headers**.
**Note:** The headers included in `VLC.app` are for *embedding* VLC (LibVLC), not for *extending* it (Plugins). You cannot build this plugin with just the VLC application installed.

You have two options:
1.  **Install VLC development headers** (via Homebrew or MacPorts if available/supported).
2.  **Use the VLC Source Code** (Recommended for macOS).

### Option 2: Using VLC Source Code

1.  Clone the VLC repository:
    ```bash
    git clone https://code.videolan.org/videolan/vlc.git
    cd vlc
    ```
2.  You may need to run `./configure` to generate `config.h` and other necessary files.
    ```bash
    ./bootstrap
    ./configure --disable-libvlc --disable-nls
    ```
    (You might need dependencies installed via brew).

3.  Export the include directory:
    ```bash
    export VLC_INCLUDE_DIR=/path/to/vlc/include
    ```

## Building

1.  Run the build script:
    ```bash
    ./build.sh
    ```
    Or manually:
    ```bash
    mkdir build && cd build
    cmake .. -DVLC_INCLUDE_DIR=/path/to/vlc/include
    make
    ```

## Installation

1.  Copy the generated `libvideo_vcam_plugin.dylib` to your user plugins folder:
    ```bash
    mkdir -p ~/Library/Application\ Support/org.videolan.vlc/plugins/
    cp build/libvideo_vcam_plugin.dylib ~/Library/Application\ Support/org.videolan.vlc/plugins/
    ```

## VLC Menu (Official VLC API)

VLC does not expose a public C/C++ plugin API to inject items into the native macOS menu bar.
The official way to add a clickable menu entry is a **VLC Lua Extension**, which appears under:
**VLC → Extensions → VCam → Start VCam**.

Install the extension:

```bash
mkdir -p ~/Library/Application\ Support/org.videolan.vlc/lua/extensions/
cp lua/extensions/VCam.lua ~/Library/Application\ Support/org.videolan.vlc/lua/extensions/
```

2.  Cache generation:
    Sometimes you need to reset the VLC plugin cache:
    ```bash
    /Applications/VLC.app/Contents/MacOS/VLC --reset-plugins-cache
    ```

## Usage

1.  Open VLC.
2.  Open the Messages window (Window -> Messages) and set verbosity to 2 (Debug).
3.  Search for "VCam".
4.  You can force the interface to start via command line:
    ```bash
    /Applications/VLC.app/Contents/MacOS/VLC --intf vcam
    ```
    Or check if it appears in the Interface settings.
