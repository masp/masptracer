#include "scene.h"
#include "math.h"

static Vec3 cyl_top(Cylinder *cyl) {
  return vecadd(cyl->center, vecmul(cyl->dir, cyl->height));
}

static int in_cylinder(Cylinder *cyl, Vec3 p) {
  return dot(cyl->dir, vecsub(p, cyl->center)) > 0 &&
         dot(cyl->dir, vecsub(p, cyl_top(cyl))) < 0;
}

static int cylinder_cap_inter(Cylinder *cyl, Ray *ray, int is_top,
                              Intersection *in) {
  Vec3 center = cyl->center;
  Vec3 dir = cyl->dir;
  // Cap intersection is cyl->dir dot (p - cyl->center) = 0 (the point is on the
  // same plane as the cap) and that distance is within the radius of the
  // cylinder's cap
  if (is_top) {
    center = cyl_top(cyl);
    dir = vecinv(dir);
  }

  // Cap-ray intersection
  // Form a plane from the normal vector (dir) and point (center) and find the
  // ray intersection of that plane A point intersection of a plane is A(x - x0)
  // + B(y - y0) + C(z - z0) = 0, where N = <A, B, C> = N dot (ray->pos +
  // ray->dir * t - center) = 0 = cyl->dir dot (ray->pos - center + t *
  // ray->dir) = 0 = (cyl->dir dot (ray->pos - center)) + t (cyl->dir dot
  // ray->dir) = 0
  //
  float a1 = -dot(dir, vecsub(ray->pos, center));
  float t = a1 / dot(dir, ray->dir);
  if (t < 0)
    return 0;
  float dist = veclen2(vecsub(ray_pos(ray, t), center));
  if (dist > cyl->radius * cyl->radius)
    return 0;

  if (t < in->t || in->t < 0) {
    in->mat = cyl->color;
    in->pos = ray_pos(ray, t);
    in->t = t;
    in->norm = vecinv(dir);
  }
  return 1;
}


int ray_intersects_cylinder(Scene *scene, Ray *ray, Cylinder *cyl, Intersection *out) {
  // We detect collsion with a cylinder with a few steps following the math
  // described here (https://mrl.nyu.edu/~dzorin/rend05/lecture2.pdf):
  // - Collision with an infinite cylinder, where cyl->center is aligned to the
  // origin with cyl->dir representing +z

  // To solve if a point P is within an arbitrary cylinder, you can find where:
  // (P - cyl->center) - ((P - cyl->center) dot cyl->dir)cyl->dir <=
  // cyl->radius^2
  //       1                               2
  // First (1) we map P to the cylinder's frame of reference. We remove the
  // component of P that is in the direction of cyl->dir (2), so that we have a
  // orthogonal vector to cyl->dir that we can compare against the radius. If
  // it's less than radius, it's within the infinite cylinder.

  // Below we use the notation "flat" to describe a vector that has had the its
  // component aligned to cyl->dir removed (subtracted)
  Vec3 dp = vecsub(ray->pos, cyl->center);
  Vec3 dp_flat = vecsub(dp, vecmul(cyl->dir, dot(dp, cyl->dir)));
  Vec3 ray_flat = vecsub(ray->dir, vecmul(cyl->dir, dot(ray->dir, cyl->dir)));

  float A = veclen2(ray_flat);
  float B = 2 * dot(dp_flat, ray_flat);
  float C = veclen2(dp_flat) - cyl->radius * cyl->radius;

  float discriminant = B * B - 4 * A * C;
  if (discriminant < 0)
    return 0; // only evaluate position if collision happens

  float t_cyl1 = (-B - sqrt(discriminant)) / (2 * A);
  float t_cyl2 = (-B + sqrt(discriminant)) / (2 * A);
  if (t_cyl1 < 0 || !in_cylinder(cyl, ray_pos(ray, t_cyl1)))
    t_cyl1 = INFINITY;
  if (t_cyl2 < 0 || !in_cylinder(cyl, ray_pos(ray, t_cyl2)))
    t_cyl2 = INFINITY;

  float best_t = MIN(t_cyl1, t_cyl2);
  if (best_t > 0) {
    out->pos = ray_pos(ray, best_t);
    Vec3 dist_from_center = vecsub(out->pos, cyl->center);
    Vec3 vert_comp = vecmul(cyl->dir, dot(dist_from_center, cyl->dir));
    Vec3 axis_pos = vecadd(cyl->center, vert_comp);
    out->norm = norm(vecsub(out->pos, axis_pos));
    out->mat = cyl->color;
    out->t = best_t;
  }

  // Find intersection of caps
  cylinder_cap_inter(cyl, ray, 0, out);
  cylinder_cap_inter(cyl, ray, 1, out);

  return out->t > 0;
}


