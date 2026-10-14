#ifndef __IMG_H__
#define __IMG_H__

#include "base.h"
#include <jpeglib.h>
#include <setjmp.h>
#include <spng.h>

// :jpeg
typedef struct {
  struct jpeg_error_mgr pub;
  jmp_buf jb;
} JpegErrorMgr;

void jpeg_error_exit(j_common_ptr cinfo);
u8* jpeg_load_scaled(const char* path, i32 min_w, i32* out_w, i32* out_h);

// :png
u8* png_load_spng(const char* path, i32* out_w, i32* out_h);

#endif // __IMG_H__
