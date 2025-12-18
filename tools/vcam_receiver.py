#!/usr/bin/env python3
"""
Simple receiver for the VLC VCamTap filter.

It listens on a UNIX stream socket (default: /tmp/vlc_vcam.sock) and prints frame info.
This is meant as a debugging bridge before wiring into a real virtual camera stack.
"""

import os
import socket
import struct

SOCK_PATH = os.environ.get("VLC_VCAM_SOCKET", "/tmp/vlc_vcam.sock")

# Must match vcam_frame_header in src/vcam_plugin.cpp
# uint32 magic, version, width, height, chroma, planes
# uint64 pts
# uint32 pitches[4], lines[4]
HDR_FMT = "<6I Q 4I 4I"
HDR_SIZE = struct.calcsize(HDR_FMT)
MAGIC = 0x5643414D  # 'VCAM'


def main():
    try:
        os.unlink(SOCK_PATH)
    except FileNotFoundError:
        pass

    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.bind(SOCK_PATH)
    s.listen(1)
    print(f"[receiver] listening (stream) on {SOCK_PATH}")

    frames = 0
    while True:
        conn, _ = s.accept()
        print("[receiver] connected")
        try:
            while True:
                hdr = b""
                while len(hdr) < HDR_SIZE:
                    chunk = conn.recv(HDR_SIZE - len(hdr))
                    if not chunk:
                        raise EOFError()
                    hdr += chunk

                (
                    magic,
                    version,
                    width,
                    height,
                    chroma,
                    planes,
                    pts,
                    p0, p1, p2, p3,
                    l0, l1, l2, l3,
                ) = struct.unpack_from(HDR_FMT, hdr, 0)

                if magic != MAGIC:
                    # If stream gets desynced, drop this connection and wait for a new one.
                    raise ValueError("bad magic")

                pitches = [p0, p1, p2, p3]
                lines = [l0, l1, l2, l3]
                total = 0
                for i in range(min(planes, 4)):
                    total += pitches[i] * lines[i]

                # Drain payload
                remaining = total
                while remaining > 0:
                    chunk = conn.recv(min(remaining, 1024 * 1024))
                    if not chunk:
                        raise EOFError()
                    remaining -= len(chunk)

                frames += 1
                print(
                    f"[frame {frames}] v{version} {width}x{height} chroma=0x{chroma:08x} "
                    f"planes={planes} pts={pts} pitches={pitches} lines={lines} "
                    f"payload_bytes={total}"
                )
        except EOFError:
            print("[receiver] disconnected")
        except Exception as e:
            print(f"[receiver] connection error: {e}")
        finally:
            try:
                conn.close()
            except Exception:
                pass


if __name__ == "__main__":
    main()



