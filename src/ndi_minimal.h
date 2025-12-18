#ifndef NDI_MINIMAL_H
#define NDI_MINIMAL_H

#include <cstdint>
#include <cstdio>

// Minimal NDI Definitions for Sending Video

#ifdef __cplusplus
extern "C" {
#endif

// Structures
typedef struct NDIlib_t {
  // We will load symbols dynamically or link them.
  // simpler to just decl functions if linking.
} NDIlib_t;

typedef struct NDIlib_source_t {
  const char *p_ndi_name;
  const char *p_url_address; // Deprecated but often present
} NDIlib_source_t;

typedef struct NDIlib_send_create_t {
  const char *p_ndi_name;
  const char *p_groups;
  bool clock_video;
  bool clock_audio;
} NDIlib_send_create_t;

typedef void *NDIlib_send_instance_t;

// Helper for FourCC
#define NDI_LIB_FOURCC(a, b, c, d)                                             \
  ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) |              \
   ((uint32_t)(d) << 24))

typedef enum NDIlib_FourCC_video_type_e {
  // YCbCr Colorspaces
  NDIlib_FourCC_video_type_UYVY = NDI_LIB_FOURCC('U', 'Y', 'V', 'Y'),
  NDIlib_FourCC_type_UYVY = NDI_LIB_FOURCC('U', 'Y', 'V', 'Y'),
  NDIlib_FourCC_video_type_UYVA = NDI_LIB_FOURCC('U', 'Y', 'V', 'A'),
  NDIlib_FourCC_video_type_P216 = NDI_LIB_FOURCC('P', '2', '1', '6'),
  NDIlib_FourCC_video_type_PA16 = NDI_LIB_FOURCC('P', 'A', '1', '6'),
  NDIlib_FourCC_video_type_YV12 = NDI_LIB_FOURCC('Y', 'V', '1', '2'),
  NDIlib_FourCC_video_type_I420 = NDI_LIB_FOURCC('I', '4', '2', '0'),
  NDIlib_FourCC_video_type_NV12 = NDI_LIB_FOURCC('N', 'V', '1', '2'),

  // RGB Colorspaces
  NDIlib_FourCC_video_type_BGRA = NDI_LIB_FOURCC('B', 'G', 'R', 'A'),
  NDIlib_FourCC_video_type_BGRX = NDI_LIB_FOURCC('B', 'G', 'R', 'X'),
  NDIlib_FourCC_video_type_RGBA = NDI_LIB_FOURCC('R', 'G', 'B', 'A'),
  NDIlib_FourCC_video_type_RGBX = NDI_LIB_FOURCC('R', 'G', 'B', 'X')
} NDIlib_FourCC_video_type_e;

typedef enum NDIlib_frame_format_type_e {
  NDIlib_frame_format_type_progressive = 1,
  NDIlib_frame_format_type_interleaved = 0,
  NDIlib_frame_format_type_field_0 = 2,
  NDIlib_frame_format_type_field_1 = 3
} NDIlib_frame_format_type_e;

typedef struct NDIlib_video_frame_v2_t {
  int xres;
  int yres;
  NDIlib_FourCC_video_type_e FourCC;
  int frame_rate_N;
  int frame_rate_D;
  float picture_aspect_ratio;
  NDIlib_frame_format_type_e frame_format_type;
  int64_t timecode;
  const uint8_t *p_data;
  int line_stride_in_bytes;
  const char *p_metadata; // XML metadata
  int64_t timestamp;
} NDIlib_video_frame_v2_t;

// Function Prototypes (to be dlopened or linked)
// Because we might dlopen, we define typedefs for function pointers
typedef bool (*NDIlib_initialize_func)(void);
typedef void (*NDIlib_destroy_func)(void);
typedef NDIlib_send_instance_t (*NDIlib_send_create_func)(
    const NDIlib_send_create_t *p_create_settings);
typedef void (*NDIlib_send_destroy_func)(NDIlib_send_instance_t p_instance);
typedef void (*NDIlib_send_send_video_v2_func)(
    NDIlib_send_instance_t p_instance,
    const NDIlib_video_frame_v2_t *p_video_data);

#ifdef __cplusplus
}
#endif

#endif // NDI_MINIMAL_H
