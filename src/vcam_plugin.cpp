#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_interface.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <new>

#include <vlc_filter.h>
#include <vlc_picture.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <inttypes.h>

// Some tooling/language-servers may not see the build-system -DMODULE_STRING,
// but VLC's plugin macros require it. The actual build still defines it.
#ifndef MODULE_STRING
#define MODULE_STRING "video_vcam"
#endif

/*
 * This plugin implements a basic VLC interface module.
 * It is a C++ project that builds into a .dylib.
 */

// Interface module callbacks
static int IntfOpen(vlc_object_t *);
static void IntfClose(vlc_object_t *);

// Video filter submodule callbacks
static int VideoTapOpen(vlc_object_t *);
static picture_t *VideoTapFilter(filter_t *, picture_t *);
static void VideoTapClose(vlc_object_t *);

// Define the module
vlc_module_begin()
    set_shortname("VCam")
        set_description("VCam Plugin for VLC - Adds Start VCam capability")
            set_capability("interface", 0)
    // set_category(CAT_INTERFACE) - Not needed/available in 3.0?
    set_subcategory(SUBCAT_INTERFACE_GENERAL)
        set_callbacks(IntfOpen, IntfClose)
            add_shortcut("vcam")

                add_submodule()
                    set_shortname("VCamTap")
                        set_description("VCam video tap (exports rendered frames)")
                            set_capability("video filter", 0)
                                set_callbacks(VideoTapOpen, VideoTapClose)
                                    add_shortcut("vcam-tap")
                                        vlc_module_end()

    // -----------------------------
    // Interface module
    // -----------------------------
    static int IntfOpen(vlc_object_t *p_this)
{
    (void)p_this;
    // Minimal plugin: prove load/activation without relying on VLC internal symbols
    // that differ between VLC builds on macOS.
    std::fprintf(stderr, "[VCam] Open called (plugin loaded)\n");

    return VLC_SUCCESS;
}

static void IntfClose(vlc_object_t *p_this)
{
    (void)p_this;
    std::fprintf(stderr, "[VCam] Close called (plugin unloaded)\n");
}

// -----------------------------
// Video tap filter submodule
// -----------------------------

struct filter_sys_t
{
    int fd = -1;
    uint64_t frames_seen = 0;
    uint64_t frames_sent = 0;
};

static const uint32_t VCAM_MAGIC = 0x5643414d; // 'VCAM'
static const uint32_t VCAM_VERSION = 1;

struct vcam_frame_header
{
    uint32_t magic;
    uint32_t version;
    uint32_t width;
    uint32_t height;
    uint32_t chroma; // vlc_fourcc_t
    uint32_t planes;
    uint64_t pts; // picture timestamp (if available)
    uint32_t pitches[4];
    uint32_t lines[4];
};

static bool write_all_blocking(int fd, const void *buf, size_t len)
{
    const uint8_t *p = static_cast<const uint8_t *>(buf);
    size_t off = 0;
    while (off < len)
    {
        ssize_t n = ::write(fd, p + off, len - off);
        if (n > 0)
        {
            off += static_cast<size_t>(n);
            continue;
        }
        if (n == 0)
            return false;
        if (errno == EINTR)
            continue;
        return false;
    }
    return true;
}

static int VideoTapOpen(vlc_object_t *obj)
{
    filter_t *filter = (filter_t *)obj;
    // Pass-through: output format equals input format.
    filter->fmt_out = filter->fmt_in;

    auto *sys = new (std::nothrow) filter_sys_t();
    if (!sys)
        return VLC_ENOMEM;

    const char *sock_path = ::getenv("VLC_VCAM_SOCKET");
    if (!sock_path || !*sock_path)
    {
        sock_path = "/tmp/vlc_vcam.sock";
    }

    sys->fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (sys->fd < 0)
    {
        delete sys;
        return VLC_EGENERIC;
    }

    sockaddr_un addr{};
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", sock_path);

    if (::connect(sys->fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0)
    {
        // No receiver running yet. Keep the filter alive but disable sending.
        ::close(sys->fd);
        sys->fd = -1;
        std::fprintf(stderr, "[VCamTap] enabled but NOT connected (start receiver first). socket=%s\n", sock_path);
    }
    else
    {
        // Blocking stream: simplest and reliable for now.
        // (We can add a ring-buffer/shared-memory later for performance.)
        std::fprintf(stderr, "[VCamTap] enabled, streaming frames to %s\n", sock_path);
    }

    filter->p_sys = sys;

    // VLC 3.0.x filter API: set the callback directly
    filter->pf_video_filter = VideoTapFilter;

    return VLC_SUCCESS;
}

static picture_t *VideoTapFilter(filter_t *filter, picture_t *pic)
{
    auto *sys = filter->p_sys;
    if (!sys || sys->fd < 0 || !pic)
        return pic;

    sys->frames_seen++;
    if (sys->frames_seen <= 5)
        std::fprintf(stderr, "[VCamTap] filter called (frame=%" PRIu64 ")\n", sys->frames_seen);

    vcam_frame_header hdr{};
    hdr.magic = VCAM_MAGIC;
    hdr.version = VCAM_VERSION;
    hdr.width = static_cast<uint32_t>(filter->fmt_in.video.i_visible_width);
    hdr.height = static_cast<uint32_t>(filter->fmt_in.video.i_visible_height);
    hdr.chroma = static_cast<uint32_t>(filter->fmt_in.video.i_chroma);
    hdr.planes = static_cast<uint32_t>(pic->i_planes);
    hdr.pts = static_cast<uint64_t>(pic->date);

    const int max_planes = (pic->i_planes > 4) ? 4 : pic->i_planes;
    for (int i = 0; i < max_planes; i++)
    {
        hdr.pitches[i] = static_cast<uint32_t>(pic->p[i].i_pitch);
        hdr.lines[i] = static_cast<uint32_t>(pic->p[i].i_lines);
    }

    // Stream: header then plane bytes.
    if (!write_all_blocking(sys->fd, &hdr, sizeof(hdr)))
    {
        std::fprintf(stderr, "[VCamTap] write failed (header) errno=%d\n", errno);
        if (errno == EPIPE)
        {
            ::close(sys->fd);
            sys->fd = -1;
        }
        return pic;
    }

    for (int i = 0; i < max_planes; i++)
    {
        const size_t len = static_cast<size_t>(pic->p[i].i_pitch) * static_cast<size_t>(pic->p[i].i_lines);
        if (!write_all_blocking(sys->fd, pic->p[i].p_pixels, len))
        {
            std::fprintf(stderr, "[VCamTap] write failed (plane=%d) errno=%d\n", i, errno);
            if (errno == EPIPE)
            {
                ::close(sys->fd);
                sys->fd = -1;
            }
            return pic;
        }
    }

    sys->frames_sent++;
    if (sys->frames_sent <= 5)
        std::fprintf(stderr, "[VCamTap] sent frame=%" PRIu64 "\n", sys->frames_sent);
    return pic;
}

static void VideoTapClose(vlc_object_t *obj)
{
    filter_t *filter = (filter_t *)obj;
    auto *sys = filter->p_sys;
    if (sys)
    {
        if (sys->fd >= 0)
            ::close(sys->fd);
        delete sys;
        filter->p_sys = NULL;
    }
    std::fprintf(stderr, "[VCamTap] disabled\n");
}
