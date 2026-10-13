#ifndef __BASE_H__
#define __BASE_H__

// :includes
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "external/glew/include/GL/glew.h"
#include "external/glfw/include/GLFW/glfw3.h"
#include "external/stb/stb_image.h"
#include "external/stb/stb_truetype.h"

#if defined(_WIN32)
  #include <windows.h>
#else
  #include <unistd.h>
#endif

// :log
#if defined(__clang__) || defined(__gcc__)
  #define STATIC_ASSERT _Static_assert
#else
  #define STATIC_ASSERT static_assert
#endif

#define panic(x, ...) \
  do {\
    if (!(x)) {\
      fprintf(stderr, "\033[31m[ASSERTION]: %s:%d:\033[0m ", __FILE__, __LINE__);\
      fprintf(stderr, " " __VA_ARGS__);\
      fprintf(stderr, "\n");\
      exit(1);\
    }\
  } while(0)

#define LOG_NORMAL 0
#define LOG_ERROR  91
#define LOG_SUCESS 92
#define LOG_WARN   93
#define LOG_INFO   95

#define log_typed(type, ...)         \
  ({                                 \
    printf("\033[%dm", type);        \
    printf(__VA_ARGS__);             \
    printf("\033[%dm", LOG_NORMAL);  \
  })

#define log(...)                          \
  ({                                      \
    log_typed(LOG_NORMAL, "[LOG]:    ");  \
    log_typed(LOG_NORMAL, __VA_ARGS__);   \
  })

#define log_sucess(...)                   \
  ({                                      \
    log_typed(LOG_SUCESS, "[SUCESS]: ");  \
    log_typed(LOG_SUCESS, __VA_ARGS__);   \
  })

#define log_warn(...)                     \
  ({                                      \
    log_typed(LOG_WARN,   "[WARN]:   ");  \
    log_typed(LOG_WARN, __VA_ARGS__);     \
  })

#define log_error(...)                    \
  ({                                      \
    log_typed(LOG_ERROR,  "[ERROR]:  ");  \
    log_typed(LOG_ERROR, __VA_ARGS__);    \
  })

#define log_info(...)                     \
  ({                                      \
    log_typed(LOG_INFO,   "[INFO]:   ");  \
    log_typed(LOG_INFO, __VA_ARGS__);     \
  })

#define GLCall(x)\
  (\
    clear_gl_error(), \
    x\
  );\
  panic(gl_error_log(#x, __FILE__, __LINE__), "Opengl failed.\n");\

static void clear_gl_error() {
  while(glGetError());
}

static int gl_error_log(const char* function, const char* file, int line) {
  GLenum error;
  while ((error = glGetError())) {
    log_error("[Error code]: %d\n", error);
    log_error("[Error message]: %s\n", gluErrorString(error));
    log_error("[Opengl error]: %s %s: %d\n", function ,file, line);
    return 0;
  }
  return 1;
}

// :types
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;
typedef float f32;
typedef double f64;
typedef bool b8;

STATIC_ASSERT(sizeof(u8)  == 1, "Expected  u8 to be 1 bytes.");
STATIC_ASSERT(sizeof(u16) == 2, "Expected u16 to be 2 bytes.");
STATIC_ASSERT(sizeof(u32) == 4, "Expected u32 to be 4 bytes.");
STATIC_ASSERT(sizeof(u64) == 8, "Expected u64 to be 8 bytes.");
STATIC_ASSERT(sizeof(i8)  == 1, "Expected  i8 to be 1 bytes.");
STATIC_ASSERT(sizeof(i16) == 2, "Expected i16 to be 2 bytes.");
STATIC_ASSERT(sizeof(i32) == 4, "Expected i32 to be 4 bytes.");
STATIC_ASSERT(sizeof(i64) == 8, "Expected i64 to be 8 bytes.");
STATIC_ASSERT(sizeof(f32) == 4, "Expected f32 to be 4 bytes.");
STATIC_ASSERT(sizeof(f64) == 8, "Expected f64 to be 8 bytes.");
STATIC_ASSERT(sizeof(b8)  == 1, "Expected  b8 to be 4 bytes.");


//===========================
// DEFINITIONS BEGINS HERE!
//===========================

// :da def
#define DA_START_CAP 50

// A Dynamic Array is expected to have the following members
// - items    => Pointer to the desired item
// - len      => Length of the dynamic array
// - capacity => Max capacity of the dyanmic array before reallocation
//
// NOTE(slok): Always set the dynamic structure to {0} first
// i.e. Frames frames = {0}; <- Frames is a dynamic array structure


// @params
// da: [Pointer] to the dynamic array
// value: Any value
#define da_append(da, value) do {                                                   \
  if ((da)->len + 1 >= (da)->capacity) {                                            \
    if ((da)->capacity <= 0) (da)->capacity = DA_START_CAP;                         \
    (da)->capacity *= 2;                                                            \
    (da)->items = mem_realloc((da)->items, (da)->capacity * sizeof(*(da)->items));  \
  }                                                                                 \
  (da)->items[(da)->len++] = (value);                                               \
} while (0);                                                                        \

// @params
// da: [Pointer] to the dynamic array
// idx: index of the value to be removed
#define da_remove_unordered(da, idx) do {                                \
  panic((idx) >= 0 && (idx) < (da)->len, "Index out of range: %d", idx); \
  (da)->items[(idx)] = (da)->items[(da)->len - 1];                       \
  (da)->len--;                                                           \
} while (0);                                                             \

// @params
// da: [Pointer] to the dynamic array
// value: value to compare
// idx: [Out] index of the value is saved
#define da_get_idx(da, value, idx) do { \
  for (i32 i = 0; i < (da)->len; i++) { \
    if ((value) == (da)->items[i]) {    \
      idx = i;                          \
      break;                            \
    }                                   \
  }                                     \
} while (0);                            \

// :dir def
typedef enum {
  LEFT,
  RIGHT,
  UP,
  DOWN,
  DIR_COUNT,
} Dir;

// :utils def
typedef struct {
  char* data;
  size_t size;
} FileData;

b8 read_file(FileData* fd, const char* path);   // Returned buffer is heap allocated. Make sure to free it after use.
u64 hash_data(char* data);
u64 zigzag(i32 x);

// :alloc def
#define TRACE_ALLOCATOR_MEMORY_CAP 1024
#define TRACE_ALLOCATOR_GROW_RATE 0.5

typedef struct {
  void* ptr;
  size_t size;
  const char* file;
  i32 line;
} Traceable_Memory_Block;
#define print_traceable_memory_block(block) ({\
  printf(\
    "%p at %s:%d of %zu bytes\n",\
    (block)->ptr,  (block)->file,\
    (block)->line, (block)->size\
  );\
});

typedef struct {
  Traceable_Memory_Block* blocks;
  u32 blocks_cnt;
  u32 blocks_cap;
} Trace_Allocator;

Trace_Allocator* trace_allocator_new();
void trace_allocator_delete(Trace_Allocator* allocator);
#define trace_allocator_alloc(allocator, size) __trace_allocator_alloc(allocator, size, __FILE__, __LINE__)
void* __trace_allocator_alloc(Trace_Allocator* allocator, size_t size, const char* file, i32 line);
#define trace_allocator_realloc(allocator, ptr, new_cap) __trace_allocator_realloc(allocator, ptr, new_cap, __FILE__, __LINE__)
void* __trace_allocator_realloc(Trace_Allocator* allocator, void* ptr, size_t new_cap, const char* file, i32 line);
void trace_allocator_free(Trace_Allocator* allocator, void* ptr);
void trace_allocator_alert(Trace_Allocator* allocator);

#define mem_alloc(size) __mem_alloc(size, __FILE__, __LINE__)
void* __mem_alloc(i64 size, const char* file, i32 line);
#define mem_realloc(ptr, new_cap) __mem_realloc(ptr, new_cap, __FILE__, __LINE__);
void* __mem_realloc(void* ptr, size_t new_cap, const char* file, i32 line);
void mem_free(void* ptr);

// :vec def
typedef struct { f32 x, y;    } v2;
typedef struct { f32 x, y, z; } v3;
typedef struct {
  union { f32 x, r; };
  union { f32 y, g; };
  union { f32 z, b; };
  union { f32 w, a; };
} v4;

void print_v2(v2 v);
void print_v3(v3 v);
void print_v4(v4 v);

b8 v2_eq(v2 a, v2 b);
b8 v3_eq(v3 a, v3 b);
b8 v4_eq(v4 a, v4 b);

v2 v2_add(v2 a, v2 b);
v3 v3_add(v3 a, v3 b);
v4 v4_add(v4 a, v4 b);

v2 v2_sub(v2 a, v2 b);
v3 v3_sub(v3 a, v3 b);
v4 v4_sub(v4 a, v4 b);

v2 v2_mul(v2 a, v2 b);
v3 v3_mul(v3 a, v3 b);
v4 v4_mul(v4 a, v4 b);

v2 v2_mul_scalar(v2 v, f32 scalar);
v3 v3_mul_scalar(v3 v, f32 scalar);
v4 v4_mul_scalar(v4 v, f32 scalar);

v2 v2_div(v2 a, v2 b);
v3 v3_div(v3 a, v3 b);
v4 v4_div(v4 a, v4 b);

f32 v2_mag(v2 v);
f32 v3_mag(v3 v);
f32 v4_mag(v4 v);

v2 v2_dir(v2 v);
v3 v3_dir(v3 v);

v2 v2_normalize(v2 v);
v3 v3_normalize(v3 v);
v4 v4_normalize(v4 v);

v2 v2_cross(v2 a, v2 b);
v3 v3_cross(v3 a, v3 b);
v4 v4_cross(v4 a, v4 b);

v2 v2_round(v2 a);

u64 v2_hash(v2 p);

// :mat def
typedef struct {
  f32 m[4][4];
} m4;

void print_m4(m4 m);
void m4_clear(m4* m);
m4 m4_mul(m4 m1, m4 m2);
v3 m4_mul_v3(m4 m, v3 v);
m4 m4_identity();
m4 m4_zero();
m4 m4_inverse(m4 in);
m4 m4_translate(m4 m, v3 v);
m4 m4_transpose(m4 m);
m4 m4_scale(f32 s);
m4 ortho_projection(f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far);
m4 persp_projection(f32 aspect_ratio, f32 fov, f32 near, f32 far);
m4 rotate_x(f32 theta);
m4 rotate_y(f32 theta);
m4 rotate_z(f32 theta);

// :rect def
typedef struct {
  f32 x, y, w, h;
} Rect;

#define print_rect(r) print_v4(*((v4*)&r))
b8 rect_intersect_inclusive(Rect r1, Rect r2);   // Includes comparision with <=
b8 rect_intersect(Rect r1, Rect r2);             // Includes comparision with <
b8 point_in_rect(v2 p, Rect r);
Rect rect_with_offset(v2 pos, Rect offset_rect);

