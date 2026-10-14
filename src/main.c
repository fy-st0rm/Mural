#include "base.h"
#include <dirent.h>
#include <limits.h>
#include <libgen.h>
#include <pthread.h>

#define THUMBNAIL_MAX 100
#define ATLAS_SIZE (v2) { THUMBNAIL_MAX, 10000 }

//===========================
// DEFINITIONS BEGINS HERE!
//===========================

// :config def
typedef struct {
  const char** items;
  i32 len;
  i32 capacity;
} WallpaperPaths;

typedef struct {
  WallpaperPaths paths;
} Config;

void config_new(Config* cfg);
void config_delete(Config* cfg);

// :wallpaper def
typedef enum {
  W_PENDING,
  W_LOADED,
  W_READY,
  W_FAILED,
} WallpaperState;

typedef struct {
  char* path;
  WallpaperState state;

  i32 atlas_idx;
  Rect uv;

  u8* data;
  i32 w, h;
  i32 channel;
} Wallpaper;

b8 wallpaper_read_from_file(Wallpaper* wp);
void wallpaper_delete(Wallpaper* wp);

// :wallpapers def
typedef struct {
  Wallpaper* items;    // This owns the Wallpaper memory
  i32 len;
  i32 capacity;
} Wallpapers;

// This will only load up the name of the wallpaper instead of the actual textures
void wallpapers_new(Wallpapers* wps, Config* cfg);
void wallpapers_delete(Wallpapers* wps);

// :atlas def
typedef struct {
  Texture texture;
  u32 width, height;
  u32 curr_height;
} Atlas;

typedef struct {
  Atlas* items;
  i32 len;
  i32 capacity;
} Atlases;

void atlases_add_new(Atlases* atlases, v2 size);
b8 atlases_add_wallpaper(Atlases* atlases, Wallpaper* wp);
void atlases_delete(Atlases* atlases);

// :worker def
typedef struct {
  i32 worker_id;
  Wallpapers* wps;
  i32 start, end;
  pthread_mutex_t* wallpapers_mutex;
} ThreadArgs;

void* worker_load_wallpapers_into_memory(void* _args);

// :app def
typedef struct {
  Window window;
  OCamera camera;
  IMR imr;

  Config config;
  Atlases atlases;

  pthread_mutex_t wallpapers_mutex;
  Wallpapers wallpapers;
} App;

void app_new(App* app);
void app_delete(App* app);
void app_spawn_workers(App* app, i32 workers);

//=============================
// IMPLEMENTATION BEGINS HERE!
//=============================

// :config impl
void config_new(Config* cfg) {
  da_append(&cfg->paths, strdup("/home/st0rm/nest/sos/Wallpapers"));
}

void config_delete(Config* cfg) {
  mem_free(cfg->paths.items);
}

// :wallpaper impl
b8 wallpaper_read_from_file(Wallpaper* wp) {
  panic(wp, "Parameter wp cannot be NULL");

  i32 w, h, c;
  i32 rgba = 4;
  u8* full_data = stbi_load(wp->path, &w, &h, &c, rgba);
  if (!full_data) {
    log_error("Failed to load file: %s\n", wp->path);
    wp->state = W_FAILED;
    return false;
  }

  // Maintaing the aspect ratio
  f32 scale = (f32)w / (f32)h;
  i32 tw = THUMBNAIL_MAX;
  i32 th = (i32)((f32)tw / scale);

  u8* thumb_data = mem_alloc(tw * th * 4);
  if (!thumb_data) {
    stbi_image_free(full_data);
    wp->state = W_FAILED;
    return false;
  }

  stbir_resize_uint8_linear(full_data, w, h, 0, thumb_data, tw, th, 0, STBIR_RGBA);
  stbi_image_free(full_data);

  wp->data = thumb_data;
  wp->w = tw;
  wp->h = th;
  wp->channel = TEX_RGBA;
  wp->state = W_LOADED;
  return true;
}

void wallpaper_delete(Wallpaper* wp) {
  mem_free(wp->data);
  free(wp->path);
}

// :wallpapers impl
void wallpapers_new(Wallpapers* wps, Config* cfg) {
  for (i32 i = 0; i < cfg->paths.len; i++) {
    const char* path = cfg->paths.items[i];
    DIR *dir = opendir(path);
    if (!dir) {
      log_error("Cannot open dir: %s\n", path);
      continue;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
      char file_path[PATH_MAX];
      sprintf(file_path, "%s/%s", path, entry->d_name);

      Wallpaper wp = {
        .path  = strdup(file_path),
        .state = W_PENDING,
      };
      da_append(wps, wp);

      log_info("Found: %s\n", file_path);
    }

    closedir(dir);
  }
}

void wallpapers_delete(Wallpapers* wps) {
  for (i32 i = 0; i < wps->len; i++) {
    wallpaper_delete(&wps->items[i]);
  }

  mem_free(wps->items);
}

