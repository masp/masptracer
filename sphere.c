#include "scene.h"
#include <math.h>

int ray_intersects_sphere(Scene *scene, Ray *ray, Sphere *sphere, Intersection *out) {
  float A = 1;
  float B = 2 * ((ray->dir.x * (ray->pos.x - sphere->center.x)) +
                  (ray->dir.y * (ray->pos.y - sphere->center.y)) +
                  (ray->dir.z * (ray->pos.z - sphere->center.z)));
  float C = (ray->pos.x - sphere->center.x) * (ray->pos.x - sphere->center.x) +
             (ray->pos.y - sphere->center.y) * (ray->pos.y - sphere->center.y) +
             (ray->pos.z - sphere->center.z) * (ray->pos.z - sphere->center.z) -
             (sphere->radius * sphere->radius);

  float discriminant = B * B - 4 * A * C;
  if (discriminant < 0)
    return 0; // only evaluate position if collision happens

  float t_smaller = (-B - sqrt(discriminant)) / (2 * A);
  float t_bigger = (-B + sqrt(discriminant)) / (2 * A);
  float t = t_smaller;
  if (t_smaller <= 0 && t_bigger > 0)
    t = t_bigger;

  if (t <= 0)
    return 0;

  out->pos = ray_pos(ray, t);
  out->norm = norm(vecsub(out->pos, sphere->center));
  out->mat = sphere->color;
  out->t = t;
  return 1;
}