// :math def
#define PI 3.14159
#define to_radians(x) ((x) * PI / 180)
#define to_degrees(x) ((x) * 180 / PI)
#define rand_init(seed) srand(seed)
#define rand_range(l, u) rand() % (u - l + 1) + l
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

static b8 f32_eq(f32 a, f32 b) {
  return fabs(a - b) < 0.01f;
}

static v2 pixel_to_gl_coords(v2 pos, u32 WIN_WIDTH, u32 WIN_HEIGHT) {
  return (v2) {
    (2 * pos.x) / WIN_WIDTH - 1,
    1 - (2 * pos.y) / WIN_HEIGHT
  };
}

static v2 dp_to_dgl_coords(v2 dp, u32 WIN_WIDTH, u32 WIN_HEIGHT) {
  return (v2) {
    (2 * dp.x) / WIN_WIDTH,
    - (2 * dp.y) / WIN_HEIGHT
  };
}

static v2 gl_to_pixel_coords(v2 pos, u32 WIN_WIDTH, u32 WIN_HEIGHT) {
  return (v2) {
    (pos.x + 1) * WIN_WIDTH / 2,
    (1 - pos.y) * WIN_HEIGHT / 2
  };
}

static v2 dgl_to_dp_coords(v2 dp, u32 WIN_WIDTH, u32 WIN_HEIGHT) {
  return (v2) {
    (dp.x) * WIN_WIDTH / 2,
    (-dp.y) * WIN_HEIGHT / 2
  };
}

// :window def
typedef struct {
  b8 fullscreen;
  b8 vsync;
} WindowSettings;

typedef struct {
  GLFWwindow* glfw_window;
  u32 width, height;
  b8 should_close;
} Window;

b8 window_new(Window* window, const char* title, u32 width, u32 height, WindowSettings settings);
void window_delete(Window window);
void window_update(Window* window);

// :frame controller def
// NOTE: The times are in seconds
typedef struct {
  f64 start_time;
  f64 start_tick;
  f64 unit_frame;
  f64 dt;
  i32 frame;
  i32 fps;
} FrameController;

FrameController frame_controller_new(i32 fps);
void frame_controller_start(FrameController* fc);
void frame_controller_end(FrameController* fc);

// :event def
typedef enum {
  KEYDOWN,
  KEYUP,

  MOUSE_BUTTON_DOWN,
  MOUSE_BUTTON_UP,
  MOUSE_MOTION,
} EventType;

typedef enum {
  MOUSE_BUTTON_LEFT,
  MOUSE_BUTTON_RIGHT
} MouseButton;

typedef struct {
  EventType type;

  union {
    u32 key;
    MouseButton button;
    v2 mouse_pos;
  } e;
} Event;

b8 event_poll(Window window, Event* event);
v2 event_mouse_pos(Window window);
void event_set_mouse_pos(Window window, v2 pos);

// :event queue def
#define MAX_EVENTS 1024
typedef struct {
  Event events[MAX_EVENTS];
  i32 front, back;
} EventQueue;

void event_enqueue(EventQueue* queue, Event e);
Event event_dequeue(EventQueue* queue);

// :ocamera def
typedef struct {
  f32 left, right, top, bottom, near, far;
} OCamera_Boundary;

typedef struct {
  v2 pos;   // In GL Coords
  v2 render_pos;      // snapped, used to render the game world
  v2 pixel_offset;     // the rounding remainder, used only at display time

  f32 zoom;
  m4 mvp;
  OCamera_Boundary boundary;

  // For camera follow
  b8 active_x;
  b8 active_y;
} OCamera;

OCamera ocamera_new(v2 pos, f32 zoom, OCamera_Boundary boundary);
void ocamera_change_zoom(OCamera* cam, f32 dz);
void ocamera_change_pos(OCamera* cam, v2 dp);
void ocamera_follow(OCamera* cam, Rect to_follow_rect, v2 offset, f32 smooth_speed, f32 dt);
m4 ocamera_calc_mvp(OCamera* cam);

// :pcamera def
typedef struct {
  f32 aspect_ratio, fov, near, far;
} PCamera_Info;

typedef struct {
  v3 dir, up, right;
  v3 pos;
  m4 look_at, mvp;
  f32 pitch, yaw;

  // Mouse
  v2 mp;
  f32 sensitivity;
  b8 first;
  b8 mouse_enable;

  PCamera_Info info;
} PCamera;

PCamera pcamera_new(v3 pos, v3 dir, f32 sensitivity, PCamera_Info info);
void pcamera_change_pos(PCamera* cam, v3 dp);
void pcamera_handle_mouse(PCamera* cam, Window window);
m4 pcamera_calc_mvp(PCamera* cam);

// :texture def
typedef struct {
  u32 id, width, height;
} Texture;

// TODO: Implement control over texture filters
// Filters are hard coded for now
b8 texture_from_file(Texture* texture, const char* filepath, b8 flip);
b8 texture_from_data(Texture* texture, u32 width, u32 height, u8* data, i32 channel);
void texture_clear(Texture texture);
void texture_bind(Texture texture);
void texture_unbind(Texture texture);
void texture_delete(Texture texture);

// :shader def
typedef u32 Shader;
typedef enum {
  VERTEX_SHADER = GL_VERTEX_SHADER,
  FRAGMENT_SHADER = GL_FRAGMENT_SHADER
} ShaderType;

b8 shader_new(Shader* shader, const char* v_src, const char* f_src);
b8 shader_new_from_file(Shader* shader, const char* v_path, const char* f_path);
void shader_delete(Shader id);
b8 shader_compile(u32* id, ShaderType type, const char* shader_src);

// :fbo def
typedef struct {
  u32 id;
  u32 width, height;
  Texture color_texture;
} FBO;

b8 fbo_new(FBO* fbo, u32 width, u32 height);
void fbo_delete(FBO* fbo);
void fbo_bind(FBO* fbo);
void fbo_unbind();

// :imr def
typedef struct {
  v3 pos;
  v4 color;
  v2 tex_coord;
  f32 tex_id;
  v4 overlay_color;
  v2 local;           // Local coordinate (0,0), (0,1), (1,0), (1,1)
  v2 size;
  f32 radius;
} Vertex;

typedef struct {
  v3 a, b, c;
} Triangle;

#define TEXTURE_SAMPLE_AMT 32
#define VERTEX_SIZE   19
#define MAX_VERT_CNT  10000
#define MAX_BUFF_CAP  MAX_VERT_CNT  * VERTEX_SIZE
#define MAX_VBO_SIZE  MAX_BUFF_CAP  * sizeof(f32)

STATIC_ASSERT(VERTEX_SIZE == sizeof(Vertex) / sizeof(f32), "Size of vertex missmatched");

typedef struct {
  u32 vao, vbo;
  Shader shader;
  Shader def_shader;
  f32 buffer[MAX_BUFF_CAP];
  u32 buff_idx;
  Texture white;
} IMR;

b8 imr_new(IMR* imr);
void imr_delete(IMR* imr);
void imr_clear(v4 color);
void imr_begin(IMR* imr);
void imr_end(IMR* imr);
void imr_switch_shader(IMR* imr, Shader shader);
void imr_reapply_samplers(IMR* imr);
void imr_switch_shader_to_default(IMR* imr);
void imr_update_mvp(IMR* imr, m4 mvp);
void imr_push_vertex(IMR* imr, Vertex v);
void imr_push_quad(IMR* imr, v3 pos, v2 size, m4 rot, v4 color);
void imr_push_quad_rounded(IMR* imr, v3 pos, v2 size, m4 rot, v4 color, f32 radius);
void imr_push_quad_overlay(IMR* imr, v3 pos, v2 size, m4 rot, v4 color, v4 overlay_color);
void imr_push_quad_overlay_rounded(IMR* imr, v3 pos, v2 size, m4 rot, v4 color, v4 overlay_color, f32 radius);
void imr_push_quad_tex(IMR* imr, v3 pos, v2 size, Rect tex_rect, f32 tex_id, m4 rot, v4 color);
void imr_push_quad_tex_rounded(IMR* imr, v3 pos, v2 size, Rect tex_rect, f32 tex_id, m4 rot, v4 color, f32 radius);
void imr_push_quad_tex_overlay(IMR* imr, v3 pos, v2 size, Rect tex_rect, f32 tex_id, m4 rot, v4 color, v4 overlay_color);
void imr_push_quad_tex_overlay_rounded(IMR* imr, v3 pos, v2 size, Rect tex_rect, f32 tex_id, m4 rot, v4 color, v4 overlay_color, f32 radius);
void imr_push_triangle(IMR* imr, v3 p1, v3 p2, v3 p3, m4 rot, v4 color);
void imr_push_triangle_tex(IMR* imr, v3 p1, v3 p2, v3 p3, Triangle tex_coord, f32 tex_id, m4 rot, v4 color);

// :context def
typedef struct {
  Trace_Allocator* t_alloc;
  EventQueue e_queue;
  void* inner;
} Context;

void ctx_end();

// :frame def
typedef Rect Frame;

typedef struct {
  Frame* items;
  i32 len;
  i32 capacity;
} Frames;

// :spritesheet def
typedef struct {
  Texture texture;
  i32 rows, cols;
} SpriteSheet;

b8 spritesheet_new(SpriteSheet* ss, const char* path, i32 tile_width, i32 tile_height);
void spritesheet_delete(SpriteSheet* ss);
void spritesheet_bind(SpriteSheet* ss);
void spritesheet_unbind(SpriteSheet* ss);
Frames spritesheet_extract_frames(SpriteSheet* ss, i32 at_row, i32 column_start, i32 column_end); // NOTE(slok): Frames.items are heap allocated, make sure to free them
Frame spritesheet_extract_frame(SpriteSheet* ss, i32 row, i32 col);
Frame spritesheet_extract_region(SpriteSheet* ss, i32 row, i32 col, i32 row_span, i32 col_span);  // Extracts a region of the spritesheet like 2x2 sprite

// :animator def
typedef struct {
  i32 id;
  f32 duration;     // NOTE(slok): Duration is in ms
  i32 curr_frame;
  b8 loop;
  Frames frames;
} Animation;

typedef struct {
  Animation* items;
  i32 len;
  i32 capacity;
} AnimationEntries;

typedef struct {
  i32 curr_state;
  f64 start_time;

  AnimationEntries entries;
} Animator;

Animator animator_new();
void animator_delete(Animator* animator);
void animator_add_animation(Animator* animator, Animation animation);
Animation* animator_get_animation(Animator* animator, i32 state);
void animator_switch(Animator* animator, i32 state);
Frame animator_get_frame(Animator* animator);

// :font def
#define FONT_ATLAS_W 512
#define FONT_ATLAS_H 512

typedef struct {
  Rect uv;

  v2 size;
  v2 offset;

  f32 advance;
} Glyph;

typedef struct {
  Texture atlas;
  Glyph glyphs[128];
  f32 size;
} Font;

