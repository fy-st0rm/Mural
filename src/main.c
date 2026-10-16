#include <dirent.h>
#include <limits.h>
#include <libgen.h>
#include <pthread.h>
#include <spawn.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <tomlc17.h>

#include "base.h"
#include "img.h"

//===========================
// DEFINITIONS BEGINS HERE!
//===========================

// :config def
typedef struct {
  f32 scroll_speed;
  const char* font_file;
  f32 font_size;
} GeneralConfig;

typedef struct {
  f32 thumbnail_width;
  f32 thumbnail_radius;
  f32 margin;
  f32 inner_gap;
  f32 border_thickness;
} LayoutConfig;

typedef struct {
  u32 background;
  u32 border_select;
  u32 border_unselect;
  u32 font_color;
} ColorConfig;

typedef struct {
  const char** items;
  i32 len;
  i32 capacity;
} WallpaperPaths;

typedef struct {
  GeneralConfig general;
  LayoutConfig layout;
  ColorConfig color;
  WallpaperPaths paths;

  char* selected_wallpaper;

  char* config_dir;
  char* config_path;

  v2 atlas_size;
} Config;

b8 config_new(Config* cfg);
void config_save(Config* cfg);
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
  f32 x, y;
  i32 w, h;
  i32 full_w, full_h;
  i32 channel;
} Wallpaper;

b8 wallpaper_read_from_file(Wallpaper* wp);
void wallpaper_delete(Wallpaper* wp);
b8 wallpaper_apply(Wallpaper* wp);

// :wallpapers def
typedef struct {
  Wallpaper* items;
  i32 len;
  i32 capacity;
  i32 top;
} Wallpapers;

void wallpapers_new(Wallpapers* wps, Config* cfg);
void wallpapers_delete(Wallpapers* wps);
Wallpaper* wallpapers_dequeue(Wallpapers* wps);

// :atlas def
typedef struct {
  FBO fbo;
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
  pthread_mutex_t* wallpapers_mutex;
} ThreadArgs;

void* worker_load_wallpapers_into_memory(void* _args);

// :app def
typedef struct {
  Window window;
  OCamera camera;
  IMR imr;

  Font font;
  Atlases atlases;

  pthread_mutex_t wallpapers_mutex;
  Wallpapers wallpapers;

  pthread_t* workers;
  i32 worker_count;
} App;

void app_new(App* app);
void app_delete(App* app);
void app_spawn_workers(App* app, i32 workers);
void app_wait_workers(App* app);
void app_create_layout(App* app);
void app_clamp_camera(App* app);
void app_select_wallpaper(App* app, Wallpaper* wp);

//=============================
// IMPLEMENTATION BEGINS HERE!
//=============================

// :config impl
Config config = {0};

static void config_write_string(FILE* file, const char* str) {
  fputc('"', file);

  for (const char* p = str; *p; p++) {
    switch (*p) {
      case '\\':
        fputs("\\\\", file);
        break;

      case '"':
        fputs("\\\"", file);
        break;

      case '\n':
        fputs("\\n", file);
        break;

      case '\r':
        fputs("\\r", file);
        break;

      case '\t':
        fputs("\\t", file);
        break;

      default:
        fputc(*p, file);
        break;
    }
  }

  fputc('"', file);
}

static void config_defaults(Config* cfg) {
  *cfg = (Config) {
    .general = {
      .scroll_speed = 30.0f,
      .font_file = strdup("font.otf"),
      .font_size = 120.0f,
    },

    .layout = {
      .thumbnail_width = 120.0f,
      .thumbnail_radius = 10.0f,
      .margin = 10.0f,
      .inner_gap = 7.0f,
      .border_thickness = 2.0f,
    },

    .color = {
      .background      = 0x12171aff,
      .border_select   = 0x7dd3fcff,
      .border_unselect = 0x334155ff,
      .font_color      = 0xe2e8f0ff,
    },

    .atlas_size = { 120.0f, 10000.0f },
  };
}

