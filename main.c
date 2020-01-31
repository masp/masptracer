
#include "ppm_file.h"
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

static void generate_gradient(PixelMap *ppm) {
  for (int x = 0; x < ppm->width; x++) {
    for (int y = 0; y < ppm->height; y++) {
      RgbColor c = {.r = x % 255, .g = y % 255, .b = x % 255};
      pixel_map_put(ppm, x, y, c);
    }
  }
}

/* Algorithm adapted from
 * https://rosettacode.org/wiki/Mandelbrot_set#PPM_non_interactive */
static void generate_mandelbrot(PixelMap *ppm) {
  const int iteration_max = 200;
  const double escape_radius = 2;
  const double er2 = escape_radius * escape_radius;

  const double cx_min = -2.5;
  const double cx_max = 1.5;
  const double cy_min = -2.0;
  const double cy_max = 2.0;

  const double pixel_width = (cx_max - cx_min) / ppm->width;
  const double pixel_height = (cy_max - cy_min) / ppm->height;

  for (int y = 0; y < ppm->width; y++) {
    double cy = cy_min + y * pixel_height;
    if (fabs(cy) < pixel_height / 2)
      cy = 0.0; /* Main antenna */
    for (int x = 0; x < ppm->width; x++) {
      double cx = cx_min + x * pixel_width;
      /* initial value of orbit = critical point Z= 0 */
      double zx = 0.0;
      double zy = 0.0;
      double zx2 = zx * zx;
      double zy2 = zy * zy;
      int i;
      for (i = 0; i < iteration_max && ((zx2 + zy2) < er2); i++) {
        zy = 2 * zx * zy + cy;
        zx = zx2 - zy2 + cx;
        zx2 = zx * zx;
        zy2 = zy * zy;
      }

      if (i == iteration_max) { /*  interior of Mandelbrot set = black */
        pixel_map_put(ppm, x, y, rgb_black());
      } else { /* exterior of Mandelbrot set = red */
        RgbColor red = {255, 0, 0};
        pixel_map_put(ppm, x, y, red);
      }
    }
  }
}

int main(int argc, char **argv) {
  parse_args(argc, argv);

  FILE *input_file = fopen(input_file_name, "r");
  if (!input_file) {
    fprintf(stderr, "failed to open input file: %s\n",
            strerror(errno));
    return EXIT_FAILURE;
  }

  int width;
  int height;
  int read = fscanf(input_file, "imsize %d %d", &width, &height);
  int rc = fclose(input_file);
  if (rc != 0 || read != 2) {
    fprintf(stderr,
            "invalid input file: format must be \"imsize [width] [height]\"\n");
    return EXIT_FAILURE;
  }

  if (width <= 0 || height <= 0)
  {
    fprintf(stderr, "invalid input file: width and height must be positive non-zero integers (given %d x %d)\n", width, height);
    return EXIT_FAILURE;
  }

  if (!output_file_name)
    output_file_name = replace_file_ext(input_file_name, "ppm");
  printf("generating %s (%d x %d) to file %s\n", gen_type, width, height,
         output_file_name);

  PixelMap *ppm = pixel_map_new(width, height);
  if (strcmp(gen_type, "gradient") == 0)
    generate_gradient(ppm);
  else if (strcmp(gen_type, "mandel") == 0)
    generate_mandelbrot(ppm);
  else
    print_usage(argv[0]);

  rc = pixel_map_write_to_ppm(ppm, output_file_name);
  if (rc != 0) {
    fprintf(stderr, "failed to write ppm file to %s: %s\n",
            output_file_name, strerror(rc));
    return EXIT_FAILURE;
  }

  pixel_map_destroy(ppm);
  return 0;
}