b8 font_new(Font* font, const char* path, f32 size);
void font_render(IMR* imr, Font* font, const char* text, v3 pos, v4 color);
v2 font_calc_size(Font* font, const char* text);
void font_delete(Font* font);


// :def end


//=============================
// IMPLEMENTATION BEGINS HERE!
//=============================


// :impl
#ifdef BASE_IMPLEMENTATION

// :context impl
Context* ctx = NULL;

void ctx_begin() {
  if (ctx) return;

  ctx = (Context*) malloc(sizeof(Context));
  ctx->t_alloc = trace_allocator_new();
  ctx->e_queue = (EventQueue) {
    .front = 0,
    .back  = 0,
  };
  ctx->inner = NULL;
}

void ctx_end() {
  if (ctx->t_alloc) trace_allocator_delete(ctx->t_alloc);
  free(ctx);
}

// :utils impl
b8 read_file(FileData* fd, const char* path) {
  panic(fd, "Paramter fd cannot be NULL");

  FILE* file = fopen(path, "rb");
  if (file == NULL) {
    log_error("Failed to open file: %s", path);
    return false;
  }

  // Get file size
  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  rewind(file);

  // Allocate memory (+1 for '\0')
  char* buffer = malloc(size + 1);
  if (!buffer) {
    fclose(file);
    panic(false, "Failed to allocated buffer");
  }

  // Read file
  fread(buffer, 1, size, file);
  buffer[size] = '\0';

  fclose(file);

  // Storing the result
  fd->data = buffer;
  fd->size = size;
  return true;
}

u64 hash_data(char* data) {
  u64 hash = 5381;
  i32 c;
  while (c = *data++) {
    hash = ((hash << 5) + hash) + c; // hash * 33 + c
  }
  return hash;
}

u64 zigzag(i32 x) {
  return ((u64)x << 1) ^ ((u64)x >> 31);
}

// :alloc impl
Trace_Allocator* trace_allocator_new() {
  Trace_Allocator* allocator = (Trace_Allocator*) calloc(1, sizeof(Trace_Allocator));

  allocator->blocks = (Traceable_Memory_Block*) calloc(
    TRACE_ALLOCATOR_MEMORY_CAP,
    sizeof(Traceable_Memory_Block)
  );
  allocator->blocks_cnt = 0;
  allocator->blocks_cap = TRACE_ALLOCATOR_MEMORY_CAP;

  return allocator;
}

void trace_allocator_delete(Trace_Allocator* allocator) {
  trace_allocator_alert(allocator);
  free(allocator->blocks);
  free(allocator);
}

void handle_blocks_overflow(Trace_Allocator* allocator) {
  if (allocator->blocks_cnt < allocator->blocks_cap) return;

  Traceable_Memory_Block* tmp_blocks = (Traceable_Memory_Block*) calloc(
    TRACE_ALLOCATOR_MEMORY_CAP,
    sizeof(Traceable_Memory_Block)
  );
  memcpy(
    tmp_blocks, allocator->blocks,
    sizeof(Traceable_Memory_Block) * allocator->blocks_cnt
  );

  free(allocator->blocks);

  allocator->blocks_cap += TRACE_ALLOCATOR_GROW_RATE * TRACE_ALLOCATOR_MEMORY_CAP;
  allocator->blocks = (Traceable_Memory_Block*) calloc(
    allocator->blocks_cap,
    sizeof(Traceable_Memory_Block)
  );
  memcpy(
    allocator->blocks, tmp_blocks,
    sizeof(Traceable_Memory_Block) * allocator->blocks_cnt
  );

  free(tmp_blocks);
}

b8 remove_block(Trace_Allocator* allocator, void* ptr) {
  if (!ptr) return false;

  for (u32 i = 0; allocator->blocks_cnt; i++) {
    if (allocator->blocks[i].ptr == ptr) {
      memmove(
        &allocator->blocks[i], &allocator->blocks[i+1],
        (allocator->blocks_cnt - i - 1) * sizeof(Traceable_Memory_Block)
      );
      allocator->blocks_cnt--;
      return true;
    }
  }
  return false;
}

void* __trace_allocator_alloc(Trace_Allocator* allocator, size_t size, const char* file, i32 line) {
  handle_blocks_overflow(allocator);

  void* ptr = (void*) malloc(size);
  Traceable_Memory_Block mem = {
    ptr,
    size,
    file,
    line
  };
  memset(ptr, 0, size);

  allocator->blocks[allocator->blocks_cnt++] = mem;
  return ptr;
}

void* __trace_allocator_realloc(Trace_Allocator* allocator, void* ptr, size_t new_cap, const char* file, i32 line) {
  // Do a normal alloc if the mem block doesnt exists
  if (!remove_block(allocator, ptr))
    return __trace_allocator_alloc(allocator, new_cap, file, line);

  ptr = (void*) realloc(ptr, new_cap);
  Traceable_Memory_Block mem = {
    ptr,
    new_cap,
    file,
    line
  };

  allocator->blocks[allocator->blocks_cnt++] = mem;
  return ptr;
}

void trace_allocator_free(Trace_Allocator* allocator, void* ptr) {
  if (remove_block(allocator, ptr))
    free(ptr);
}

void trace_allocator_alert(Trace_Allocator* allocator) {
  if (!allocator->blocks_cnt) return;

  printf("\n---------Unfreed memories---------\n");
  size_t total = 0;
  for (u32 i = 0; i < allocator->blocks_cnt; i++) {
    print_traceable_memory_block(&allocator->blocks[i]);
    total += allocator->blocks[i].size;
  }
  printf("\nTotal unfreed memories count = %d\n", allocator->blocks_cnt);
  printf("Total unfreed memories = %d bytes\n", total);
  printf("---------Unfreed memories---------\n\n");
}

void* __mem_alloc(i64 size, const char* file, i32 line) {
  panic(ctx, "Context hasnt been initialized");
  panic(ctx->t_alloc, "Allocator hasnt been initialized");
  return __trace_allocator_alloc(ctx->t_alloc, size, file, line);
}

void* __mem_realloc(void* ptr, size_t new_cap, const char* file, i32 line){
  panic(ctx, "Context hasnt been initialized");
  panic(ctx->t_alloc, "Allocator hasnt been initialized");
  return __trace_allocator_realloc(ctx->t_alloc, ptr, new_cap, file, line);
}

void mem_free(void* ptr) {
  panic(ctx, "Context hasnt been initialized");
  panic(ctx->t_alloc, "Allocator hasnt been initialized");
  trace_allocator_free(ctx->t_alloc, ptr);
}

// :vec impl
void print_v2(v2 v) {
  printf("(%f, %f)\n", v.x, v.y);
}

void print_v3(v3 v) {
  printf("(%f, %f, %f)\n", v.x, v.y, v.z);
}

void print_v4(v4 v) {
  printf("(%f, %f, %f, %f)\n", v.x, v.y, v.z, v.w);
}

b8 v2_eq(v2 a, v2 b) {
  return f32_eq(a.x, b.x) &&
         f32_eq(a.y, b.y);
}

b8 v3_eq(v3 a, v3 b) {
  return f32_eq(a.x, b.x) &&
         f32_eq(a.y, b.y) &&
         f32_eq(a.z, b.z);
}

b8 v4_eq(v4 a, v4 b) {
  return f32_eq(a.x, b.x) &&
         f32_eq(a.y, b.y) &&
         f32_eq(a.z, b.z) &&
         f32_eq(a.w, b.w);
}

v2 v2_add(v2 a, v2 b) {
  return (v2) {
    .x = a.x + b.x,
    .y = a.y + b.y
  };
}

v3 v3_add(v3 a, v3 b) {
  return (v3) {
    .x = a.x + b.x,
    .y = a.y + b.y,
    .z = a.z + b.z
  };
}

v4 v4_add(v4 a, v4 b) {
  return (v4) {
    .x = a.x + b.x,
    .y = a.y + b.y,
    .z = a.z + b.z,
    .w = a.w + b.w
  };
}

v2 v2_sub(v2 a, v2 b) {
  return (v2) {
    .x = a.x - b.x,
    .y = a.y - b.y
  };
}

v3 v3_sub(v3 a, v3 b) {
  return (v3) {
    .x = a.x - b.x,
    .y = a.y - b.y,
    .z = a.z - b.z
  };
}

v4 v4_sub(v4 a, v4 b) {
  return (v4) {
    .x = a.x - b.x,
    .y = a.y - b.y,
    .z = a.z - b.z,
    .w = a.w - b.w
  };
}

v2 v2_mul(v2 a, v2 b) {
  return (v2) {
    .x = a.x * b.x,
    .y = a.y * b.y
  };
}

v3 v3_mul(v3 a, v3 b) {
  return (v3) {
    .x = a.x * b.x,
    .y = a.y * b.y,
    .z = a.z * b.z
  };
}

v4 v4_mul(v4 a, v4 b) {
  return (v4) {
    .x = a.x * b.x,
    .y = a.y * b.y,
    .z = a.z * b.z,
    .w = a.w * b.w
  };
}

v2 v2_mul_scalar(v2 v, f32 scalar) {
  return (v2) {
    .x = v.x * scalar,
    .y = v.y * scalar
  };
}

v3 v3_mul_scalar(v3 v, f32 scalar) {
  return (v3) {
    .x = v.x * scalar,
    .y = v.y * scalar,
    .z = v.z * scalar
  };
}

v4 v4_mul_scalar(v4 v, f32 scalar) {
  return (v4) {
    .x = v.x * scalar,
    .y = v.y * scalar,
    .z = v.z * scalar,
    .w = v.w * scalar
  };
}

v2 v2_div(v2 a, v2 b) {
  return (v2) {
    .x = a.x / b.x,
    .y = a.y / b.y
  };
}

v3 v3_div(v3 a, v3 b) {
  return (v3) {
    .x = a.x / b.x,
    .y = a.y / b.y,
    .z = a.z / b.z
  };
}

v4 v4_div(v4 a, v4 b) {
  return (v4) {
    .x = a.x / b.x,
    .y = a.y / b.y,
    .z = a.z / b.z,
    .w = a.w / b.w
  };
}

f32 v2_mag(v2 v) {
  return sqrt(v.x * v.x + v.y * v.y);
}