static char* config_get_dir(void) {
  const char* home = getenv("HOME");

  if (!home)
    return NULL;

  char* path = mem_alloc(PATH_MAX);
  snprintf(path, PATH_MAX, "%s/.config/mural", home);

  return path;
}

static char* config_get_path(const char* dir) {
  char* path = mem_alloc(PATH_MAX);
  snprintf(path, PATH_MAX, "%s/mural.toml", dir);

  return path;
}

static b8 config_make_dir(const char* dir) {
  const char* home = getenv("HOME");

  if (!home)
    return false;

  char config_dir[PATH_MAX];
  snprintf(config_dir, sizeof(config_dir), "%s/.config", home);

  if (mkdir(config_dir, 0755) != 0 && errno != EEXIST) {
    log_error("Failed to create directory: %s\n", config_dir);
    return false;
  }

  if (mkdir(dir, 0755) != 0 && errno != EEXIST) {
    log_error("Failed to create directory: %s\n", dir);
    return false;
  }

  return true;
}

static b8 config_copy_file(const char* src, const char* dst) {
  FILE* input = fopen(src, "rb");

  if (!input)
    return false;

  FILE* output = fopen(dst, "wb");

  if (!output) {
    fclose(input);
    return false;
  }

  char buffer[8192];
  size_t n;

  while ((n = fread(buffer, 1, sizeof(buffer), input)) > 0) {
    if (fwrite(buffer, 1, n, output) != n) {
      fclose(input);
      fclose(output);
      return false;
    }
  }

  fclose(input);

  if (fclose(output) != 0)
    return false;

  return true;
}

static b8 config_copy_font(const char* config_dir) {
  char exe_path[PATH_MAX];

  ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);

  if (len <= 0)
    return false;

  exe_path[len] = '\0';

  char* exe_dir = dirname(exe_path);

  char src[PATH_MAX];
  char dst[PATH_MAX];

  // bin/mural -> ../font.otf
  snprintf(src, sizeof(src), "%s/../font.otf", exe_dir);
  snprintf(dst, sizeof(dst), "%s/font.otf", config_dir);

  if (access(dst, F_OK) == 0)
    return true;

  if (!config_copy_file(src, dst)) {
    log_warn("Failed to copy font: %s\n", src);
    return false;
  }

  return true;
}

