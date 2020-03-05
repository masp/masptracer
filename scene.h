#ifndef RAYTRACERPROJ__SCENE_DESC_H
#define RAYTRACERPROJ__SCENE_DESC_H

#include "vec.h"
#include <stdint.h>
#include <stddef.h>

#define CLAMP(x) ((x) < 0 ? 0 : ((x) > 1 ? 1 : (x)))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef Vec3 Color;
static inline Vec3 clamp(Vec3 v) {
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

typedef struct Triangle {
  int p[3];
  int n[3];
  int t[3];
  Material *mat;
} Triangle;

typedef enum {
  OBJECT_SPHERE,
  OBJECT_CYLINDER,
  OBJECT_TRIANGLE
} ObjectType;

typedef struct Object {
  union {
    Sphere sphere;
    Cylinder cyl;
    Triangle tri;
  };
  ObjectType type;
} Object;

typedef struct Intersection {
  Vec3 pos;
  Vec3 norm;
  Material *mat;
  Object *obj;

  double t; // parameter along ray where intersection occurred (used for distance calc)
} Intersection;

typedef struct Light {
  Vec3 pos;
  int w; // directional or point (w = 0/directional, w = 1/point)
  Color color;
  int is_attenuated;
  Vec3 att;
} Light;

typedef struct DepthCue {
  Vec3 color;
  double a_min, a_max;
  double dist_min, dist_max;
} DepthCue;

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

  Vec3 *vertices;
  size_t vert_cap;
  size_t vert_len;

  Vec3 *normals;
  size_t norm_cap;
  size_t norm_len;

  Vec2 *texs;
  size_t texs_cap;
  size_t texs_len;

  int depth_cueing_enabled;
  DepthCue depth_cueing;
} Scene;

Material *scene_add_material(Scene *scene);
Object *scene_add_object(Scene *scene);
Light *scene_add_light(Scene *scene);
Vec3 *scene_add_vertex(Scene *scene);
Vec3 *scene_add_norm(Scene *scene);
Vec2 *scene_add_tex(Scene *scene);

int ray_intersects_object(Scene *scene, Ray *ray, Object *obj, Intersection *out);
int ray_intersects_sphere(Scene *scene, Ray *ray, Sphere *sphere, Intersection *out);
int ray_intersects_cylinder(Scene *scene, Ray *ray, Cylinder *sphere, Intersection *out);
int ray_intersects_triangle(Scene  *scene, Ray *ray, Triangle *tri, Intersection *out);

Intersection scene_find_best_inter(Scene *scene, Ray *ray);
Color scene_shade_ray(Scene *scene, Ray *ray, Intersection *in);

void scene_destroy(Scene *s);

#endif // RAYTRACERPROJ__SCENE_DESC_H
