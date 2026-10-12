#define BASE_IMPLEMENTATION
#include "base.h"
#include <dirent.h>
#include <limits.h>
#include <libgen.h>


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
  const char name[NAME_MAX];
} Wallpaper;

Wallpaper* wallpaper_new(const char* path);
void wallpaper_delete(Wallpaper* wp);

// :wallpapers def
typedef struct {
  Wallpaper** items;
  i32 len;
  i32 capacity;
} Wallpapers;

void load_wallpapers(Wallpapers* wps, Config* cfg);
void wallpapers_delete(Wallpapers* wps);

// :app def
typedef struct {
  Window window;
  OCamera camera;
  IMR imr;

  Config config;
  Wallpapers wallpapers;
} App;

void app_new(App* app);
void app_delete(App* app);

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
Wallpaper* wallpaper_new(const char* path) {
  Texture tex;
  if (!texture_from_file(&tex, path, false)) {
    return NULL;
  }

  Wallpaper* wp = mem_alloc(sizeof(Wallpaper));
  wp->texture = tex;
  strcpy((char*) wp->name, basename((char*) path));
  return wp;
}

void wallpaper_delete(Wallpaper* wp) {
  texture_delete(wp->texture);
  mem_free(wp);
}

// :wallpapers impl
void load_wallpapers(Wallpapers* wps, Config* cfg) {
  for (i32 i = 0; i < cfg->paths.len; i++) {
    const char* path = cfg->paths.items[i];
    DIR *dir = opendir(path);
    panic(dir, "Cannot open dir: %s\n", path);

    struct dirent* entry;
    i32 j = 0;
    while ((entry = readdir(dir)) != NULL) {
      //if (j > 5) break;

      char file_path[PATH_MAX];
      sprintf(file_path, "%s/%s", path, entry->d_name);

      Wallpaper* wp = wallpaper_new(file_path);
      da_append(wps, wp);
      log_info("Loaded: %s\n", file_path);
      j++;
    }

    closedir(dir);
  }
}

void wallpapers_delete(Wallpapers* wps) {
  for (i32 i = 0; i < wps->len; i++) {
    wallpaper_delete(wps->items[i]);
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

  // Load the wallpapers
  app->wallpapers = (Wallpapers) {0};
  load_wallpapers(&app->wallpapers, &app->config);
}

void app_delete(App* app) {
  config_delete(&app->config);
  wallpapers_delete(&app->wallpapers);

  imr_delete(&app->imr);
  window_delete(app->window);
}

// :main
int main() {
  App app = {0};
  app_new(&app);

  while (!app.window.should_close) {
    imr_clear((v4) { 0.5, 0.5, 0.5, 1 });

    m4 mvp = ocamera_calc_mvp(&app.camera);
    imr_update_mvp(&app.imr, mvp);

    imr_begin(&app.imr);

    v2 size = { 300, 300 };
    f32 gap = 50.0f;
    for (i32 i = 0; i < app.wallpapers.len; i++) {
      Wallpaper* wp = app.wallpapers.items[i];
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