b8 config_new(Config* cfg) {
  config_defaults(cfg);

  cfg->config_dir = config_get_dir();

  if (!cfg->config_dir)
    return false;

  if (!config_make_dir(cfg->config_dir)) {
    config_delete(cfg);
    return false;
  }

  // Copy font into the config directory.
  config_copy_font(cfg->config_dir);

  cfg->config_path = config_get_path(cfg->config_dir);

  // Create default config if it does not exist.
  if (access(cfg->config_path, F_OK) != 0) {
    config_save(cfg);
    return true;
  }

  toml_result_t result = toml_parse_file_ex(cfg->config_path);

  if (!result.ok) {
    log_error("Failed to parse config: %s\n", result.errmsg);
    config_delete(cfg);
    return false;
  }

  // General
  toml_datum_t scroll_speed = toml_seek(result.toptab, "general.scroll_speed");
  toml_datum_t font_file = toml_seek(result.toptab, "general.font_file");
  toml_datum_t font_size = toml_seek(result.toptab, "general.font_size");

  if (scroll_speed.type == TOML_FP64)
    cfg->general.scroll_speed = (f32)scroll_speed.u.fp64;

  if (font_file.type == TOML_STRING) {
    free((void*)cfg->general.font_file);
    cfg->general.font_file = strdup(font_file.u.s);
  }

  if (font_size.type == TOML_FP64)
    cfg->general.font_size = (f32)font_size.u.fp64;

  // Layout
  toml_datum_t thumbnail_width = toml_seek(result.toptab, "layout.thumbnail_width");
  toml_datum_t thumbnail_radius = toml_seek(result.toptab, "layout.thumbnail_radius");
  toml_datum_t margin = toml_seek(result.toptab, "layout.margin");
  toml_datum_t inner_gap = toml_seek(result.toptab, "layout.inner_gap");
  toml_datum_t border_thickness = toml_seek(result.toptab, "layout.border_thickness");

  if (thumbnail_width.type == TOML_FP64)
    cfg->layout.thumbnail_width = (f32)thumbnail_width.u.fp64;

  if (thumbnail_radius.type == TOML_FP64)
    cfg->layout.thumbnail_radius = (f32)thumbnail_radius.u.fp64;

  if (margin.type == TOML_FP64)
    cfg->layout.margin = (f32)margin.u.fp64;

  if (inner_gap.type == TOML_FP64)
    cfg->layout.inner_gap = (f32)inner_gap.u.fp64;

  if (border_thickness.type == TOML_FP64)
    cfg->layout.border_thickness = (f32)border_thickness.u.fp64;

  // Color
  toml_datum_t background = toml_seek(result.toptab, "color.background");
  toml_datum_t border_select = toml_seek(result.toptab, "color.border_select");
  toml_datum_t border_unselect = toml_seek(result.toptab, "color.border_unselect");
  toml_datum_t font_color = toml_seek(result.toptab, "color.font_color");

  if (background.type == TOML_INT64)
    cfg->color.background = (u32)background.u.int64;

  if (border_select.type == TOML_INT64)
    cfg->color.border_select = (u32)border_select.u.int64;

  if (border_unselect.type == TOML_INT64)
    cfg->color.border_unselect = (u32)border_unselect.u.int64;

  if (font_color.type == TOML_INT64)
    cfg->color.font_color = (u32)font_color.u.int64;

  // Wallpapers
  toml_datum_t paths = toml_seek(result.toptab, "wallpapers.paths");

  if (paths.type == TOML_ARRAY) {
    for (i32 i = 0; i < paths.u.arr.size; i++) {
      toml_datum_t path = paths.u.arr.elem[i];

      if (path.type == TOML_STRING)
        da_append(&cfg->paths, strdup(path.u.s));
    }
  }

  // Selected wallpaper
  toml_datum_t selected = toml_seek(result.toptab, "wallpapers.selected_wallpaper");

  if (selected.type == TOML_STRING)
    cfg->selected_wallpaper = strdup(selected.u.s);

  cfg->atlas_size = (v2) {
    cfg->layout.thumbnail_width,
    10000
  };

  toml_free(result);

  // Resolve font relative to the config directory.
  if (cfg->general.font_file && cfg->general.font_file[0] != '/') {
    char font_path[PATH_MAX];

    snprintf(
      font_path,
      sizeof(font_path),
      "%s/%s",
      cfg->config_dir,
      cfg->general.font_file
    );

    free((void*)cfg->general.font_file);
    cfg->general.font_file = strdup(font_path);
  }

  return true;
}

