#define BASE_IMPLEMENTATION
#include "base.h"

int main() {
  Window window = window_new(
    "Mural", 800, 600, (WindowSettings) {
      .fullscreen = false,
      .vsync = true,
    }
  );

  OCamera camera = ocamera_new(
    (v2) {0,0},
    1,
    (OCamera_Boundary) {
      .left   = 0,
      .right  = 800,
      .top    = 0,
      .bottom = 600,
      .near   = -1.0f,
      .far    = 1000.0f,
    }
  );

  IMR imr = imr_new();

  while (!window.should_close) {
    imr_clear((v4) { 0.5, 0.5, 0.5, 1 });

    m4 mvp = ocamera_calc_mvp(&camera);
    imr_update_mvp(&imr, mvp);

    imr_begin(&imr);
    imr_push_quad_rounded(
      &imr,
      (v3) { 100, 100, 0 },
      (v2) { 200, 50 },
      m4_identity(),
      (v4) { 1, 0, 0, 1 },
      20.0f
    );
    imr_end(&imr);

    window_update(&window);
  }

  imr_delete(&imr);
  window_delete(window);
  return 0;
}
