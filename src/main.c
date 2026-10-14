#define BASE_IMPLEMENTATION
#include "base.h"
#include <dirent.h>
#include <limits.h>
#include <libgen.h>
#include <pthread.h>

#define THUMBNAIL_MAX 400
#define ATLAS_SIZE (v2) { THUMBNAIL_MAX, 1000 }

// TODOS:
// - Load only thumbnails
// - Create a atlas for thumbnails (make atlas per thread so that we can keep max threads at 31)

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

// :main
int main() {
  App app = {0};
  app_new(&app);
  app_spawn_workers(&app, 20);

  while (!app.window.should_close) {
    imr_clear((v4) { 0.5, 0.5, 0.5, 1 });

    m4 mvp = ocamera_calc_mvp(&app.camera);
    imr_update_mvp(&app.imr, mvp);

    imr_begin(&app.imr);

    f32 gap = 50.0f;
    for (i32 i = 0; i < app.wallpapers.len; i++) {
      Wallpaper* wp = &app.wallpapers.items[i];

      switch (wp->state) {
        case W_PENDING:
          break;

        case W_LOADED:
          if (!atlases_add_wallpaper(&app.atlases, wp)) {
            log_error("Failed to create texture for: %s\n", basename(wp->path));
            continue;
          }
          log_info("TEXTURED: %s\n", basename(wp->path));
          break;

        case W_READY:
          Atlas* at = &app.atlases.items[wp->atlas_idx];
          v2 size = { wp->w, wp->h };
          texture_bind(at->texture);
          imr_push_quad_tex_rounded(
            &app.imr,
            (v3) { i * size.x, 0 },
            size,
            wp->uv,
            at->texture.id,
            m4_identity(),
            (v4) { 1, 1, 1, 1 },
            20.0f
          );
          break;

        case W_FAILED:
          break;
      }
    }

    imr_end(&app.imr);

    window_update(&app.window);
  }

  app_delete(&app);
  return 0;
}
