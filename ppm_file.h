//
// Created by masp on 1/30/20.
//

#ifndef RAYTRACERPROJ__PPM_FILE_H
#define RAYTRACERPROJ__PPM_FILE_H

#include <stdint.h>

typedef struct RgbColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} RgbColor;

RgbColor rgb_black();
RgbColor rgb_white();

typedef struct PpmFile {
  int width;
  int height;
  RgbColor *data; // two-dimensional array sized by width * height, indexed by data[x][y]
} PixelMap;

PixelMap *pixel_map_new(int width, int height);
void pixel_map_destroy(PixelMap *f);

// Writes value to PixelMap
// Returns 1 if success, 0 if invalid x y
int pixel_map_put(PixelMap *this, int x, int y, RgbColor value);

// Returns value located at x y pixel in PixelMap, 0 if invalid x y
RgbColor pixel_map_get(PixelMap *this, int x, int y);

// Writes the stored memory to file specified by output_filename following the P3 format
int pixel_map_write_to_ppm(PixelMap *this, const char *output_filename);

#endif // RAYTRACERPROJ__PPM_FILE_H