f32 v3_mag(v3 v) {
  return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

f32 v4_mag(v4 v) {
  return sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

v2 v2_dir(v2 v) {
  v2 d = { 0, 0 };
  if (v.x < 0) d.x = -1;
  if (v.x > 0) d.x = 1;
  if (v.x == 0) d.x = 0;

  if (v.y < 0) d.y = -1;
  if (v.y > 0) d.y = 1;
  if (v.y == 0) d.y = 0;
  return d;
}

v3 v3_dir(v3 v) {
  v3 d = { 0, 0 };
  if (v.x < 0) d.x = -1;
  if (v.x > 0) d.x = 1;
  if (v.x == 0) d.x = 0;

  if (v.y < 0) d.y = -1;
  if (v.y > 0) d.y = 1;
  if (v.y == 0) d.y = 0;

  if (v.z < 0) d.z = -1;
  if (v.z > 0) d.z = 1;
  if (v.z == 0) d.z = 0;
  return d;
}

v2 v2_normalize(v2 v) {
  f32 r = v2_mag(v);
  return (v2) {
    .x = v.x / r,
    .y = v.y / r
  };
}

v3 v3_normalize(v3 v) {
  f32 r = v3_mag(v);
  return (v3) {
    .x = v.x / r,
    .y = v.y / r,
    .z = v.z / r
  };
}
v4 v4_normalize(v4 v) {
  f32 r = v4_mag(v);
  return (v4) {
    .x = v.x / r,
    .y = v.y / r,
    .z = v.z / r,
    .w = v.w / r
  };
}

v2 v2_cross(v2 a, v2 b) {
  panic(false, "v2_cross is not implemented yet.\n");
}

v3 v3_cross(v3 a, v3 b) {
  return (v3) {
    .x = a.y * b.z - a.z * b.y,
    .y = - (a.x * b.z - a.z * b.x),
    .z = a.x * b.y - a.y * b.x
  };
}

v4 v4_cross(v4 a, v4 b) {
  panic(false, "v3_cross is not implemented yet.\n");
}

v2 v2_round(v2 a) {
  return (v2) { roundf(a.x), roundf(a.y) };
}

u64 v2_hash(v2 p) {
  u64 x = zigzag(p.x);
  u64 y = zigzag(p.y);

  x ^= x >> 16;
  x *= 0x45d9f3b;

  y ^= y >> 16;
  y *= 0x45d9f3b;

  return x ^ (y + 0x9e3779b9 + (x << 6) + (x >> 2));
}


// :mat impl
void print_m4(m4 m) {
  for (i32 i = 0; i < 4; i++) {
    for (i32 j = 0; j < 4; j++) {
      printf("%f\t", m.m[i][j]);
    }
    printf("\n");
  }
}

void m4_clear(m4* m) {
  memset(m, 0, sizeof(m4));
}

m4 m4_mul(m4 m1, m4 m2) {
  m4 out;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      out.m[i][j] = 0;
      for (int k = 0; k < 4; k++) {
        out.m[i][j] += m1.m[i][k] * m2.m[k][j];
      }
    }
  }
  return out;
}

v3 m4_mul_v3(m4 m, v3 v) {
  v3 out;
  out.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0];
  out.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1];
  out.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2];
  f32 w = v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + m.m[3][3];
  if (w) {
    out.x /= w;
    out.y /= w;
    out.z /= w;
  }
  return out;
}

m4 m4_identity() {
  return (m4) {
    .m = {
      { 1.0f, 0.0f, 0.0f, 0.0f },
      { 0.0f, 1.0f, 0.0f, 0.0f },
      { 0.0f, 0.0f, 1.0f, 0.0f },
      { 0.0f, 0.0f, 0.0f, 1.0f }
    }
  };
}

m4 m4_zero() {
  return (m4) {
    .m = {
      { 0.0f, 0.0f, 0.0f, 0.0f },
      { 0.0f, 0.0f, 0.0f, 0.0f },
      { 0.0f, 0.0f, 0.0f, 0.0f },
      { 0.0f, 0.0f, 0.0f, 0.0f }
    }
  };
}

m4 m4_inverse(m4 in) {
  m4 out;
  m4_clear(&out);
  out.m[0][0] = in.m[0][0]; 
  out.m[0][1] = in.m[1][0]; 
  out.m[0][2] = in.m[2][0]; 
  out.m[0][3] = 0.0f;
  out.m[1][0] = in.m[0][1]; 
  out.m[1][1] = in.m[1][1];
  out.m[1][2] = in.m[2][1];
  out.m[1][3] = 0.0f;
  out.m[2][0] = in.m[0][2];
  out.m[2][1] = in.m[1][2];
  out.m[2][2] = in.m[2][2];
  out.m[2][3] = 0.0f;
  out.m[3][0] = -(in.m[3][0] * out.m[0][0] + in.m[3][1] * out.m[1][0] + in.m[3][2] * out.m[2][0]);
  out.m[3][1] = -(in.m[3][0] * out.m[0][1] + in.m[3][1] * out.m[1][1] + in.m[3][2] * out.m[2][1]);
  out.m[3][2] = -(in.m[3][0] * out.m[0][2] + in.m[3][1] * out.m[1][2] + in.m[3][2] * out.m[2][2]);
  out.m[3][3] = 1.0f;
  return out;
}

//m4 m4_translate(m4 m, v3 v) {
//  return (m4) {
//    .m = {
//      {      1.0f, m.m[0][1], m.m[0][2],  v.x },
//      { m.m[1][0],      1.0f, m.m[1][2],  v.y },
//      { m.m[2][0], m.m[2][1],      1.0f,  v.z },
//      { m.m[3][0], m.m[3][1], m.m[3][2], 1.0f },
//    }
//  };
//}
m4 m4_translate(m4 m, v3 v) {
  m4 out = m;
  out.m[3][0] += v.x;
  out.m[3][1] += v.y;
  out.m[3][2] += v.z;
  return out;
}

m4 m4_transpose(m4 m) {
  return (m4) {
    .m = {
      { m.m[0][0], m.m[1][0], m.m[2][0], m.m[3][0] },
      { m.m[0][1], m.m[1][1], m.m[2][1], m.m[3][1] },
      { m.m[0][2], m.m[1][2], m.m[2][2], m.m[3][2] },
      { m.m[0][3], m.m[1][3], m.m[2][3], m.m[3][3] }
    }
  };
}

m4 m4_scale(f32 s) {
  return (m4) {
    .m = {
      {    s, 0.0f, 0.0f, 0.0f },
      { 0.0f,    s, 0.0f, 0.0f },
      { 0.0f, 0.0f,    s, 0.0f },
      { 0.0f, 0.0f, 0.0f, 1.0f },
    }
  };
}

m4 ortho_projection(f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far) {
  f32 x_range = right - left;
  f32 y_range = top - bottom;
  f32 z_range = far - near;

  return (m4) {
    .m = {
      {               2 / x_range,                         0,                       0, 0 },
      {                         0,               2 / y_range,                       0, 0 },
      {                         0,                         0,            -2 / z_range, 0 },
      { -(right + left) / x_range, -(top + bottom) / y_range, -(far + near) / z_range, 1 }
    }
  };
}

m4 persp_projection(f32 aspect_ratio, f32 fov, f32 near, f32 far) {
  f32 t = tanf(to_radians(fov / 2));
  f32 z_range = near - far;
  f32 A = (-far - near) / z_range;
  f32 B = (2 * far * near) / z_range;

  return (m4) {
    .m = {
      { 1 / (aspect_ratio * t),     0, 0, 0 },
      {                      0, 1 / t, 0, 0 },
      {                      0,     0, A, B },
      {                      0,     0, 1, 0 }
    }
  };
}

m4 rotate_x(f32 theta) {
  return (m4) {
    .m = {
      { 1.0f,         0.0f,        0.0f, 0.0f },
      { 0.0f,  cosf(theta), sinf(theta), 0.0f },
      { 0.0f, -sinf(theta), cosf(theta), 0.0f },
      { 0.0f,         0.0f,        0.0f, 1.0f }
    }
  };
}


m4 rotate_y(f32 theta) {
  return (m4) {
    .m = {
      {  cosf(theta),  0.0f, sinf(theta), 0.0f },
      {         0.0f,  1.0f,        0.0f, 0.0f },
      { -sinf(theta),  0.0f, cosf(theta), 0.0f },
      {         0.0f,  0.0f,        0.0f, 1.0f }
    }
  };
}

m4 rotate_z(f32 theta) {
  return (m4) {
    .m = {
      {  cosf(theta), sinf(theta), 0.0f, 0.0f },
      { -sinf(theta), cosf(theta), 0.0f, 0.0f },
      {         0.0f,        0.0f, 1.0f, 0.0f },
      {         0.0f,        0.0f, 0.0f, 1.0f }
    }
  };
}

// :rect impl
b8 rect_intersect_inclusive(Rect r1, Rect r2) {
  return (
    ((r1.x <= r2.x && r2.x <= r1.x + r1.w)  ||
     (r2.x <= r1.x && r1.x <= r2.x + r2.w)) &&
    ((r1.y <= r2.y && r2.y <= r1.y + r1.h)  ||
     (r2.y <= r1.y && r1.y <= r2.y + r2.h))
  );
}

b8 rect_intersect(Rect r1, Rect r2) {
  return (
    ((r1.x < r2.x && r2.x < r1.x + r1.w) ||
     (r2.x < r1.x && r1.x < r2.x + r2.w) ||
     (r1.x < r2.x && r2.x + r2.w < r1.x + r1.w) ||
     (r2.x < r1.x && r1.x + r1.w < r2.x + r2.w)) &&
    ((r1.y < r2.y && r2.y < r1.y + r1.h) ||
     (r2.y < r1.y && r1.y < r2.y + r2.h) ||
     (r1.y < r2.y && r2.y + r2.h < r1.y + r1.h) ||
     (r2.y < r1.y && r1.y + r1.h < r2.y + r2.h))
  );
}

b8 point_in_rect(v2 p, Rect r) {
  return (
    (r.x < p.x && p.x < r.x + r.w) &&
    (r.y < p.y && p.y < r.y + r.h)
  );
}

Rect rect_with_offset(v2 pos, Rect offset_rect) {
  return (Rect) {
    pos.x + offset_rect.x,
    pos.y + offset_rect.y,
    offset_rect.w,
    offset_rect.h
  };
}

// :window impl
b8 window_new(Window* window, const char* title, u32 width, u32 height, WindowSettings settings) {
  // Initialize the context
  ctx_begin();

  if (!glfwInit()) {
    log_error("Failed to initialize glfw\n");
    return false;
  }

  GLFWmonitor* monitor = NULL;
  if (settings.fullscreen) {
    monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    // Change the width and height for the fullscreen mode
    width = mode->width;
    height = mode->height;

    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
  }

  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow* glfw_window = glfwCreateWindow(
    width, height,
    title, monitor, NULL
  );
  if (!glfw_window) {
    log_error("Failed to create glfw window\n");
    return false;
  }

  glfwMakeContextCurrent(glfw_window);
  if (glewInit() != GLEW_OK) {
    log_error("Failed to initialize glew\n");
    return false;
  }

  // vsync
  glfwSwapInterval((settings.vsync) ? 1 : 0);

  window->glfw_window = glfw_window;
  window->width = width;
  window->height = height;
  window->should_close = glfwWindowShouldClose(glfw_window);

  return true;
}

void window_delete(Window window) {
  glfwDestroyWindow(window.glfw_window);

  // Deleting the context
  ctx_end();
}

void window_update(Window* window) {
  window->should_close = glfwWindowShouldClose(window->glfw_window);
  glfwSwapBuffers(window->glfw_window);
  glfwPollEvents();
}

