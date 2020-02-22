
#include "ppm_file.h"
#include "camera.h"
#include "scene_config.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *input_file_name;
static const char *gen_type;
static const char *output_file_name;

static void print_usage(const char *program_name) {
  fprintf(stderr,
          "invalid usage: raytracer [input desc file] [-g gradient/mandel] [-o outputfile]\n");
  exit(EXIT_FAILURE);
}

// Returns the filename without the extension
// i.e. myfile.txt -> myfile
// If no extension, returns end of file
static const char *get_base_filename(const char *filename) {
  const char *last_slash = strrchr(filename, '/');
  const char *dot = strrchr(filename, '.');

  int is_empty_or_just_dot = !dot || dot == filename;
  int no_dot_in_filename = last_slash && last_slash > dot;
  if (is_empty_or_just_dot || no_dot_in_filename)
    return filename + strlen(filename);
  return dot;
}

// Replaces file extension of filename with ext
// Returns new string that must be freed
static char *replace_file_ext(const char *filename, const char *ext) {
  size_t new_filename_size = strlen(filename) + strlen(ext) + 1;
  char *out =
    malloc(new_filename_size); // even if filename has no extension, space is
  // available for ext and extra space
  memset(out, 0, new_filename_size);
  strncpy(out, filename, get_base_filename(filename) - filename);
  strcat(out, ".");
  strcat(out, ext);
  return out;
}

static void parse_args(int argc, char **argv) {
  if (argc < 2)
    print_usage(argv[0]);

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-g") == 0) {
      if (++i >= argc)
        print_usage(argv[0]);
      gen_type = argv[i];
    } else if (strcmp(argv[i], "-o") == 0) {
      if (++i >= argc)
        print_usage(argv[0]);
      output_file_name = argv[i];
    } else {
      if (input_file_name)
        print_usage(argv[0]);
      input_file_name = argv[i];
    }
  }

  if (!gen_type)
    gen_type = "mandel";
  if (!input_file_name)
    print_usage(argv[0]);

  if (!output_file_name)
    output_file_name = replace_file_ext(input_file_name, "ppm");
}

int main(int argc, char **argv) {
  parse_args(argc, argv);

  Camera camera;
  Scene *scene = scene_create_from_file(input_file_name);
  if (!scene)
    return EXIT_FAILURE;

  PixelMap *ppm = pixel_map_new(scene->pixel_width, scene->pixel_height);
  if (camera_create_from_scene(scene, &camera) != 0) {
    fprintf(stderr,
            "invalid updir/viewdir combination provided, both must be non-zero and not parallel\n");
    return EXIT_FAILURE;
  }
  scene->camera = &camera;

  for (int x = 0; x < scene->pixel_width; x++) {
    for (int y = 0; y < scene->pixel_height; y++) {
      Ray ray = camera_trace_ray(&camera, x, y);
      Intersection best_inter = scene_find_best_inter(scene, &ray);
      if (best_inter.t != INFINITY) {
        Color c = scene_shade_ray(scene, &ray, &best_inter);
        pixel_map_put(ppm, x, y, ppm_color_from_color(c));
      } else
        pixel_map_put(ppm, x, y, ppm_color_from_color(scene->bg_color));
    }
  }

  int rc = pixel_map_write_to_ppm(ppm, output_file_name);
  if (rc != 0) {
    fprintf(stderr, "failed to write ppm file to %s: %s\n",
            output_file_name, strerror(rc));
    return EXIT_FAILURE;
  }

  pixel_map_destroy(ppm);
  scene_destroy(scene);
  return 0;
}