void config_save(Config* cfg) {
  char temp_path[PATH_MAX];
  snprintf(temp_path, sizeof(temp_path), "%s.tmp", cfg->config_path);

  FILE* file = fopen(temp_path, "w");

  if (!file) {
    log_error("Failed to save config: %s\n", cfg->config_path);
    return;
  }

  fprintf(file,
    "[general]\n"
    "scroll_speed = %.1f\n"
    "font_file = \"font.otf\"\n"
    "font_size = %.1f\n"
    "\n"
    "[layout]\n"
    "thumbnail_width  = %.1f\n"
    "thumbnail_radius = %.1f\n"
    "margin           = %.1f\n"
    "inner_gap        = %.1f\n"
    "border_thickness = %.1f\n"
    "\n"
    "[color]\n"
    "background      = 0x%08x\n"
    "border_select   = 0x%08x\n"
    "border_unselect = 0x%08x\n"
    "font_color      = 0x%08x\n"
    "\n"
    "[wallpapers]\n"
    "paths = [\n",
    cfg->general.scroll_speed,
    cfg->general.font_size,
    cfg->layout.thumbnail_width,
    cfg->layout.thumbnail_radius,
    cfg->layout.margin,
    cfg->layout.inner_gap,
    cfg->layout.border_thickness,
    cfg->color.background,
    cfg->color.border_select,
    cfg->color.border_unselect,
    cfg->color.font_color
  );

  for (i32 i = 0; i < cfg->paths.len; i++) {
    fputs("  ", file);
    config_write_string(file, cfg->paths.items[i]);

    if (i + 1 < cfg->paths.len)
      fputc(',', file);

    fputc('\n', file);
  }

  fprintf(file, "]\n");

  if (cfg->selected_wallpaper) {
    fputs("selected_wallpaper = ", file);
    config_write_string(file, cfg->selected_wallpaper);
    fputc('\n', file);
  }

  if (fclose(file) != 0) {
    log_error("Failed to save config: %s\n", cfg->config_path);
    remove(temp_path);
    return;
  }

  if (rename(temp_path, cfg->config_path) != 0) {
    log_error("Failed to replace config: %s\n", cfg->config_path);
    remove(temp_path);
  }
}

void config_delete(Config* cfg) {
  for (i32 i = 0; i < cfg->paths.len; i++)
    free((void*)cfg->paths.items[i]);

  mem_free(cfg->paths.items);

  free((void*)cfg->general.font_file);
  free(cfg->selected_wallpaper);
  free(cfg->config_dir);
  free(cfg->config_path);

  cfg->paths = (WallpaperPaths) {0};
  cfg->general.font_file = NULL;
  cfg->selected_wallpaper = NULL;
  cfg->config_dir = NULL;
  cfg->config_path = NULL;
}

// :wallpaper impl
b8 wallpaper_read_from_file(Wallpaper* wp) {
  panic(wp, "Parameter wp cannot be NULL");

  i32 w, h, c;

  u8* full_data = jpeg_load_scaled(wp->path, config.layout.thumbnail_width * 2, &w, &h);

  if (!full_data)
    full_data = png_load_spng(wp->path, &w, &h);

  if (!full_data)
    full_data = stbi_load(wp->path, &w, &h, &c, 4);

  if (!full_data) {
    log_error("Failed to load file: %s\n", wp->path);
    wp->state = W_FAILED;
    return false;
  }

  wp->data = full_data;
  wp->full_w = w;
  wp->full_h = h;
  wp->channel = TEX_RGBA;
  wp->state = W_LOADED;

  return true;
}

void wallpaper_delete(Wallpaper* wp) {
  free(wp->data);
  free(wp->path);
}

b8 wallpaper_apply(Wallpaper* wp) {
  if (!wp || !wp->path)
    return false;

  if (access(wp->path, R_OK) != 0) {
    log_error("Wallpaper does not exist: %s\n", wp->path);
    return false;
  }

  pid_t pid;
  char* const args[] = { "feh", "--bg-fill", wp->path, NULL };
  int result = posix_spawnp(&pid, "feh", NULL, NULL, args, environ);

  if (result != 0) {
    log_error("Failed to start feh: %s\n", wp->path);
    return false;
  }

  int status;

  if (waitpid(pid, &status, 0) < 0) {
    log_error("Failed to wait for feh\n");
    return false;
  }

  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    log_error("feh failed: %s\n", wp->path);
    return false;
  }

  return true;
}