// :frame controller impl
FrameController frame_controller_new(i32 fps) {
  return (FrameController) {
    .start_time = glfwGetTime(),
    .start_tick = glfwGetTime(),
    .unit_frame = 1.0f / fps,
    .dt = 0.0f,
    .frame = 0,
    .fps = 0
  };
}

void frame_controller_start(FrameController* fc) {
  fc->start_tick = glfwGetTime();
}

void frame_controller_end(FrameController* fc) {
  fc->frame++;

  fc->dt = glfwGetTime() - fc->start_tick;
  if (fc->unit_frame > fc->dt) {
    f64 sleep_time = fc->unit_frame - fc->dt;
    #if defined(_WIN32)
      Sleep((DWORD)(sleep_time * 1000.0));
    #else
      usleep((useconds_t)(sleep_time * 1000000.0));
    #endif
  }
  fc->dt = glfwGetTime() - fc->start_tick;

  if (glfwGetTime() - fc->start_time >= 1.0f) {
    fc->fps = fc->frame;
    fc->start_time = glfwGetTime();
    fc->frame = 0;
  }
}

// :event impl
void key_callback(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods) {
  Event event = { 0 };
  event.e.key = key;
  switch (action) {
    case GLFW_PRESS:
    case GLFW_REPEAT:
      event.type = KEYDOWN;
      break;
    case GLFW_RELEASE:
      event.type = KEYUP;
      break;
  }

  event_enqueue(&ctx->e_queue, event);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
  Event event = {0};

  switch (button) {
    case GLFW_MOUSE_BUTTON_LEFT:
      event.e.button = MOUSE_BUTTON_LEFT;
      break;
    case GLFW_MOUSE_BUTTON_RIGHT:
      event.e.button = MOUSE_BUTTON_RIGHT;
      break;
    default:
      panic(0, "Unhandled mouse button: %d\n", button);
  }

  switch (action) {
    case GLFW_PRESS:
      event.type = MOUSE_BUTTON_DOWN;
      break;
    case GLFW_RELEASE:
      event.type = MOUSE_BUTTON_UP;
      break;
    default:
      panic(0, "Unhandled mouse action: %d\n", action);
  }

  event_enqueue(&ctx->e_queue, event);
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
  Event event = { 0 };
  event.e.mouse_pos = (v2) { xpos, ypos };
  event.type = MOUSE_MOTION;
  event_enqueue(&ctx->e_queue, event);
}

b8 event_poll(Window window, Event* event) {
  glfwSetKeyCallback(window.glfw_window, key_callback);
  glfwSetMouseButtonCallback(window.glfw_window, mouse_button_callback);
  glfwSetCursorPosCallback(window.glfw_window, cursor_position_callback);

  if (ctx->e_queue.front != ctx->e_queue.back) {
    *event = event_dequeue(&ctx->e_queue);  // Dequeue the next event
    return true;  // Event found
  }
  return false;
}

v2 event_mouse_pos(Window window) {
  f64 xpos, ypos;
  glfwGetCursorPos(window.glfw_window, &xpos, &ypos);
  return (v2) { xpos, ypos };
}

void event_set_mouse_pos(Window window, v2 pos) {
  glfwSetCursorPos(window.glfw_window, pos.x, pos.y);
}

// :event queue impl
void event_enqueue(EventQueue* queue, Event e) {
  int next = (queue->back + 1) % MAX_EVENTS;
  panic(next != queue->front, "EventQueue is full\n");
  queue->events[queue->back] = e;
  queue->back = next;
}

Event event_dequeue(EventQueue* queue) {
  panic(queue->front != queue->back, "EventQueue is empty\n");
  Event value = queue->events[queue->front];
  queue->front = (queue->front + 1) % MAX_EVENTS;
  return value;
}

  // :ocamera impl
OCamera ocamera_new(v2 pos, f32 zoom, OCamera_Boundary boundary) {
  return (OCamera) {
    .pos = pos,
    .zoom = zoom,
    .mvp = m4_zero(),
    .boundary = boundary,
    .active_x = false,
    .active_y = false
  };
}

void ocamera_change_zoom(OCamera* cam, f32 dz) {
  f32 temp = cam->zoom + dz;
  if (temp < 1.0f) return;
  cam->zoom = roundf(temp);
}

void ocamera_change_pos(OCamera* cam, v2 dp) {
  cam->pos = v2_add(cam->pos, dp);
}

void ocamera_follow(OCamera* cam, Rect to_follow_rect, v2 offset, f32 smooth_speed, f32 dt) {
  v2 to_follow = {
    to_follow_rect.x + to_follow_rect.w / 2 + offset.x,
    to_follow_rect.y + to_follow_rect.h / 2 + offset.y
  };

  if (smooth_speed <= 0.0f) {
    cam->pos = to_follow;
  } else {
    f32 t = 1.0f - expf(-smooth_speed * dt);
    v2 change = {
      (to_follow.x - cam->pos.x) * t,
      (to_follow.y - cam->pos.y) * t
    };

    cam->pos.x += change.x;
    cam->pos.y += change.y;
  }
}

m4 ocamera_calc_mvp(OCamera* cam) {
  cam->render_pos = (v2){ roundf(cam->pos.x), roundf(cam->pos.y) };
  cam->pixel_offset = (v2){
    cam->pos.x - cam->render_pos.x,
    cam->pos.y - cam->render_pos.y
  };

  f32 center_x = (cam->boundary.left + cam->boundary.right) / 2.0f;
  f32 center_y = (cam->boundary.top + cam->boundary.bottom) / 2.0f;
  f32 half_w = (cam->boundary.right - cam->boundary.left) / 2.0f * cam->zoom;
  f32 half_h = (cam->boundary.bottom - cam->boundary.top) / 2.0f * cam->zoom;

  m4 proj = ortho_projection(
    center_x - half_w, center_x + half_w,
    center_y - half_h, center_y + half_h,
    cam->boundary.near, cam->boundary.far
  );

  m4 translate = m4_translate(
    m4_identity(),
    (v3){ -cam->render_pos.x, -cam->render_pos.y, 0.0f }
  );

  cam->mvp = m4_mul(translate, proj);
  return cam->mvp;
}

// :pcamera impl
PCamera pcamera_new(v3 pos, v3 dir, f32 sensitivity, PCamera_Info info) {
  dir = v3_normalize(dir);
  v3 up = { 0.0f, 1.0f, 0.0f };
  v3 right = v3_cross(dir, up);
  return (PCamera) {
    .pos = pos,
    .dir = dir,
    .up = up,
    .right = right,
    .pitch = 0.0f,
    .yaw = -90.0f,
    .mp = (v2) { 0, 0 },
    .sensitivity = sensitivity,
    .first = true,
    .mouse_enable = false,
    .info = info
  };
}

void pcamera_change_pos(PCamera* cam, v3 dp) {
  cam->pos = v3_add(cam->pos, dp);
}

void pcamera_handle_mouse(PCamera* cam, Window window) {
  if (!cam->mouse_enable) {
    glfwSetInputMode(
      window.glfw_window,
      GLFW_CURSOR,
      GLFW_CURSOR_NORMAL
    );
    return;
  }

  glfwSetInputMode(
    window.glfw_window,
    GLFW_CURSOR,
    GLFW_CURSOR_DISABLED
  );

  if (cam->first) {
    event_set_mouse_pos(
      window, (v2) {
        window.width / 2.0f,
        window.height / 2.0f
      }
    );
    cam->mp = event_mouse_pos(window);
    cam->first = false;
  }

  v2 p = event_mouse_pos(window);
  v2 dp = v2_sub(cam->mp, p);
  v2 sp = v2_mul_scalar(dp, cam->sensitivity);

  cam->yaw -= sp.x;
  cam->pitch += sp.y;

  if (cam->pitch > 89.0f) {
    cam->pitch = 89.0f;
  }
  else if (cam->pitch < -89.0f) {
    cam->pitch = -89.0f;
  }

  v3 front;
  front.x = cos(to_radians(cam->yaw)) * cos(to_radians(cam->pitch));
  front.y = sin(to_radians(cam->pitch));
  front.z = sin(to_radians(cam->yaw)) * cos(to_radians(cam->pitch));

  cam->dir = v3_normalize(front);
  cam->right = v3_normalize(
    v3_cross(cam->dir, (v3) { 0, 1, 0 })
  );
  cam->up = v3_normalize(
    v3_cross(cam->right, cam->dir)
  );

  cam->mp = p;
}

m4 pcamera_calc_mvp(PCamera* cam) {
  m4 proj = persp_projection(
    cam->info.aspect_ratio,
    cam->info.fov,
    cam->info.near,
    cam->info.far
  );

  m4 camera_mat = {
    .m = {
      { cam->right.x, cam->right.y, cam->right.z, 0.0f },
      { cam->up.x   , cam->up.y   , cam->up.z   , 0.0f },
      { cam->dir.x  , cam->dir.y  , cam->dir.z  , 0.0f },
      {         0.0f,         0.0f,         0.0f, 1.0f }
    }
  };

  m4 camera_trans = m4_translate(
    m4_identity(),
    (v3) { -cam->pos.x, -cam->pos.y, -cam->pos.z }
  );

  cam->look_at = m4_mul(camera_mat, camera_trans);
  cam->mvp = m4_mul(proj, cam->look_at);
  return cam->mvp;
}

// :texture impl
b8 texture_from_file(Texture* texture, const char* filepath, b8 flip) {
  panic(texture, "Parameter texture cannot be NULL");

  stbi_set_flip_vertically_on_load(flip);

  i32 w, h, c;
  u8* data = stbi_load(filepath, &w, &h, &c, 0);
  if (!data) {
    log_error("Failed to load file: %s\n", filepath);
    return false;
  }

  // Binding the texture
  u32 id;
  GLCall(glGenTextures(1, &id));
  GLCall(glBindTexture(GL_TEXTURE_2D, id));

  // Setting up some basic modes to display texture
  GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
  GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
  GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

  // Sending the pixel data to opengl
  GLenum internal_format, format;
  if (c == 1) {
    internal_format = GL_RED;
    format = GL_RED;
  } else if (c == 3) {
    internal_format = GL_RGB;
    format = GL_RGB;
  } else if (c == 4) {
    internal_format = GL_RGBA;
    format = GL_RGBA;
  }

  GLCall(glTexImage2D(GL_TEXTURE_2D, 0, internal_format, w, h, 0, format, GL_UNSIGNED_BYTE, data));
  GLCall(glBindTexture(GL_TEXTURE_2D, 0));

  if (data) {
    stbi_image_free(data);
  }

  texture->id = id;
  texture->width = w;
  texture->height = h;
  return true;
}

