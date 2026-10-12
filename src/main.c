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
typedef struct {
  Texture texture;
  char* name;
} Wallpaper;

b8 wallpaper_new(Wallpaper* wp, const char* path);
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

void* load_wallpapers(void* _args);
void wallpapers_delete(Wallpapers* wps);

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
b8 wallpaper_new(Wallpaper* wp, const char* path) {
  panic(wp, "Parameter wp cannot be NULL");

  Texture tex;
  if (!texture_from_file(&tex, path, false)) {
    return false;
  }

  char* name = basename((char*) path);
  wp->texture = tex;
  wp->name = mem_alloc(strlen(name) + 1);
  strcpy(wp->name, name);
  return true;
}

void wallpaper_delete(Wallpaper* wp) {
  texture_delete(wp->texture);
  mem_free(wp->name);
}

// :wallpapers impl
void* load_wallpapers(void* _args) {
  ThreadArgs* args = (ThreadArgs*) _args;
  Wallpapers* wps = args->wps;
  Config* cfg = args->cfg;
  pthread_mutex_t* wallpapers_mutex = args->wallpapers_mutex;

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

      Wallpaper wp;
      if (!wallpaper_new(&wp, file_path)) {
        log_warn("Skipping: %s\n", file_path);
        continue;
      }

      // Using mutex lock for changing the wallpapers array
      pthread_mutex_lock(wallpapers_mutex);
      da_append(wps, wp);
      pthread_mutex_unlock(wallpapers_mutex);

      log_info("Loaded: %s\n", file_path);
    }

    closedir(dir);
  }

  return NULL;
}

void wallpapers_delete(Wallpapers* wps) {
  for (i32 i = 0; i < wps->len; i++) {
    wallpaper_delete(&wps->items[i]);
  }

  mem_free(wps->items);
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

  // Setup the wallpaper mutex
  app->wallpapers_mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
  app->wallpapers = (Wallpapers) {0};
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
  pthread_create(&thread, NULL, load_wallpapers, (void*) args);
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

    v2 size = { 10, 10 };
    f32 gap = 50.0f;
    for (i32 i = 0; i < app.wallpapers.len; i++) {
      Wallpaper* wp = &app.wallpapers.items[i];
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
    }

    imr_end(&app.imr);

    window_update(&app.window);
  }

  app_delete(&app);
  return 0;
}
