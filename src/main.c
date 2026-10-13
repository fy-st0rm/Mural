#define BASE_IMPLEMENTATION
#include "base.h"
#include <dirent.h>
#include <limits.h>
#include <libgen.h>
#include <pthread.h>

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
  Texture texture;
  char* path;
  WallpaperState state;

  u8* data;
  i32 w, h;
  i32 channel;
} Wallpaper;

b8 wallpaper_read_from_file(Wallpaper* wp);
b8 wallpaper_create_texture(Wallpaper* wp);
void wallpaper_delete(Wallpaper* wp);

// :wallpapers def
typedef struct {
  Wallpaper* items;    // This owns the Wallpaper memory
  i32 len;
  i32 capacity;
} Wallpapers;

typedef struct {
  Wallpapers* wps;
  Config* cfg;
  pthread_mutex_t* wallpapers_mutex;
} ThreadArgs;

// This will only load up the name of the wallpaper instead of the actual textures
void wallpapers_new(Wallpapers* wps, Config* cfg);
void wallpapers_delete(Wallpapers* wps);
void* load_wallpapers(void* _args);

// :app def
typedef struct {
  Window window;
  OCamera camera;
  IMR imr;

  Config config;

  pthread_mutex_t wallpapers_mutex;
  Wallpapers wallpapers;
} App;

void app_new(App* app);
void app_delete(App* app);
void app_spawn_worker(App* app);

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
  u8* data = stbi_load(wp->path, &w, &h, &c, 0);
  if (!data) {
    log_error("Failed to load file: %s\n", wp->path);
    wp->state = W_FAILED;
    return false;
  }

  wp->data = data;
  wp->w = w;
  wp->h = h;
  wp->channel = c;
  wp->state = W_LOADED;
  return true;
}

b8 wallpaper_create_texture(Wallpaper* wp) {
  panic(wp, "Parameter wp cannot be NULL");

  Texture tex = {0};
  if (!texture_from_data(&tex, wp->w, wp->h, wp->data, wp->channel)) {
    wp->state = W_FAILED;
    return false;
  }

  stbi_image_free(wp->data);

  wp->texture = tex;
  wp->state = W_READY;
  return true;
}

void wallpaper_delete(Wallpaper* wp) {
  texture_delete(wp->texture);
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

void* load_wallpapers_into_memory(void* _args) {
  ThreadArgs* args = (ThreadArgs*) _args;
  Wallpapers* wps = args->wps;
  Config* cfg = args->cfg;
  pthread_mutex_t* wallpapers_mutex = args->wallpapers_mutex;

  for (i32 i = 0; i < wps->len; i++) {
    Wallpaper* wp = &wps->items[i];

    // NOTE(slok): Currently the mutex is not used rn
    //pthread_mutex_lock(wallpapers_mutex);
    if (wp->state == W_PENDING) {
      if (!wallpaper_read_from_file(wp)) {
        log_warn("Skipping: %s\n", wp->path);
        //pthread_mutex_unlock(wallpapers_mutex);
        continue;
      }
    }

    log_info("Loaded: %s\n", wp->path);
    //pthread_mutex_unlock(wallpapers_mutex);
  }

  log_sucess("Done Loading\n");

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

  // Setup the wallpapers and mutex
  app->wallpapers_mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
  app->wallpapers = (Wallpapers) {0};
  wallpapers_new(&app->wallpapers, &app->config);
}

void app_delete(App* app) {
  config_delete(&app->config);
  wallpapers_delete(&app->wallpapers);

  imr_delete(&app->imr);
  window_delete(app->window);
}

void app_spawn_worker(App* app) {
  ThreadArgs* args = mem_alloc(sizeof(ThreadArgs));
  args->wps = &app->wallpapers;
  args->cfg = &app->config;
  args->wallpapers_mutex = &app->wallpapers_mutex;

  pthread_t thread;
  pthread_create(&thread, NULL, load_wallpapers_into_memory, (void*) args);
}

// :main
int main() {
  App app = {0};
  app_new(&app);
  app_spawn_worker(&app);

  while (!app.window.should_close) {
    imr_clear((v4) { 0.5, 0.5, 0.5, 1 });

    m4 mvp = ocamera_calc_mvp(&app.camera);
    imr_update_mvp(&app.imr, mvp);

    imr_begin(&app.imr);

    v2 size = { 300, 300 };
    f32 gap = 50.0f;
    for (i32 i = 0; i < app.wallpapers.len; i++) {
      Wallpaper* wp = &app.wallpapers.items[i];

      switch (wp->state) {
        case W_PENDING:
          break;

        case W_LOADED:
          if (!wallpaper_create_texture(wp)) {
            log_error("Failed to create texture for: %s\n", basename(wp->path));
            continue;
          }
          log_info("TEXTURE: %d: %s\n", wp->texture.id, basename(wp->path));
          break;

        case W_READY:
          texture_bind(wp->texture);
          imr_push_quad_tex_rounded(
            &app.imr,
            (v3) { i * size.x + gap, 100, 0 },
            size,
            (Rect) { 0, 0, 1, 1 },
            wp->texture.id,
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