b8 texture_from_data(Texture* texture, u32 width, u32 height, u8* data, i32 channel) {
  panic(texture, "Parameter texture cannot be NULL");

  u32 id;
  GLCall(glGenTextures(1, &id));
  GLCall(glBindTexture(GL_TEXTURE_2D, id));

  // Setting up some basic modes to display texture
  GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
  GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
  GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

  // Sending the pixel data to opengl
  GLenum internal_format, format;
  if (channel == 1) {
    internal_format = GL_RED;
    format = GL_RED;
  } else if (channel == 3) {
    internal_format = GL_RGB;
    format = GL_RGB;
  } else if (channel == 4) {
    internal_format = GL_RGBA;
    format = GL_RGBA;
  } else {
    log_error("Invalid channel %d, Only Supports (1, 3, 4)\n", channel);
  }

  // Sending the pixel data to opengl
  GLCall(glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, GL_UNSIGNED_BYTE, data));
  GLCall(glBindTexture(GL_TEXTURE_2D, 0));

  texture->id = id;
  texture->width = width;
  texture->height = height;
  return true;
}

void texture_clear(Texture texture) {
  u32 t = 0;
  GLCall(glClearTexImage(texture.id, 0, GL_RGBA, GL_UNSIGNED_BYTE, &t));
}

void texture_bind(Texture texture) {
  GLCall(glActiveTexture(GL_TEXTURE0 + (i32) texture.id));
  GLCall(glBindTexture(GL_TEXTURE_2D, texture.id));
}

void texture_unbind(Texture texture) {
  GLCall(glActiveTexture(GL_TEXTURE0 + (i32) texture.id));
  GLCall(glBindTexture(GL_TEXTURE_2D, 0));
}

void texture_delete(Texture texture) {
  GLCall(glDeleteTextures(1, &texture.id));
}

// :shader impl
b8 shader_new(Shader* shader, const char* v_src, const char* f_src) {
  panic(shader , "Parameter shader cannot be NULL");

  u32 program = glCreateProgram();
  u32 vs, fs;

  if (!shader_compile(&vs, GL_VERTEX_SHADER,   v_src)) return false;
  if (!shader_compile(&fs, GL_FRAGMENT_SHADER, f_src)) return false;

  // Attaching shader
  GLCall(glAttachShader(program, vs));
  GLCall(glAttachShader(program, fs));
  GLCall(glLinkProgram(program));
  GLCall(glValidateProgram(program));

  GLCall(glDeleteShader(vs));
  GLCall(glDeleteShader(fs));

  *shader = program;
  return true;
}

b8 shader_new_from_file(Shader* shader, const char* v_path, const char* f_path) {
  panic(shader , "Parameter shader cannot be NULL");

  FileData v_file = {0};
  FileData f_file = {0};

  if (!read_file(&v_file, v_path)) return false;
  if (!read_file(&f_file, f_path)) return false;

  char* v_src = v_file.data;
  char* f_src = f_file.data;

  if (!shader_new(shader, v_src, f_src)) return false;

  free(v_src);
  free(f_src);
  return true;
}

void shader_delete(Shader id) {
  GLCall(glDeleteProgram(id));
}

b8 shader_compile(u32* src_id, ShaderType type, const char* shader_src) {
  panic(src_id, "Parameter src_id cannot be NULL");

  u32 id = glCreateShader(type);
  GLCall(glShaderSource(id, 1, &shader_src, NULL));
  GLCall(glCompileShader(id));

  // Checking error in shader
  i32 result;
  GLCall(glGetShaderiv(id, GL_COMPILE_STATUS, &result));
  if (result == GL_FALSE) {
    i32 length;
    GLCall(glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length));

    char message[length];
    GLCall(glGetShaderInfoLog(id, length, &length, message));

    log_error("Failed to compile [%s shader]\n%s\n",
      (type == GL_VERTEX_SHADER ? "Vertex" : "Fragment"), message
    );
    return false;
  }

  *src_id = id;
  return true;
}

// :fbo impl
b8 fbo_new(FBO* fbo, u32 width, u32 height) {
  panic(fbo, "Parameter fbo cannot be NULL");

  u32 id;

  // Generate and bind framebuffer
  GLCall(glGenFramebuffers(1, &id));
  GLCall(glBindFramebuffer(GL_FRAMEBUFFER, id));

  // Color texture
  Texture color_texture;
  if (!texture_from_data(&color_texture, width, height, NULL, 4)) return false;

  GLCall(glFramebufferTexture2D(
    GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
    GL_TEXTURE_2D, color_texture.id, 0
  ));

  // Setting up draw buffers
  GLuint attachments[1] = { GL_COLOR_ATTACHMENT0 };
  GLCall(glDrawBuffers(1, attachments));

  GLCall(glBindTexture(GL_TEXTURE_2D, 0));
  GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    log_error("Framebuffer is not complete!\n");
  }

  fbo->id = id;
  fbo->width = width;
  fbo->height = height;
  fbo->color_texture = color_texture;
  return true;
}

void fbo_delete(FBO* fbo) {
  texture_delete(fbo->color_texture);
  GLCall(glDeleteFramebuffers(1, &fbo->id));
}

void fbo_bind(FBO* fbo) {
  texture_bind(fbo->color_texture);
  texture_clear(fbo->color_texture);
  GLCall(glBindFramebuffer(GL_FRAMEBUFFER, fbo->id));
}

void fbo_unbind() {
  GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

// :imr impl
const char* __internal_v_src =
  "#version 330 core\n"
  "layout (location = 0) in vec3 position;\n"
  "layout (location = 1) in vec4 color;\n"
  "layout (location = 2) in vec2 tex_coord;\n"
  "layout (location = 3) in float tex_id;\n"
  "layout (location = 4) in vec4 overlay_color;\n"
  "layout (location = 5) in vec2 local;\n"
  "layout (location = 6) in vec2 size;\n"
  "layout (location = 7) in float radius;\n"
  "uniform mat4 mvp;\n"
  "out vec4 o_color;\n"
  "out vec2 o_tex_coord;\n"
  "out float o_tex_id;\n"
  "out vec4 o_overlay_color;\n"
  "out vec2 o_local;\n"
  "out vec2 o_size;\n"
  "out float o_radius;\n"
  "void main() {\n"
  "o_color = color;\n"
  "o_tex_coord = tex_coord;\n"
  "o_tex_id = tex_id;\n"
  "o_overlay_color = overlay_color;\n"
  "o_local = local;\n"
  "o_size = size;\n"
  "o_radius = radius;\n"
  "gl_Position = vec4(position, 1.0f) * mvp;\n"
  "}\n";

const char* __internal_f_src =
  "#version 330 core\n"
  "layout (location = 0) out vec4 color;\n"
  "uniform sampler2D textures[32];\n"
  "in vec4 o_color;\n"
  "in vec2 o_tex_coord;\n"
  "in float o_tex_id;\n"
  "in vec4 o_overlay_color;\n"
  "in vec2 o_local;\n"
  "in vec2 o_size;\n"
  "in float o_radius;\n"
  "void main() {\n"
  "int index = int(o_tex_id);\n"
  "vec4 t_color;\n"
  "switch (index) {\n"
  "case 0: t_color = texture(textures[0], o_tex_coord) * o_color; break;\n"
  "case 1: t_color = texture(textures[1], o_tex_coord) * o_color; break;\n"
  "case 2: t_color = texture(textures[2], o_tex_coord) * o_color; break;\n"
  "case 3: t_color = texture(textures[3], o_tex_coord) * o_color; break;\n"
  "case 4: t_color = texture(textures[4], o_tex_coord) * o_color; break;\n"
  "case 5: t_color = texture(textures[5], o_tex_coord) * o_color; break;\n"
  "case 6: t_color = texture(textures[6], o_tex_coord) * o_color; break;\n"
  "case 7: t_color = texture(textures[7], o_tex_coord) * o_color; break;\n"
  "case 8: t_color = texture(textures[8], o_tex_coord) * o_color; break;\n"
  "case 9: t_color = texture(textures[9], o_tex_coord) * o_color; break;\n"
  "case 10: t_color = texture(textures[10], o_tex_coord) * o_color; break;\n"
  "case 11: t_color = texture(textures[11], o_tex_coord) * o_color; break;\n"
  "case 12: t_color = texture(textures[12], o_tex_coord) * o_color; break;\n"
  "case 13: t_color = texture(textures[13], o_tex_coord) * o_color; break;\n"
  "case 14: t_color = texture(textures[14], o_tex_coord) * o_color; break;\n"
  "case 15: t_color = texture(textures[15], o_tex_coord) * o_color; break;\n"
  "case 16: t_color = texture(textures[16], o_tex_coord) * o_color; break;\n"
  "case 17: t_color = texture(textures[17], o_tex_coord) * o_color; break;\n"
  "case 18: t_color = texture(textures[18], o_tex_coord) * o_color; break;\n"
  "case 19: t_color = texture(textures[19], o_tex_coord) * o_color; break;\n"
  "case 20: t_color = texture(textures[20], o_tex_coord) * o_color; break;\n"
  "case 21: t_color = texture(textures[21], o_tex_coord) * o_color; break;\n"
  "case 22: t_color = texture(textures[22], o_tex_coord) * o_color; break;\n"
  "case 23: t_color = texture(textures[23], o_tex_coord) * o_color; break;\n"
  "case 24: t_color = texture(textures[24], o_tex_coord) * o_color; break;\n"
  "case 25: t_color = texture(textures[25], o_tex_coord) * o_color; break;\n"
  "case 26: t_color = texture(textures[26], o_tex_coord) * o_color; break;\n"
  "case 27: t_color = texture(textures[27], o_tex_coord) * o_color; break;\n"
  "case 28: t_color = texture(textures[28], o_tex_coord) * o_color; break;\n"
  "case 29: t_color = texture(textures[29], o_tex_coord) * o_color; break;\n"
  "case 30: t_color = texture(textures[30], o_tex_coord) * o_color; break;\n"
  "case 31: t_color = texture(textures[31], o_tex_coord) * o_color; break;\n"
  "}\n"
  "color = mix(t_color, vec4(o_overlay_color.rgb, t_color.a), o_overlay_color.a);\n"
  "if (o_radius > 0.0) {\n"
  "vec2 half_size = o_size * 0.5;\n"
  "float r = min(o_radius, min(half_size.x, half_size.y));\n"
  "vec2 q = abs(o_local) - half_size + r;\n"
  "float d = min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;\n"
  "float aa = fwidth(d);\n"
  "color.a *= 1.0 - smoothstep(-aa, aa, d);\n"
  "}\n"
  "}\n";

b8 imr_new(IMR* imr) {
  panic(imr, "Parameter imr cannot be NULL");

  u32 vao, vbo;

  GLCall(glEnable(GL_BLEND));
  GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

  // Buffers
  GLCall(glGenVertexArrays(1, &vao));
  GLCall(glBindVertexArray(vao));

  GLCall(glGenBuffers(1, &vbo));
  GLCall(glBindBuffer(GL_ARRAY_BUFFER, vbo));
  GLCall(glBufferData(GL_ARRAY_BUFFER, MAX_VBO_SIZE, NULL, GL_DYNAMIC_DRAW));

  // VAO format
  STATIC_ASSERT(
    19 == sizeof(Vertex) / sizeof(f32),
    "Vertex has been updated. Update VAO format."
  );

  GLCall(glEnableVertexAttribArray(0));
  GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*) offsetof(Vertex, pos)));
  GLCall(glEnableVertexAttribArray(1));
  GLCall(glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*) offsetof(Vertex, color)));
  GLCall(glEnableVertexAttribArray(2));
  GLCall(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*) offsetof(Vertex, tex_coord)));
  GLCall(glEnableVertexAttribArray(3));
  GLCall(glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*) offsetof(Vertex, tex_id)));
  GLCall(glEnableVertexAttribArray(4));
  GLCall(glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*) offsetof(Vertex, overlay_color)));
  GLCall(glEnableVertexAttribArray(5));
  GLCall(glVertexAttribPointer(5, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*) offsetof(Vertex, local)));
  GLCall(glEnableVertexAttribArray(6));
  GLCall(glVertexAttribPointer(6, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*) offsetof(Vertex, size)));
  GLCall(glEnableVertexAttribArray(7));
  GLCall(glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*) offsetof(Vertex, radius)));

  // Generating white texture
  u8 data = 0xff;
  Texture white;
  if (!texture_from_data(&white, 1, 1, &data, 4)) return false;
  texture_bind(white);

  // Shader
  Shader shader;
  if (!shader_new(&shader, __internal_v_src, __internal_f_src)) {
    return false;
  }
  GLCall(glUseProgram(shader));

  // Providing texture samples
  u32 samplers[TEXTURE_SAMPLE_AMT];
  for (u32 i = 0; i < TEXTURE_SAMPLE_AMT; i++)
    samplers[i] = i;

  // Providing samplers to the shader
  int loc = GLCall(glGetUniformLocation(shader, "textures"));
  panic(loc != -1, "Cannot find uniform: textures\n");
  GLCall(glUniform1iv(loc, TEXTURE_SAMPLE_AMT, samplers));

  imr->vao = vao;
  imr->vbo = vbo;
  imr->shader = shader;
  imr->def_shader = shader;
  imr->buff_idx = 0;
  imr->white = white;
  return true;
}

