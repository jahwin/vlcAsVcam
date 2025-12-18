#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_filter.h>
#include <vlc_picture.h>
#include <vlc_plugin.h>

#include "ndi_minimal.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <new>

#ifndef MODULE_STRING
#define MODULE_STRING "video_vcam"
#endif

// Forward decls
static int IntfOpen(vlc_object_t *);
static void IntfClose(vlc_object_t *);
static int VideoTapOpen(vlc_object_t *);
static picture_t *VideoTapFilter(filter_t *, picture_t *);
static void VideoTapClose(vlc_object_t *);

// Module definition
vlc_module_begin() set_shortname("VCamNDI") set_description("VLC NDI Output")
    set_capability("interface", 0) set_subcategory(SUBCAT_INTERFACE_GENERAL)
        set_callbacks(IntfOpen, IntfClose) add_shortcut("vcam")

            add_submodule() set_shortname("NDITap")
                set_description("NDI Video Output Filter")
                    set_capability("video filter", 0)
                        set_callbacks(VideoTapOpen, VideoTapClose)
                            add_shortcut("vcam-tap") vlc_module_end()

    // Internal System
    struct filter_sys_t {
  NDIlib_send_instance_t p_ndi_send;
  int width;
  int height;
};

// Global NDI Init
static bool ndi_initialized = false;

extern "C" {
bool NDIlib_initialize(void);
NDIlib_send_instance_t
NDIlib_send_create(const NDIlib_send_create_t *p_create_settings);
void NDIlib_send_destroy(NDIlib_send_instance_t p_instance);
void NDIlib_send_send_video_v2(NDIlib_send_instance_t p_instance,
                               const NDIlib_video_frame_v2_t *p_video_data);
}

static int IntfOpen(vlc_object_t *p_this) {
  fprintf(stderr, "[VCam] Interface Loaded\n");
  return VLC_SUCCESS;
}

static void IntfClose(vlc_object_t *p_this) { (void)p_this; }

static int VideoTapOpen(vlc_object_t *obj) {
  filter_t *filter = (filter_t *)obj;
  filter->fmt_out = filter->fmt_in;

  fprintf(stderr, "[VCamTap] VideoTapOpen called. Input chroma: %4.4s\n",
          (const char *)&filter->fmt_in.video.i_chroma);

  if (!ndi_initialized) {
    fprintf(stderr, "[VCamTap] Initializing NDIlib...\n");
    if (!NDIlib_initialize()) {
      fprintf(stderr, "[VCamTap] NDIlib_initialize returned false!\n");
      return VLC_EGENERIC;
    }
    ndi_initialized = true;
    fprintf(stderr, "[VCamTap] NDIlib initialized successfully.\n");
  }

  auto *sys = new (std::nothrow) filter_sys_t();
  if (!sys)
    return VLC_ENOMEM;

  NDIlib_send_create_t create_settings;
  create_settings.p_ndi_name = "VLC Output";
  create_settings.p_groups = NULL;
  create_settings.clock_video = true;
  create_settings.clock_audio = false;

  fprintf(stderr, "[VCamTap] Creating NDI sender...\n");
  sys->p_ndi_send = NDIlib_send_create(&create_settings);
  if (!sys->p_ndi_send) {
    fprintf(stderr, "[VCamTap] NDIlib_send_create failed!\n");
    delete sys;
    return VLC_EGENERIC;
  }

  sys->width = filter->fmt_in.video.i_visible_width;
  sys->height = filter->fmt_in.video.i_visible_height;

  filter->p_sys = sys;
  filter->pf_video_filter = VideoTapFilter;

  fprintf(stderr, "[VCamTap] NDI Sender started: 'VLC Output' %dx%d\n",
          sys->width, sys->height);

  return VLC_SUCCESS;
}

