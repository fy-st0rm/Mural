#define CBUILD_IMPLEMENTATION
#include "cbuild.h"

int main(int argc, char** argv) {
  cbuild_rebuild_itself(argc, argv, "gcc");

  CBuild build = {0};
  cbuild_cc(&build, "gcc");
  cbuild_out(&build, "./mural");

  cbuild_flags(&build, "-D_GNU_SOURCE", "-ggdb");
  cbuild_include_paths(
    &build,
    "./src/",
    "./src/external/glew/include/",
    "./src/external/glfw/include/",
    "./src/external/tomlc17/"
  );
  cbuild_libs(
    &build,
    "GL",
    "GLU",
    "m",
    "pthread",
    "jpeg",
    "spng",
    "z"
  );
  cbuild_srcs(
    &build,
    "src/external/glew/src/glew.c",
    "src/external/glfw/src/glfw.c",
    "src/external/tomlc17/tomlc17.c",
    "src/base.c",
    "src/img.c",
    "src/main.c"
  );

  cbuild_execute(&build);
  return cbuild_run_bin(&build, argc, argv);
}
