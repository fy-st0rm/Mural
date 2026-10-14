#include "img.h"

void jpeg_error_exit(j_common_ptr cinfo) {
  JpegErrorMgr* err = (JpegErrorMgr*) cinfo->err;
  longjmp(err->jb, 1);
}

u8* jpeg_load_scaled(const char* path, i32 min_w, i32* out_w, i32* out_h) {
  FILE* file = fopen(path, "rb");
  if (!file) return NULL;

  struct jpeg_decompress_struct cinfo;
  JpegErrorMgr jerr;
  volatile u8* pixels = NULL;

  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = jpeg_error_exit;

  if (setjmp(jerr.jb)) {
    jpeg_destroy_decompress(&cinfo);
    fclose(file);
    free((void*) pixels);
    return NULL;
  }

  jpeg_create_decompress(&cinfo);
  jpeg_stdio_src(&cinfo, file);
  jpeg_read_header(&cinfo, TRUE);

  // Pick the largest downscale denominator that is still >= min_w
  i32 denom = 1;
  if ((i32) cinfo.image_width / 2 >= min_w) denom = 2;
  if ((i32) cinfo.image_width / 4 >= min_w) denom = 4;
  if ((i32) cinfo.image_width / 8 >= min_w) denom = 8;

  cinfo.scale_num = 1;
  cinfo.scale_denom = denom;
  cinfo.out_color_space = JCS_EXT_RGBA;

  jpeg_start_decompress(&cinfo);

  i32 w = cinfo.output_width;
  i32 h = cinfo.output_height;

  u8* buf = malloc(w * h * 4);
  if (!buf) {
    jpeg_destroy_decompress(&cinfo);
    fclose(file);
    return NULL;
  }
  pixels = buf;

  while (cinfo.output_scanline < cinfo.output_height) {
    u8* row = buf + (size_t) cinfo.output_scanline * w * 4;
    jpeg_read_scanlines(&cinfo, &row, 1);
  }

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
  fclose(file);

  *out_w = w;
  *out_h = h;
  return buf;
}

u8* png_load_spng(const char* path, i32* out_w, i32* out_h) {
  FILE* f = fopen(path, "rb");
  if (!f) return NULL;

  spng_ctx* ctx = spng_ctx_new(SPNG_CTX_IGNORE_ADLER32);
  if (!ctx) {
    fclose(f);
    return NULL;
  }

  spng_set_crc_action(ctx, SPNG_CRC_USE, SPNG_CRC_USE);
  spng_set_png_file(ctx, f);

  struct spng_ihdr ihdr;
  if (spng_get_ihdr(ctx, &ihdr)) {
    spng_ctx_free(ctx);
    fclose(f);
    return NULL;
  }

  size_t size;
  if (spng_decoded_image_size(ctx, SPNG_FMT_RGBA8, &size)) {
    spng_ctx_free(ctx);
    fclose(f);
    return NULL;
  }

  u8* buf = malloc(size);
  if (!buf) {
    spng_ctx_free(ctx);
    fclose(f);
    return NULL;
  }

  if (spng_decode_image(ctx, buf, size, SPNG_FMT_RGBA8, 0)) {
    free(buf);
    spng_ctx_free(ctx);
    fclose(f);
    return NULL;
  }

  spng_ctx_free(ctx);
  fclose(f);

  *out_w = ihdr.width;
  *out_h = ihdr.height;
  return buf;
}