// :atlas impl
void atlases_add_new(Atlases* atlases, v2 size) {
  Atlas atlas = {
    .texture = (Texture) {0},
    .width = size.x,
    .height = size.y,
    .curr_height = 0,
  };

  panic(
    texture_from_data(&atlas.texture, atlas.width, atlas.height, NULL, TEX_RGBA),
    "Failed to create the first atlas"
  );

  da_append(atlases, atlas);
}

b8 atlases_add_wallpaper(Atlases* atlases, Wallpaper* wp) {
  Atlas* atlas = &atlases->items[atlases->len - 1];

  // Create a new atlas when it runs out of space
  if (wp->h > atlas->height - atlas->curr_height) {
    atlases_add_new(atlases, ATLAS_SIZE);
    atlas = &atlases->items[atlases->len - 1];
  }

  i32 y0 = atlas->curr_height; 

  // Update the atlas
  texture_update(
    atlas->texture,
    0,
    (v2) { 0, y0 },
    (v2) { wp->w, wp->h },
    wp->data
  );

  // Adjust the atlas pointer
  atlas->curr_height += wp->h;

  wp->atlas_idx = atlases->len - 1;
  wp->uv = (Rect) {
    0.0f,
    (f32) y0 / (f32) atlas->height,
    (f32) wp->w / (f32) atlas->width,
    (f32) wp->h / (f32) atlas->height
  };
  wp->state = W_READY;
  return true;
}

void atlases_delete(Atlases* atlases) {
  for (i32 i = 0; i < atlases->len; i++) {
    texture_delete(atlases->items[i].texture);
  }
}

// :worker impl
void* worker_load_wallpapers_into_memory(void* _args) {
  ThreadArgs* args = (ThreadArgs*) _args;
  Wallpapers* wps = args->wps;
  pthread_mutex_t* wallpapers_mutex = args->wallpapers_mutex;

  for (i32 i = args->start; i < args->end; i++) {
    Wallpaper* wp = &wps->items[i];

    if (wp->state == W_PENDING) {
      if (!wallpaper_read_from_file(wp)) {
        log_warn("Skipping: %s\n", wp->path);
        continue;
      }
    }

    log_info("Loaded: %s\n", wp->path);
  }

  log_sucess("Worker Done: %d\n", args->worker_id);

  return NULL;
}