// :wallpapers impl
void wallpapers_new(Wallpapers* wps, Config* cfg) {
  for (i32 i = 0; i < cfg->paths.len; i++) {
    const char* path = cfg->paths.items[i];

    DIR* dir = opendir(path);

    if (!dir) {
      log_error("Cannot open dir: %s\n", path);
      continue;
    }

    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL) {
      if (entry->d_name[0] == '.')
        continue;

      char file_path[PATH_MAX];
      snprintf(file_path, sizeof(file_path), "%s/%s", path, entry->d_name);

      Wallpaper wp = {
        .path = strdup(file_path),
        .state = W_PENDING
      };

      // Query image dimensions without loading the image
      i32 fw, fh, fc;

      if (!stbi_info(file_path, &fw, &fh, &fc)) {
        free(wp.path);
        continue;
      }

      wp.w = config.layout.thumbnail_width;
      wp.h = (i32)((f32)fh * config.layout.thumbnail_width / fw);

      if (wp.h < 1)
        wp.h = 1;

      da_append(wps, wp);
      log_info("Found: %s\n", file_path);
    }

    closedir(dir);
  }
}

void wallpapers_delete(Wallpapers* wps) {
  for (i32 i = 0; i < wps->len; i++)
    wallpaper_delete(&wps->items[i]);

  mem_free(wps->items);
}

Wallpaper* wallpapers_dequeue(Wallpapers* wps) {
  if (wps->top >= wps->len)
    return NULL;

  return &wps->items[wps->top++];
}

// :atlas impl
void atlases_add_new(Atlases* atlases, v2 size) {
  Atlas atlas = {
    .fbo = (FBO) {0},
    .width = size.x,
    .height = size.y,
    .curr_height = 0,
  };

  panic(fbo_new(&atlas.fbo, atlas.width, atlas.height), "Failed to create atlas");

  da_append(atlases, atlas);
}

b8 atlases_add_wallpaper(Atlases* atlases, Wallpaper* wp) {
  Atlas* atlas = &atlases->items[atlases->len - 1];

  // Create a new atlas when it runs out of space
  if (wp->h > atlas->height - atlas->curr_height) {
    atlases_add_new(atlases, config.atlas_size);
    atlas = &atlases->items[atlases->len - 1];
  }

  i32 y0 = atlas->curr_height;

  // Upload the image into a temporary texture
  Texture full;

  if (!texture_from_data(&full, wp->full_w, wp->full_h, wp->data, TEX_RGBA))
    return false;

  u32 src_fbo;

  GLCall(glGenFramebuffers(1, &src_fbo));
  GLCall(glBindFramebuffer(GL_READ_FRAMEBUFFER, src_fbo));
  GLCall(glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, full.id, 0));
  GLCall(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, atlas->fbo.id));

  GLCall(glBlitFramebuffer(
    0, 0, wp->full_w, wp->full_h,
    0, y0, wp->w, y0 + wp->h,
    GL_COLOR_BUFFER_BIT, GL_LINEAR
  ));

  GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
  GLCall(glDeleteFramebuffers(1, &src_fbo));

  texture_delete(full);

  free(wp->data);
  wp->data = NULL;

  atlas->curr_height += wp->h;

  wp->atlas_idx = atlases->len - 1;
  wp->uv = (Rect) {
    0.0f,
    (f32)y0 / (f32)atlas->height,
    (f32)wp->w / (f32)atlas->width,
    (f32)wp->h / (f32)atlas->height
  };

  wp->state = W_READY;

  return true;
}

void atlases_delete(Atlases* atlases) {
  for (i32 i = 0; i < atlases->len; i++)
    fbo_delete(&atlases->items[i].fbo);

  mem_free(atlases->items);
}

// :worker impl
void* worker_load_wallpapers_into_memory(void* _args) {
  ThreadArgs* args = (ThreadArgs*)_args;

  while (true) {
    pthread_mutex_lock(args->wallpapers_mutex);
    Wallpaper* wp = wallpapers_dequeue(args->wps);
    pthread_mutex_unlock(args->wallpapers_mutex);

    if (!wp)
      break;

    if (wp->state != W_PENDING)
      continue;

    if (!wallpaper_read_from_file(wp)) {
      log_warn("Skipping: %s\n", wp->path);
      continue;
    }

    log_info("Loaded: %s\n", wp->path);
  }

  log_sucess("Worker Done: %d\n", args->worker_id);
  free(args);

  return NULL;
}

