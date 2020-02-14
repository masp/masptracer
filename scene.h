#ifndef RAYTRACERPROJ__SCENE_DESC_H
#define RAYTRACERPROJ__SCENE_DESC_H

#include "vec.h"
#include <stdint.h>
#include <stddef.h>

typedef struct Material {
  double r, g, b;
} Material;

typedef struct Intersection {
  Vec3 pos;
  Vec3 norm;
  Material *mat;

  double t; // parameter along ray where intersection occurred (used for distance calc)
} Intersection;

typedef struct Sphere {
  Vec3 center;
  double radius;
  Material *color;
} Sphere;

typedef enum {
  OBJECT_SPHERE,
  OBJECT_CYLINDER,
} ObjectType;

typedef struct Cylinder {
  Vec3 center;
  Vec3 dir;
  double radius;
  double height;
  Material *color;
} Cylinder;

typedef struct Object {
  union {
    Sphere sphere;
    Cylinder cyl;
  };
  ObjectType type;
} Object;

int ray_intersects_object(Ray *ray, Object *obj, Intersection *out);

typedef struct Scene {
  Vec3 eye;
  Vec3 viewdir;
  Vec3 updir;
  double fov_h;
  int pixel_width, pixel_height;
  Material bg_color;

  Material *palette;
  size_t palette_cap;
  size_t palette_len;

  Object *objects;
  size_t objects_cap;
  size_t objects_len;
} Scene;

Material *scene_add_material(Scene *scene);
Object *scene_add_object(Scene *scene);

Intersection *scene_find_best_inter(Scene *scene, Ray *ray);

void scene_destroy(Scene *s);

#endif // RAYTRACERPROJ__SCENE_DESC_H