static picture_t *VideoTapFilter(filter_t *filter, picture_t *pic) {
  auto *sys = filter->p_sys;
  if (!sys || !sys->p_ndi_send || !pic)
    return pic;

  // Validate Chroma
  if (filter->fmt_in.video.i_chroma != VLC_CODEC_I420 &&
      filter->fmt_in.video.i_chroma != VLC_CODEC_YV12) {
    static bool warned = false;
    if (!warned) {
      fprintf(stderr,
              "[VCamTap] Unsupported chroma for NDI: %4.4s. Expected I420.\n",
              (const char *)&filter->fmt_in.video.i_chroma);
      warned = true;
    }
    return pic;
  }

  // Setup NDI Frame
  NDIlib_video_frame_v2_t NDI_video_frame;
  memset(&NDI_video_frame, 0, sizeof(NDIlib_video_frame_v2_t));

  NDI_video_frame.xres = filter->fmt_in.video.i_visible_width;
  NDI_video_frame.yres = filter->fmt_in.video.i_visible_height;
  NDI_video_frame.frame_format_type = NDIlib_frame_format_type_progressive;
  NDI_video_frame.picture_aspect_ratio =
      (float)NDI_video_frame.xres / (float)NDI_video_frame.yres;
  NDI_video_frame.frame_rate_N = filter->fmt_in.video.i_frame_rate;
  NDI_video_frame.frame_rate_D = filter->fmt_in.video.i_frame_rate_base;
  NDI_video_frame.timecode = pic->date * 10;

  // Using I420 (Corrected FourCC in header now)
  NDI_video_frame.FourCC = NDIlib_FourCC_video_type_I420;

  // Copy Buffer (Planar I420)
  static uint8_t *temp_buffer = NULL;
  static size_t temp_size = 0;

  // I420 size: W*H + (W/2*H/2)*2 = W*H * 1.5
  size_t required = NDI_video_frame.xres * NDI_video_frame.yres * 3 / 2;

  if (temp_size < required) {
    free(temp_buffer);
    temp_buffer = (uint8_t *)malloc(required);
    temp_size = required;
  }

  uint8_t *dst = temp_buffer;

  // Copy Y Plane
  for (int i = 0; i < NDI_video_frame.yres; i++) {
    memcpy(dst + i * NDI_video_frame.xres,
           pic->p[0].p_pixels + i * pic->p[0].i_pitch, NDI_video_frame.xres);
  }

  int uv_h = NDI_video_frame.yres / 2;
  int uv_w = NDI_video_frame.xres / 2;

  // Copy U Plane
  uint8_t *u_dst = dst + (NDI_video_frame.xres * NDI_video_frame.yres);
  for (int i = 0; i < uv_h; i++) {
    memcpy(u_dst + i * uv_w, pic->p[1].p_pixels + i * pic->p[1].i_pitch, uv_w);
  }

  // Copy V Plane
  uint8_t *v_dst = u_dst + (uv_w * uv_h);
  for (int i = 0; i < uv_h; i++) {
    memcpy(v_dst + i * uv_w, pic->p[2].p_pixels + i * pic->p[2].i_pitch, uv_w);
  }

  NDI_video_frame.p_data = temp_buffer;
  NDI_video_frame.line_stride_in_bytes =
      NDI_video_frame.xres; // Stride is width for Y plane

  NDIlib_send_send_video_v2(sys->p_ndi_send, &NDI_video_frame);

  // Logging
  static int frame_count = 0;
  frame_count++;
  if (frame_count % 60 == 0) {
    fprintf(stderr, "[VCamTap] Sent Frame %d (I420). Res: %dx%d\n", frame_count,
            NDI_video_frame.xres, NDI_video_frame.yres);
  }

  return pic;
}

static void VideoTapClose(vlc_object_t *obj) {
  filter_t *filter = (filter_t *)obj;
  auto *sys = filter->p_sys;
  if (sys) {
    if (sys->p_ndi_send) {
      NDIlib_send_destroy(sys->p_ndi_send);
    }
    delete sys;
    filter->p_sys = NULL;
  }
  fprintf(stderr, "[VCamTap] NDI Sender stopped\n");
}
