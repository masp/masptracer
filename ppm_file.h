//
// Created by masp on 1/30/20.
//

#ifndef RAYTRACERPROJ__PPM_FILE_H
#define RAYTRACERPROJ__PPM_FILE_H

#include <stdint.h>
#include "scene.h"

typedef struct PpmColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} PpmColor;

PpmColor rgb_black();
PpmColor rgb_white();
PpmColor ppm_color_from_mat(Material *mat);

typedef struct PpmFile {
  int width;
  int height;
  PpmColor *data; // two-dimensional array sized by width * height, indexed by data[x][y]
} PixelMap;

PixelMap *pixel_map_new(int width, int height);
void pixel_map_destroy(PixelMap *f);

// Writes value to PixelMap
// Returns 1 if success, 0 if invalid x y
int pixel_map_put(PixelMap *this, int x, int y, PpmColor value);

// Returns value located at x y pixel in PixelMap, 0 if invalid x y
PpmColor pixel_map_get(PixelMap *this, int x, int y);

// Writes the stored memory to file specified by output_filename following the P3 format
int pixel_map_write_to_ppm(PixelMap *this, const char *output_filename);

#endif // RAYTRACERPROJ__PPM_FILE_H