void imr_delete(IMR* imr) {
  GLCall(glDeleteVertexArrays(1, &imr->vao));
  GLCall(glDeleteBuffers(1, &imr->vbo));
  texture_delete(imr->white);
  shader_delete(imr->shader);
  shader_delete(imr->def_shader);
}

void imr_clear(v4 color) {
  GLCall(glClearColor(color.r, color.g, color.b, color.a));
  GLCall(glClear(GL_COLOR_BUFFER_BIT));
}

void imr_begin(IMR* imr) {
  imr->buff_idx = 0;
  texture_bind(imr->white);
  GLCall(glUseProgram(imr->shader));
  GLCall(glBindBuffer(GL_ARRAY_BUFFER, imr->vbo));
}

void imr_end(IMR* imr) {
  GLCall(glBindBuffer(GL_ARRAY_BUFFER, imr->vbo));
  GLCall(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(imr->buffer), imr->buffer));

  GLCall(glBindVertexArray(imr->vao));
  GLCall(glDrawArrays(GL_TRIANGLES, 0, imr->buff_idx / VERTEX_SIZE));
}

void imr_switch_shader(IMR* imr, Shader shader) {
  imr->shader = shader;
  GLCall(glUseProgram(imr->shader));
}

void imr_switch_shader_to_default(IMR* imr) {
  imr->shader = imr->def_shader;
  imr_reapply_samplers(imr);
}

void imr_reapply_samplers(IMR* imr) {
  GLCall(glUseProgram(imr->shader));

  // Providing texture samples
  u32 samplers[TEXTURE_SAMPLE_AMT];
  for (u32 i = 0; i < TEXTURE_SAMPLE_AMT; i++)
    samplers[i] = i;
  
  // Providing samplers to the shader
  int loc = GLCall(glGetUniformLocation(imr->shader, "textures"));
  panic(loc != -1, "Cannot find uniform: textures\n");
  GLCall(glUniform1iv(loc, TEXTURE_SAMPLE_AMT, samplers));
}

void imr_update_mvp(IMR* imr, m4 mvp) {
  i32 loc = GLCall(glGetUniformLocation(imr->shader, "mvp"));
  GLCall(glUniformMatrix4fv(loc, 1, GL_TRUE, &mvp.m[0][0]));
}

void imr_push_vertex(IMR* imr, Vertex v) {
  STATIC_ASSERT(
    19 == sizeof(Vertex) / sizeof(f32),
    "Vertex has been updated. Update this method."
  );

  imr->buffer[imr->buff_idx++] = v.pos.x;
  imr->buffer[imr->buff_idx++] = v.pos.y;
  imr->buffer[imr->buff_idx++] = v.pos.z;
  imr->buffer[imr->buff_idx++] = v.color.r;
  imr->buffer[imr->buff_idx++] = v.color.g;
  imr->buffer[imr->buff_idx++] = v.color.b;
  imr->buffer[imr->buff_idx++] = v.color.a;
  imr->buffer[imr->buff_idx++] = v.tex_coord.x;
  imr->buffer[imr->buff_idx++] = v.tex_coord.y;
  imr->buffer[imr->buff_idx++] = v.tex_id;
  imr->buffer[imr->buff_idx++] = v.overlay_color.r;
  imr->buffer[imr->buff_idx++] = v.overlay_color.g;
  imr->buffer[imr->buff_idx++] = v.overlay_color.b;
  imr->buffer[imr->buff_idx++] = v.overlay_color.a;
  imr->buffer[imr->buff_idx++] = v.local.x;
  imr->buffer[imr->buff_idx++] = v.local.y;
  imr->buffer[imr->buff_idx++] = v.size.x;
  imr->buffer[imr->buff_idx++] = v.size.y;
  imr->buffer[imr->buff_idx++] = v.radius;
}

void imr_push_quad(IMR* imr, v3 pos, v2 size, m4 rot, v4 color) {
  Rect tex_rect = {
    0, 0, 1, 1
  };
  imr_push_quad_tex(imr, pos, size, tex_rect, imr->white.id, rot, color);
}

void imr_push_quad_rounded(IMR* imr, v3 pos, v2 size, m4 rot, v4 color, f32 radius) {
  Rect tex_rect = {
    0, 0, 1, 1
  };
  imr_push_quad_tex_rounded(imr, pos, size, tex_rect, imr->white.id, rot, color, radius);
}

void imr_push_quad_overlay(IMR* imr, v3 pos, v2 size, m4 rot, v4 color, v4 overlay_color) {
  Rect tex_rect = {
    0, 0, 1, 1
  };
  imr_push_quad_tex_overlay(imr, pos, size, tex_rect, imr->white.id, rot, color, overlay_color);
}

void imr_push_quad_overlay_rounded(IMR* imr, v3 pos, v2 size, m4 rot, v4 color, v4 overlay_color, f32 radius) {
  Rect tex_rect = {
    0, 0, 1, 1
  };
  imr_push_quad_tex_overlay_rounded(imr, pos, size, tex_rect, imr->white.id, rot, color, overlay_color, radius);
}

void imr_push_quad_tex(IMR* imr, v3 pos, v2 size, Rect tex_rect, f32 tex_id, m4 rot, v4 color) {
  imr_push_quad_tex_overlay(imr, pos, size, tex_rect, tex_id, rot, color, (v4) {0});
}

void imr_push_quad_tex_rounded(IMR* imr, v3 pos, v2 size, Rect tex_rect, f32 tex_id, m4 rot, v4 color, f32 radius) {
  imr_push_quad_tex_overlay_rounded(imr, pos, size, tex_rect, tex_id, rot, color, (v4) {0}, radius);
}

void imr_push_quad_tex_overlay(IMR* imr, v3 pos, v2 size, Rect tex_rect, f32 tex_id, m4 rot, v4 color, v4 overlay_color) {
  imr_push_quad_tex_overlay_rounded(imr, pos, size, tex_rect, tex_id, rot, color, overlay_color, 0.0f);
}

void imr_push_quad_tex_overlay_rounded(IMR* imr, v3 pos, v2 size, Rect tex_rect, f32 tex_id, m4 rot, v4 color, v4 overlay_color, f32 radius) {
  if (((imr->buff_idx + 6 * VERTEX_SIZE) / VERTEX_SIZE) >= MAX_VERT_CNT) {
    imr_end(imr);
    imr_begin(imr);
  }

  Vertex p1, p2, p3, p4, p5, p6;

  // Rotating over origin
  p1.pos = m4_mul_v3(rot, (v3) { -size.x / 2, -size.y / 2, 0.0f });
  p2.pos = m4_mul_v3(rot, (v3) {  size.x / 2, -size.y / 2, 0.0f });
  p3.pos = m4_mul_v3(rot, (v3) {  size.x / 2,  size.y / 2, 0.0f });

  p4.pos = m4_mul_v3(rot, (v3) {  size.x / 2,  size.y / 2, 0.0f });
  p5.pos = m4_mul_v3(rot, (v3) { -size.x / 2,  size.y / 2, 0.0f });
  p6.pos = m4_mul_v3(rot, (v3) { -size.x / 2, -size.y / 2, 0.0f });

  // Shifting to the desired position
  p1.pos = v3_add(p1.pos, (v3) { pos.x + size.x / 2, pos.y + size.y / 2, pos.z });
  p2.pos = v3_add(p2.pos, (v3) { pos.x + size.x / 2, pos.y + size.y / 2, pos.z });
  p3.pos = v3_add(p3.pos, (v3) { pos.x + size.x / 2, pos.y + size.y / 2, pos.z });

  p4.pos = v3_add(p4.pos, (v3) { pos.x + size.x / 2, pos.y + size.y / 2, pos.z });
  p5.pos = v3_add(p5.pos, (v3) { pos.x + size.x / 2, pos.y + size.y / 2, pos.z });
  p6.pos = v3_add(p6.pos, (v3) { pos.x + size.x / 2, pos.y + size.y / 2, pos.z });

  // Making the texure coordinates
  p1.tex_coord = (v2) { tex_rect.x, tex_rect.y };
  p2.tex_coord = (v2) { tex_rect.x + tex_rect.w, tex_rect.y };
  p3.tex_coord = (v2) { tex_rect.x + tex_rect.w, tex_rect.y + tex_rect.h };
  p4.tex_coord = (v2) { tex_rect.x + tex_rect.w, tex_rect.y + tex_rect.h };
  p5.tex_coord = (v2) { tex_rect.x, tex_rect.y + tex_rect.h };
  p6.tex_coord = (v2) { tex_rect.x, tex_rect.y };

  p1.color = p2.color = p3.color = p4.color = p5.color = p6.color = color;
  p1.tex_id = p2.tex_id = p3.tex_id = p4.tex_id = p5.tex_id = p6.tex_id = tex_id;
  p1.overlay_color = p2.overlay_color = p3.overlay_color = p4.overlay_color = p5.overlay_color = p6.overlay_color = overlay_color;

  // Quad-local coordinates and rounded corner parameters
  p1.local = p6.local = (v2) { -size.x / 2, -size.y / 2 };
  p2.local = (v2) {  size.x / 2, -size.y / 2 };
  p3.local = p4.local = (v2) {  size.x / 2,  size.y / 2 };
  p5.local = (v2) { -size.x / 2,  size.y / 2 };

  p1.size = p2.size = p3.size = p4.size = p5.size = p6.size = size;
  p1.radius = p2.radius = p3.radius = p4.radius = p5.radius = p6.radius = radius;

  imr_push_vertex(imr, p1);
  imr_push_vertex(imr, p2);
  imr_push_vertex(imr, p3);
  imr_push_vertex(imr, p4);
  imr_push_vertex(imr, p5);
  imr_push_vertex(imr, p6);
}

