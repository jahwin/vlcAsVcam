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

## Capturing what VLC renders (Video Tap)

This project also contains a **video filter submodule** named `vcam-tap` that receives the
same `picture_t*` frames VLC is rendering (post-decoding, and after other filters in the chain).

### Enable the tap

Run VLC with:

```bash
export VLC_PLUGIN_PATH=~/Library/Application\ Support/org.videolan.vlc/plugins
export VLC_VCAM_SOCKET=/tmp/vlc_vcam.sock
/Applications/VLC.app/Contents/MacOS/VLC --video-filter=vcam-tap
```

The filter sends each frame as a UNIX datagram to `VLC_VCAM_SOCKET` (default `/tmp/vlc_vcam.sock`).

### Receive frames (debug)

In another terminal:

```bash
python3 tools/vcam_receiver.py
```

This prints frame metadata and payload size. Next step is to connect this receiver to your
third-party virtual camera system (tell me which one you want to target: OBS Virtual Camera,
CoreMediaIO DAL, Syphon/NDI, etc.).

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