// :app impl
void app_new(App* app) {
  panic(window_new(
    &app->window,
    "Mural", 800, 600, (WindowSettings) {
      .fullscreen = false,
      .vsync = true,
    }
  ), "Failed to create window\n");

  app->camera = ocamera_new(
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

  panic(imr_new(&app->imr), "Failed to initialize renderer");

  // Load the config
  app->config = (Config) {0};
  config_new(&app->config);

  // Create the first atlas
  app->atlases = (Atlases) {0};
  atlases_add_new(&app->atlases, ATLAS_SIZE);

  // Setup the wallpapers and mutex
  app->wallpapers_mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
  app->wallpapers = (Wallpapers) {0};
  wallpapers_new(&app->wallpapers, &app->config);
}

void app_delete(App* app) {
  log_info("Atlases used: %d\n", app->atlases.len);

  atlases_delete(&app->atlases);
  wallpapers_delete(&app->wallpapers);
  config_delete(&app->config);

  imr_delete(&app->imr);
  window_delete(app->window);
}

void app_spawn_workers(App* app, i32 workers) {
  i32 per_worker = app->wallpapers.len / workers;
  i32 remainder = app->wallpapers.len % workers;

  i32 start = 0;
  for (i32 i = 0; i < workers; i++) {
    i32 chunk = per_worker + (i < remainder ? 1 : 0);
    i32 end = start + chunk;

    ThreadArgs* args = mem_alloc(sizeof(ThreadArgs));
    args->worker_id = i + 1;
    args->wps = &app->wallpapers;
    args->wallpapers_mutex = &app->wallpapers_mutex;
    args->start = start;
    args->end = end;

    pthread_t thread;
    pthread_create(&thread, NULL, worker_load_wallpapers_into_memory, (void*) args);

    start = end;
  }
}

#define CELL_W   100.0f
#define CELL_H   100.0f
#define GAP      20.0f
#define MARGIN   20.0f
#define MAX_UPLOADS_PER_FRAME 4

// Crop the atlas UV so the image covers the cell without stretching
static Rect uv_cover(Rect uv, f32 img_w, f32 img_h, f32 cell_w, f32 cell_h) {
  f32 img_aspect  = img_w / img_h;
  f32 cell_aspect = cell_w / cell_h;

  if (img_aspect > cell_aspect) {          // image is wider: crop left/right
    f32 keep = cell_aspect / img_aspect;
    uv.x += uv.w * (1.0f - keep) * 0.5f;
    uv.w *= keep;
  } else {                                 // image is taller: crop top/bottom
    f32 keep = img_aspect / cell_aspect;
    uv.y += uv.h * (1.0f - keep) * 0.5f;
    uv.h *= keep;
  }
  return uv;
}

int main() {
  App app = {0};
  app_new(&app);
  app_spawn_workers(&app, 8);

  f32 scroll_y = 0;   // hook this up to a GLFW scroll callback later

  while (!app.window.should_close) {
    imr_clear((v4) { 0.5, 0.5, 0.5, 1 });

    m4 mvp = ocamera_calc_mvp(&app.camera);
    imr_update_mvp(&app.imr, mvp);

    // ---- 1. Upload pass (budgeted, no layout involved) ----
    i32 uploads = 0;
    for (i32 i = 0; i < app.wallpapers.len && uploads < MAX_UPLOADS_PER_FRAME; i++) {
      Wallpaper* wp = &app.wallpapers.items[i];
      if (wp->state != W_LOADED) continue;

      uploads++;
      if (!atlases_add_wallpaper(&app.atlases, wp)) {
        log_error("Failed to create texture for: %s\n", basename(wp->path));
        wp->state = W_FAILED;
      }
    }

    // ---- 2. Static grid layout, derived only from the index ----
    i32 n       = app.wallpapers.len;
    f32 win_w   = (f32) app.window.width;
    f32 win_h   = (f32) app.window.height;
    f32 stride_x = CELL_W + GAP;
    f32 stride_y = CELL_H + GAP;

    i32 cols = (i32)((win_w - 2 * MARGIN + GAP) / stride_x);
    if (cols < 1) cols = 1;
    i32 rows = (n + cols - 1) / cols;

    // Clamp scrolling to the content
    f32 content_h = MARGIN * 2 + rows * stride_y - GAP;
    f32 max_scroll = content_h - win_h;
    if (max_scroll < 0) max_scroll = 0;
    if (scroll_y < 0) scroll_y = 0;
    if (scroll_y > max_scroll) scroll_y = max_scroll;

    // Only visit rows that are on screen
    i32 first_row = (i32)((scroll_y - MARGIN) / stride_y);
    i32 last_row  = (i32)((scroll_y + win_h - MARGIN) / stride_y);
    if (first_row < 0) first_row = 0;
    if (last_row >= rows) last_row = rows - 1;

    imr_begin(&app.imr);

    for (i32 row = first_row; row <= last_row; row++) {
      for (i32 col = 0; col < cols; col++) {
        i32 i = row * cols + col;
        if (i >= n) break;

        Wallpaper* wp = &app.wallpapers.items[i];
        v3 pos = {
          MARGIN + col * stride_x,
          MARGIN + row * stride_y - scroll_y,
          0
        };
        v2 size = { CELL_W, CELL_H };

        if (wp->state == W_READY) {
          Atlas* at = &app.atlases.items[wp->atlas_idx];
          texture_bind(at->texture);
          imr_push_quad_tex_rounded(
            &app.imr, pos, size,
            uv_cover(wp->uv, wp->w, wp->h, CELL_W, CELL_H),
            at->texture.id,
            m4_identity(), (v4){ 1, 1, 1, 1 }, 20.0f
          );
        } else {
          // Placeholder keeps the grid visible while loading
          v4 c = (wp->state == W_FAILED) ? (v4){ 0.5f, 0.2f, 0.2f, 1 }
                                         : (v4){ 0.35f, 0.35f, 0.35f, 1 };
          imr_push_quad_rounded(&app.imr, pos, size, m4_identity(), c, 20.0f);
        }
      }
    }

    imr_end(&app.imr);
    window_update(&app.window);
  }

  app_delete(&app);
  return 0;
}

// :main
/*
int main() {
  App app = {0};
  app_new(&app);
  app_spawn_workers(&app, 20);

  while (!app.window.should_close) {
    imr_clear((v4) { 0.5, 0.5, 0.5, 1 });

    m4 mvp = ocamera_calc_mvp(&app.camera);
    imr_update_mvp(&app.imr, mvp);

    imr_begin(&app.imr);

    f32 gap = 0.0f;

    f32 x = 0;
    f32 y = 0;
    for (i32 i = 0; i < app.wallpapers.len; i++) {
      Wallpaper* wp = &app.wallpapers.items[i];

      v3 pos = { x, y, 0 };
      // print_v3(pos);

      switch (wp->state) {
        case W_PENDING:
          break;

        case W_LOADED: {
          if (!atlases_add_wallpaper(&app.atlases, wp)) {
            log_error("Failed to create texture for: %s\n", basename(wp->path));
            continue;
          }
          log_info("TEXTURED: %s\n", basename(wp->path));
        };

        case W_READY: {
          Atlas* at = &app.atlases.items[wp->atlas_idx];
          v2 size = { wp->w, wp->h };
          texture_bind(at->texture);
          imr_push_quad_tex_rounded(
            &app.imr,
            pos,
            size,
            wp->uv,
            at->texture.id,
            m4_identity(),
            (v4) { 1, 1, 1, 1 },
            20.0f
          );
        } break;

        case W_FAILED:
          break;
      }


      x += wp->w + gap;
      if (x + wp->w > 800) {
        x = 0;
        y += wp->h + gap;
      }


    }

    imr_end(&app.imr);

    window_update(&app.window);
  }

  app_delete(&app);
  return 0;
}
*/