// :app impl
void app_new(App* app) {
  panic(window_new(
    &app->window,
    "Mural",
    800,
    600,
    (WindowSettings) {
      .fullscreen = false,
      .vsync = false,
      .resizable = true,
    }
  ), "Failed to create window\n");

  app->camera = ocamera_new(
    (v2) { 0, 0 },
    1,
    (OCamera_Boundary) {
      .left = 0,
      .right = app->window.width,
      .top = 0,
      .bottom = app->window.height,
      .near = -1.0f,
      .far = 1000.0f,
    }
  );

  panic(imr_new(&app->imr), "Failed to initialize renderer");

  // Load font
  app->font = (Font) {0};
  panic(font_new(&app->font, config.general.font_file, config.general.font_size), "Failed to load font");

  // Create the first atlas
  app->atlases = (Atlases) {0};
  atlases_add_new(&app->atlases, config.atlas_size);

  // Setup wallpapers and mutex
  app->wallpapers_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
  app->wallpapers = (Wallpapers) {0};
  wallpapers_new(&app->wallpapers, &config);
}

void app_delete(App* app) {
  app_wait_workers(app);

  log_info("Atlases used: %d\n", app->atlases.len);

  atlases_delete(&app->atlases);
  wallpapers_delete(&app->wallpapers);
  config_delete(&config);

  imr_delete(&app->imr);
  window_delete(app->window);

  pthread_mutex_destroy(&app->wallpapers_mutex);
}

void app_spawn_workers(App* app, i32 workers) {
  if (workers < 1)
    workers = 1;

  if (app->wallpapers.len == 0)
    return;

  if (workers > app->wallpapers.len)
    workers = app->wallpapers.len;

  app->workers = mem_alloc(sizeof(pthread_t) * workers);
  app->worker_count = workers;

  for (i32 i = 0; i < workers; i++) {
    ThreadArgs* args = mem_alloc(sizeof(ThreadArgs));

    args->worker_id = i + 1;
    args->wps = &app->wallpapers;
    args->wallpapers_mutex = &app->wallpapers_mutex;

    pthread_create(&app->workers[i], NULL, worker_load_wallpapers_into_memory, args);
  }
}

void app_wait_workers(App* app) {
  for (i32 i = 0; i < app->worker_count; i++)
    pthread_join(app->workers[i], NULL);

  free(app->workers);

  app->workers = NULL;
  app->worker_count = 0;
}

void app_create_layout(App* app) {
  f32 window_width = (f32)app->window.width;

  // Calculate usable width
  f32 available_width = window_width - 2.0f * config.layout.margin;

  // Calculate how many thumbnails fit
  i32 per_row = (i32)((available_width + config.layout.inner_gap) / (config.layout.thumbnail_width + config.layout.inner_gap));

  if (per_row < 1)
    per_row = 1;

  // Calculate gap needed to fill the row
  f32 gap;

  if (per_row == 1) {
    gap = 0.0f;
  } else {
    gap = (available_width - per_row * config.layout.thumbnail_width) / (per_row - 1);
  }

  // Starting position
  f32 x = config.layout.margin;
  f32 y = config.layout.margin;

  i32 row_start = 0;

  while (row_start < app->wallpapers.len) {
    // Find the end of the current row
    i32 row_end = row_start;
    i32 max_h = 0;

    while (row_end < app->wallpapers.len && row_end - row_start < per_row) {
      Wallpaper* wp = &app->wallpapers.items[row_end];

      // Find the tallest wallpaper
      if (wp->h > max_h)
        max_h = wp->h;

      row_end++;
    }

    // Position wallpapers in the row
    x = config.layout.margin;

    for (i32 i = row_start; i < row_end; i++) {
      Wallpaper* wp = &app->wallpapers.items[i];

      // Center vertically in the row
      wp->x = x;
      wp->y = y + (max_h - wp->h) / 2.0f;

      x += config.layout.thumbnail_width + gap;
    }

    // Move to the next row
    y += max_h + config.layout.inner_gap;
    row_start = row_end;
  }
}

