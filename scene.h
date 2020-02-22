#ifndef RAYTRACERPROJ__SCENE_DESC_H
#define RAYTRACERPROJ__SCENE_DESC_H

#include "vec.h"
#include <stdint.h>
#include <stddef.h>

#define CLAMP(x) ((x) < 0 ? 0 : ((x) > 1 ? 1 : (x)))

typedef Vec3 Color;
inline Vec3 clamp(Vec3 v) {
  Vec3 result;
  result.x = CLAMP(v.x);
  result.y = CLAMP(v.y);
  result.z = CLAMP(v.z);
  return result;
}

typedef struct Material {
  Color diffuse_color;
  Color spec_color;
  double ka, kd, ks;
  int n;
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

typedef struct Cylinder {
  Vec3 center;
  Vec3 dir;
  double radius;
  double height;
  Material *color;
} Cylinder;

typedef enum {
  OBJECT_SPHERE,
  OBJECT_CYLINDER,
} ObjectType;

typedef struct Object {
  union {
    Sphere sphere;
    Cylinder cyl;
  };
  ObjectType type;
} Object;

typedef struct Light {
  Vec3 pos;
  int w; // directional or point (w = 0/directional, w = 1/point)
  Color color;
  int is_attenuated;
  Vec3 att;
} Light;

int ray_intersects_object(Ray *ray, Object *obj, Intersection *out);

typedef struct Scene {
  Vec3 eye;
  Vec3 viewdir;
  Vec3 updir;
  double fov_h;
  int pixel_width, pixel_height;
  Color bg_color;
  struct Camera *camera;

  Material *palette;
  size_t palette_cap;
  size_t palette_len;

  Object *objects;
  size_t objects_cap;
  size_t objects_len;

  Light *lights;
  size_t lights_cap;
  size_t lights_len;
} Scene;

Material *scene_add_material(Scene *scene);
Object *scene_add_object(Scene *scene);
Light *scene_add_light(Scene *scene);

Intersection scene_find_best_inter(Scene *scene, Ray *ray);
Color scene_shade_ray(Scene *scene, Ray *ray, Intersection *in);

void scene_destroy(Scene *s);

#endif // RAYTRACERPROJ__SCENE_DESC_H
