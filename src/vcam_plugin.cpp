#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_interface.h>
#include <cstdio>

/*
 * This plugin implements a basic VLC interface module.
 * It is a C++ project that builds into a .dylib.
 */

static int Open(vlc_object_t *);
static void Close(vlc_object_t *);

/*
 * VLC macOS (3.x) expects a versioned entry point symbol:
 *   vlc_entry__3_0_0f
 * and (during cache scan) a versioned API version symbol:
 *   vlc_entry_api_version__3_0_0f
 *
 * We build the module with MODULE_NAME=3_0_0f so the macros generate those.
 * To keep compatibility with loaders that look for unversioned symbols,
 * we provide thin unversioned wrappers below.
 */
extern "C" {
int vlc_entry__3_0_0f(vlc_set_cb, void *);
const char *vlc_entry_api_version__3_0_0f(void);

/* VLC on macOS may expect this symbol to exist in dynamic modules. */
const char vlc_module_name[] = "video_vcam";

int vlc_entry(vlc_set_cb cb, void *opaque) { return vlc_entry__3_0_0f(cb, opaque); }
const char *vlc_entry_api_version(void) { return vlc_entry_api_version__3_0_0f(); }
}

// Define the module
vlc_module_begin()
    set_shortname("VCam")
        set_description("VCam Plugin for VLC - Adds Start VCam capability")
            set_capability("interface", 0)
    // set_category(CAT_INTERFACE) - Not needed/available in 3.0?
    set_subcategory(SUBCAT_INTERFACE_GENERAL)
        set_callbacks(Open, Close)
            add_shortcut("vcam")
                vlc_module_end()

// Open: Called when the module is loaded
static int Open(vlc_object_t *p_this)
{
    (void)p_this;
    // Minimal plugin: prove load/activation without relying on VLC internal symbols
    // that differ between VLC builds on macOS.
    std::fprintf(stderr, "[VCam] Open called (plugin loaded)\n");

    return VLC_SUCCESS;
}

// Close: Called when the module is unloaded
static void Close(vlc_object_t *p_this)
{
    (void)p_this;
    std::fprintf(stderr, "[VCam] Close called (plugin unloaded)\n");
}
