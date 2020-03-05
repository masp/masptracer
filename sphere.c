#include "scene.h"
#include <math.h>

int ray_intersects_sphere(Scene *scene, Ray *ray, Sphere *sphere, Intersection *out) {
  double A = 1;
  double B = 2 * ((ray->dir.x * (ray->pos.x - sphere->center.x)) +
                  (ray->dir.y * (ray->pos.y - sphere->center.y)) +
                  (ray->dir.z * (ray->pos.z - sphere->center.z)));
  double C = (ray->pos.x - sphere->center.x) * (ray->pos.x - sphere->center.x) +
             (ray->pos.y - sphere->center.y) * (ray->pos.y - sphere->center.y) +
             (ray->pos.z - sphere->center.z) * (ray->pos.z - sphere->center.z) -
             (sphere->radius * sphere->radius);

  double discriminant = B * B - 4 * A * C;
  if (discriminant < 0)
    return 0; // only evaluate position if collision happens

  double t = (-B - sqrt(discriminant)) / (2 * A);
  // double t_bigger = (-B + sqrt(discriminant)) / (2 * A);
  // the only time we'd want t_bigger is if t was negative and t_bigger was not.
  // If t is negative, it means t_bigger is also negative unless the ray is
  // inside the sphere. We ignore this case and return no intersection if t is
  // negative (behind eye).
  if (t <= 0)
    return 0;

  out->pos = ray_pos(ray, t);
  out->norm = norm(vecsub(out->pos, sphere->center));
  out->mat = sphere->color;
  out->t = t;
  return 1;
}

