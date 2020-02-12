#ifndef RAYTRACERPROJ__SCENE_DESC_H
#define RAYTRACERPROJ__SCENE_DESC_H

#include <stdint.h>
#include <stddef.h>

typedef struct Color {
  double r, g, b;
} Color;

typedef struct Vec2 {
  double x, y;
} Vec2;

typedef struct Vec3 {
  double x, y, z;
} Vec3;

typedef struct Sphere {
  Vec3 center;
  double radius;
  Color color;
} Sphere;

typedef enum {
  OBJECT_SPHERE,
  OBJECT_CYLINDER,
} ObjectType;

typedef struct Cylinder {
  Vec3 base_center;
  double radius;
  double height;
} Cylinder;

typedef struct Object {
  union {
    Sphere sphere;
    Cylinder cyl;
  };
  ObjectType type;
} Object;

typedef struct Scene {
  Vec3 eye;
  Vec3 viewdir;
  Vec3 updir;
  double fov_h;
  int pixel_width, pixel_height;
  Color bg_color;

  Object *objects;
  size_t objects_cap;
  size_t objects_len;
} Scene;

Scene *scene_create_from_file(const char *scene_desc_file_path);
Object *scene_add_object(Scene *scene);
void scene_destroy(Scene *s);

#endif // RAYTRACERPROJ__SCENE_DESC_H