void app_clamp_camera(App* app) {
  if (app->wallpapers.len == 0) {
    app->camera.pos.y = 0;
    return;
  }

  f32 content_bottom = 0.0f;

  for (i32 i = 0; i < app->wallpapers.len; i++) {
    Wallpaper* wp = &app->wallpapers.items[i];

    f32 bottom = wp->y + wp->h;

    if (bottom > content_bottom)
      content_bottom = bottom;
  }

  f32 view_height = (f32)app->window.height;

  // Keep a margin below the last thumbnail
  f32 max_y = content_bottom + config.layout.margin - view_height;

  if (max_y < 0.0f)
    max_y = 0.0f;

  if (app->camera.pos.y < 0.0f)
    app->camera.pos.y = 0.0f;

  if (app->camera.pos.y > max_y)
    app->camera.pos.y = max_y;
}

void app_select_wallpaper(App* app, Wallpaper* wp) {
  if (!wallpaper_apply(wp))
    return;

  free(config.selected_wallpaper);
  config.selected_wallpaper = strdup(wp->path);

  config_save(&config);

  log_info("Selected wallpaper: %s\n", wp->path);
}

// :main
int main(int argc, char** argv) {
  ctx_begin();

  // Load the config before creating the app
  if (!config_new(&config)) {
    log_error("Failed to load config\n");
    return 1;
  }

  // Restore the previously selected wallpaper
  if (argc > 1 && strcmp(argv[1], "--restore") == 0) {
    if (!config.selected_wallpaper) {
      log_error("No selected wallpaper in config\n");
      config_delete(&config);
      return 1;
    }

    Wallpaper wp = {
      .path = config.selected_wallpaper,
    };

    b8 success = wallpaper_apply(&wp);

    config_delete(&config);
    return success ? 0 : 1;
  }

  App app = {0};

  app_new(&app);
  app_spawn_workers(&app, 5);

  while (!app.window.should_close) {
    // Upload one image per frame
    Wallpaper* loaded = NULL;

    pthread_mutex_lock(&app.wallpapers_mutex);

    for (i32 i = 0; i < app.wallpapers.len; i++) {
      if (app.wallpapers.items[i].state == W_LOADED) {
        loaded = &app.wallpapers.items[i];
        break;
      }
    }

    pthread_mutex_unlock(&app.wallpapers_mutex);

    if (loaded) {
      if (!atlases_add_wallpaper(&app.atlases, loaded))
        loaded->state = W_FAILED;
      else
        log_info("TEXTURED: %s\n", basename(loaded->path));
    }

    // Event loop
    Event event = {0};

    while (event_poll(app.window, &event)) {
      switch (event.type) {
        case WINDOW_RESIZE:
          // Keep the camera boundary in sync
          app.camera.boundary.right = event.e.window_size.x;
          app.camera.boundary.bottom = event.e.window_size.y;
          break;

        case MOUSE_WHEEL_DOWN:
          ocamera_change_pos(&app.camera, (v2) { 0, config.general.scroll_speed });
          break;

        case MOUSE_WHEEL_UP:
          ocamera_change_pos(&app.camera, (v2) { 0, -config.general.scroll_speed });
          break;

        case MOUSE_BUTTON_DOWN: {
          if (event.e.button != MOUSE_BUTTON_LEFT)
            break;

          // Convert mouse position to world position
          v2 mp = event_mouse_pos(app.window);
          mp.x += app.camera.pos.x;
          mp.y += app.camera.pos.y;

          for (i32 i = 0; i < app.wallpapers.len; i++) {
            Wallpaper* wp = &app.wallpapers.items[i];
            Rect r = { wp->x, wp->y, wp->w, wp->h };

            if (point_in_rect(mp, r)) {
              app_select_wallpaper(&app, wp);
              break;
            }
          }
        } break;

        default:
          break;
      }
    }

    app_create_layout(&app);
    app_clamp_camera(&app);

    // Rendering
    imr_clear(hex_to_rgba(config.color.background));
    glViewport(0, 0, app.window.width, app.window.height);

    m4 mvp = ocamera_calc_mvp(&app.camera);
    imr_update_mvp(&app.imr, mvp);

    imr_begin(&app.imr);

    v2 mouse = event_mouse_pos(app.window);
    mouse.x += app.camera.pos.x;
    mouse.y += app.camera.pos.y;

    if (app.wallpapers.len) {
      for (i32 i = 0; i < app.wallpapers.len; i++) {
        Wallpaper* wp = &app.wallpapers.items[i];

        pthread_mutex_lock(&app.wallpapers_mutex);
        WallpaperState state = wp->state;
        pthread_mutex_unlock(&app.wallpapers_mutex);

        switch (state) {
          case W_PENDING:
            imr_push_quad_rounded(
              &app.imr,
              (v3) { wp->x, wp->y, 0 },
              (v2) { wp->w, wp->h },
              m4_identity(),
              (v4) { 1, 1, 1, 1 },
              config.layout.thumbnail_radius
            );
            break;

          case W_LOADED:
            // Waiting for the main thread to upload it
            imr_push_quad_rounded(
              &app.imr,
              (v3) { wp->x, wp->y, 0 },
              (v2) { wp->w, wp->h },
              m4_identity(),
              (v4) { 1, 1, 1, 1 },
              config.layout.thumbnail_radius
            );
            break;

          case W_READY: {
            Rect r = { wp->x, wp->y, wp->w, wp->h };

            if (point_in_rect(mouse, r)) {
              imr_push_quad_rounded(
                &app.imr,
                (v3) { wp->x - config.layout.border_thickness, wp->y - config.layout.border_thickness, 0 },
                (v2) { wp->w + config.layout.border_thickness * 2, wp->h + config.layout.border_thickness * 2 },
                m4_identity(),
                hex_to_rgba(config.color.border_select),
                config.layout.thumbnail_radius
              );
            } else {
              imr_push_quad_rounded(
                &app.imr,
                (v3) { wp->x - config.layout.border_thickness, wp->y - config.layout.border_thickness, 0 },
                (v2) { wp->w + config.layout.border_thickness * 2, wp->h + config.layout.border_thickness * 2 },
                m4_identity(),
                hex_to_rgba(config.color.border_unselect),
                config.layout.thumbnail_radius
              );
            }

            Atlas* at = &app.atlases.items[wp->atlas_idx];
            texture_bind(at->fbo.color_texture);

            imr_push_quad_tex_rounded(
              &app.imr,
              (v3) { wp->x, wp->y, 0 },
              (v2) { wp->w, wp->h },
              wp->uv,
              at->fbo.color_texture.id,
              m4_identity(),
              (v4) { 1, 1, 1, 1 },
              config.layout.thumbnail_radius
            );
          } break;

          case W_FAILED:
            imr_push_quad_rounded(
              &app.imr,
              (v3) { wp->x, wp->y, 0 },
              (v2) { wp->w, wp->h },
              m4_identity(),
              (v4) { 1, 0, 0, 1 },
              config.layout.thumbnail_radius
            );
            break;
        }
      }
    } else {
      const char* text = "Mural";
      v2 size = font_calc_size(&app.font, text);

      font_render(
        &app.imr,
        &app.font,
        text,
        (v3) { app.window.width / 2.0f - size.x / 2.0f, app.window.height / 2.0f + size.y / 2.0f },
        hex_to_rgba(config.color.font_color)
      );
    }

    imr_end(&app.imr);

    window_update(&app.window);
  }

  app_delete(&app);

  return 0;
}