void imr_push_triangle(IMR* imr, v3 p1, v3 p2, v3 p3, m4 rot, v4 color) {
  Triangle tex_coord = {
    (v3) { 0, 0, 0 },
    (v3) { 1, 0, 0 },
    (v3) { 1, 1, 0 }
  };
  imr_push_triangle_tex(imr, p1, p2, p3, tex_coord, imr->white.id, rot, color);
}

void imr_push_triangle_tex(IMR* imr, v3 p1, v3 p2, v3 p3, Triangle tex_coord, f32 tex_id, m4 rot, v4 color) {
  if (((imr->buff_idx + 3 * VERTEX_SIZE) / VERTEX_SIZE) >= MAX_VERT_CNT) {
    imr_end(imr);
    imr_begin(imr);
  }

  v3 centroid = {
    (p1.x + p2.x + p3.x) / 3.0f,
    (p1.y + p2.y + p3.y) / 3.0f,
    (p1.z + p2.z + p3.z) / 3.0f,
  };

  Vertex a1 = {0};
  Vertex a2 = {0};
  Vertex a3 = {0};

  // Rotating over origin
  a1.pos = m4_mul_v3(rot, (v3) { p1.x - centroid.x, p1.y - centroid.y, p1.z - centroid.z });
  a2.pos = m4_mul_v3(rot, (v3) { p2.x - centroid.x, p2.y - centroid.y, p2.z - centroid.z });
  a3.pos = m4_mul_v3(rot, (v3) { p3.x - centroid.x, p3.y - centroid.y, p3.z - centroid.z });

  // Shifting to the desired position
  a1.pos = v3_add(a1.pos, (v3) { centroid.x, centroid.y, centroid.z });
  a2.pos = v3_add(a2.pos, (v3) { centroid.x, centroid.y, centroid.z });
  a3.pos = v3_add(a3.pos, (v3) { centroid.x, centroid.y, centroid.z });

  // Texture coordinates
  a1.tex_coord = (v2) { tex_coord.a.x, tex_coord.a.y };
  a2.tex_coord = (v2) { tex_coord.b.x, tex_coord.b.y };
  a3.tex_coord = (v2) { tex_coord.c.x, tex_coord.c.y };

  a1.color = a2.color = a3.color = color;
  a1.tex_id = a2.tex_id = a3.tex_id = tex_id;

  imr_push_vertex(imr, a1);
  imr_push_vertex(imr, a2);
  imr_push_vertex(imr, a3);
}

// :spritesheet impl
b8 spritesheet_new(SpriteSheet* ss, const char* path, i32 tile_width, i32 tile_height) {
  panic(ss, "Parameter ss cannot be NULL");

  Texture texture;
  if (!texture_from_file(&texture, path, false)) return false;

  i32 rows = floorf(texture.height / tile_height);
  i32 cols = floorf(texture.width / tile_width);

  ss->texture = texture;
  ss->rows = rows;
  ss->cols = cols;
  return true;
}

void spritesheet_delete(SpriteSheet* ss) {
  texture_delete(ss->texture);
}

void spritesheet_bind(SpriteSheet* ss) {
  texture_bind(ss->texture);
}

void spritesheet_unbind(SpriteSheet* ss) {
  texture_unbind(ss->texture);
}

Frames spritesheet_extract_frames(SpriteSheet* ss, i32 at_row, i32 column_start, i32 column_end) {
  panic(ss, "Parameter ss cannot be NULL");

  Frames frames = {0};

  for (i32 i = column_start; i <= column_end; i++) {
    f32 x = (f32) i      / ss->cols;
    f32 y = (f32) at_row / ss->rows;
    f32 w = 1.0f         / ss->cols;
    f32 h = 1.0f         / ss->rows;
    da_append(&frames, ((Frame) {x, y, w, h}));
  }

  return frames;
}

Frame spritesheet_extract_frame(SpriteSheet* ss, i32 row, i32 col) {
  panic(ss, "Parameter ss cannot be NULL");

  return (Frame) {
    .x = (f32) col / ss->cols,
    .y = (f32) row / ss->rows,
    .w = 1.0f / ss->cols,
    .h = 1.0f / ss->rows,
  };
}

Frame spritesheet_extract_region(
  SpriteSheet* ss,
  i32 row, i32 col,
  i32 row_span, i32 col_span
) {
  panic(ss, "Parameter ss cannot be NULL");

  return (Frame) {
    .x = (f32) col / ss->cols,
    .y = (f32) row / ss->rows,
    .w = (f32) row_span / ss->cols,
    .h = (f32) col_span / ss->rows,
  };
}

// :animator impl
Animator animator_new() {
  return (Animator) {
    .curr_state = -1,
    .entries = {0},
    .start_time = glfwGetTime(),
  };
}

void animator_delete(Animator* animator) {
  panic(animator, "Parameter animator cannot be NULL");

  for (i32 i = 0; i < animator->entries.len; i++) {
    // Free the internal frame of each animation
    mem_free(animator->entries.items[i].frames.items);
  }

  // Free the entries
  mem_free(animator->entries.items);
}

void animator_add_animation(Animator* animator, Animation animation) {
  da_append(&animator->entries, animation);
}

Animation* animator_get_animation(Animator* animator, i32 state) {
  for (i32 i = 0; i < animator->entries.len; i++) {
    Animation* anim = &animator->entries.items[i];
    if (anim->id == state) {
      return anim;
    }
  }
  panic(0, "Invalid state provided: %d", state);
}

void animator_switch(Animator* animator, i32 state) {
  if (state == animator->curr_state)
    return;

  animator->curr_state = state;
  animator->start_time = glfwGetTime();

  // Reset the animation frame start
  Animation* anim = animator_get_animation(animator, state);
  anim->curr_frame = 0;
}

Frame animator_get_frame(Animator* animator) {
  Animation* anim = animator_get_animation(animator, animator->curr_state);

  f64 dt = (glfwGetTime() - animator->start_time) * 1000;

  // Responsible for looping the animation
  if (dt >= anim->duration && anim->loop) {
    animator->start_time = glfwGetTime();
    dt = (glfwGetTime() - animator->start_time) * 1000;
  }

  f64 fpt = anim->frames.len / anim->duration;
  anim->curr_frame = floor(fpt * dt);

  // NOTE: Limiting the current frame index cuz sometimes it crashes due to overflow
  if (anim->curr_frame >= anim->frames.len)
    anim->curr_frame = anim->frames.len - 1;

  panic(
    anim->curr_frame < anim->frames.len,
    "Invalid animation frame: id: %d count: %d",
    anim->curr_frame, anim->frames.len
  );

  return anim->frames.items[anim->curr_frame];
}

// :font impl
b8 font_new(Font* font, const char* path, f32 size) {
  panic(font, "Parameter font cannot be NULL");

  FileData file = {0};
  if (!read_file(&file, path)) return false;

  u8* bitmap = mem_alloc(FONT_ATLAS_W * FONT_ATLAS_H);

  stbtt_packedchar chars[96];
  stbtt_pack_context ctx;

  // Pack the bitmap into the cpu buffer
  i32 ok = stbtt_PackBegin(
    &ctx,
    bitmap,
    FONT_ATLAS_W,
    FONT_ATLAS_H,
    0,
    1,
    NULL
  );
  if (!ok) {
    log_error("Failed creating font atlas\n");
    return false;
  }

  stbtt_PackFontRange(
    &ctx,
    (u8*)file.data,
    0,
    size,
    32,
    96,
    chars
  );

  stbtt_PackEnd(&ctx);

  // convert RED -> RGBA
  u32* rgba = mem_alloc(FONT_ATLAS_W * FONT_ATLAS_H * sizeof(u32));

  for(i32 i = 0; i < FONT_ATLAS_W * FONT_ATLAS_H; i++) {
    u8 a = bitmap[i];
    rgba[i] = (a << 24) | 0xffffff;
  }

  // Create opengl texture from the rgba buffer
  font->size = size;
  if (!texture_from_data(&font->atlas, FONT_ATLAS_W, FONT_ATLAS_H, (u8*) rgba, 4))
    return false;

  // Load the glyphs
  for(i32 i = 0; i < 96; i++) {
    stbtt_packedchar c = chars[i];
    font->glyphs[i + 32] = (Glyph) {
      .uv = {
        c.x0 / (f32)FONT_ATLAS_W,
        c.y0 / (f32)FONT_ATLAS_H,

        (c.x1 - c.x0)
          / (f32)FONT_ATLAS_W,

        (c.y1 - c.y0)
          / (f32)FONT_ATLAS_H
      },

      .size = {
        c.x1 - c.x0,
        c.y1 - c.y0
      },

      .offset = {
        c.xoff,
        c.yoff
      },

      .advance = c.xadvance
    };
  }

  free(file.data);
  mem_free(bitmap);
  mem_free(rgba);
  return true;
}

void font_render(IMR* imr, Font* font, const char* text, v3 pos, v4 color) {
  texture_bind(font->atlas);

  f32 x = pos.x;
  f32 y = pos.y;

  for(const char* c = text; *c; c++) {
    if(*c == '\n') {
      y += font->size;
      x = pos.x;
      continue;
    }

    Glyph g = font->glyphs[(i32)*c];
    v3 p = {
      x + g.offset.x,
      y + g.offset.y,
      pos.z
    };

    imr_push_quad_tex(
      imr,
      p,
      g.size,
      g.uv,
      font->atlas.id,
      m4_identity(),
      color
    );

    x += g.advance;
  }
}

v2 font_calc_size(Font* font, const char* text) {
  v2 size = {0};

  for (const char* c = text; *c; c++) {
    Glyph g = font->glyphs[(i32)*c];
    size.x += g.advance;
    if (size.y < g.size.y) size.y = g.size.y;
  }

  return size;
}

void font_delete(Font* font) {
  texture_delete(font->atlas);
}

// :impl end

// :external impl
#define STB_IMAGE_IMPLEMENTATION
#include "external/stb/stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "external/stb/stb_truetype.h"

#endif // BASE_IMPLEMENTATION

#endif // __BASE_H__
