# VLC NDI Output Plugin for macOS

## 📖 What is this?
This is a **plugin for VLC Media Player** on macOS.
It allows VLC to send its video stream directly to **NDI** (Network Device Interface).

**Why use this?**
- To send video from VLC to **OBS**, **vMix**, or **Resolume** on the same network.
- To use VLC as a source for other NDI-enabled tools.

---

## ⚠️ Important Compatibility Notes
Before you start, make sure you meet the requirements:
*   **Computer**: macOS with **Apple Silicon** (M1/M2/M3 chips).
    *   *Note: It will not work on Intel Macs without recompiling.*
*   **VLC Version**: **VLC 3.0.x** (Standard version).
    *   *Note: It does not work with VLC 4.0.*
*   **Software**: You must have [NDI Tools](https://ndi.video/tools/) installed.

---

## 🛠 How to Build

If you want to compile the plugin yourself (e.g., if you changed the code):

1.  **Open Terminal** in this folder.
2.  Run the build script:
    ```bash
    ./build.sh
    ```
3.  The new plugin will appear in the `build/` folder as `libvideo_vcam_plugin.dylib`.

---

## 💾 How to Install

To install the plugin so VLC uses it automatically:

### 1. Copy the Plugin
Copy the file `build/libvideo_vcam_plugin.dylib` to this folder:
```text
/Applications/VLC.app/Contents/MacOS/plugins/
```
*(You may need Administrator permission/password to paste here).*

### 2. Unblock Security (Quarantine)
macOS will block the plugin by default because it wasn't downloaded from the App Store. You must unblock it using the Terminal:

```bash
xattr -d com.apple.quarantine /Applications/VLC.app/Contents/MacOS/plugins/libvideo_vcam_plugin.dylib
```
*If you see "No such xattr", that's good! It means it's already unblocked.*

---

## ⚙️ How to Configure VLC (Crucial Steps!)

For the plugin to work, you **MUST** change these settings in VLC:

### 1. Disable Hardware Acceleration
NDI needs "raw" video data, but hardware acceleration hides it.
1.  Open **VLC Preferences** (Cmd + ,).
2.  Go to **Input / Codecs**.
3.  Find **Hardware-accelerated decoding**.
4.  Set it to **Disable**.
5.  Click **Save**.

### 2. Enable the NDI Filter
Tell VLC to use our plugin.
1.  Open **VLC Preferences**.
2.  Click **Show All** (Button in the bottom-left corner).
3.  In the left list, scroll down to **Video** -> **Filters**.
4.  On the right side, verify the checkbox **NDI Video Output Filter** (or "VCamNDI") is **CHECKED**.
5.  Click **Save**.

### 3. Restart VLC
Quit VLC completely (Cmd + Q) and open it again. Play a video. You should now see it as a source in your NDI Monitor! 

---

## 🐛 Troubleshooting

*   **"I see the NDI source, but the screen is black!"**
    *   Did you disable Hardware Acceleration? (See Step 1 above).
    *   Are you on an M1/M2 Mac? (This plugin is built for Apple Silicon).

*   **"VLC crashes when I enable the filter."**
    *   Make sure you have NDI Tools installed. The plugin needs the NDI library to run.
