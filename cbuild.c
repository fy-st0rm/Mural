#define CBUILD_IMPLEMENTATION
#include "cbuild.h"

int main(int argc, char** argv) {
  cbuild_rebuild_itself(argc, argv, "gcc");

  CBuild build = {0};
  cbuild_cc(&build, "gcc");
  cbuild_out(&build, "bin/mural");

  cbuild_flags(&build, "-D_GNU_SOURCE");
  cbuild_include_paths(
    &build,
    "./src/",
    "./src/external/glew/include/",
    "./src/external/glfw/include/"
  );
  cbuild_libs(
    &build,
    "GL",
    "GLU",
    "m"
  );
  cbuild_srcs(
    &build,
    "src/external/glew/src/glew.c",
    "src/external/glfw/src/glfw.c",
    "src/main.c"
  );

  cbuild_execute(&build);
  return cbuild_run_bin(&build, argc, argv);
}
