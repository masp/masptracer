#include "scene.h"
#include "camera.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>

static int ray_intersects_sphere(Ray *ray, Sphere *sphere, Intersection *out) {
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
  double a1 = -dot(dir, vecsub(ray->pos, center));
  double t = a1 / dot(dir, ray->dir);
  if (t < 0)
    return 0;
  double dist = veclen2(vecsub(ray_pos(ray, t), center));
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

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

static int ray_intersects_cylinder(Ray *ray, Cylinder *cyl, Intersection *out) {
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

  double A = veclen2(ray_flat);
  double B = 2 * dot(dp_flat, ray_flat);
  double C = veclen2(dp_flat) - cyl->radius * cyl->radius;

  double discriminant = B * B - 4 * A * C;
  if (discriminant < 0)
    return 0; // only evaluate position if collision happens

  double t_cyl1 = (-B - sqrt(discriminant)) / (2 * A);
  double t_cyl2 = (-B + sqrt(discriminant)) / (2 * A);
  if (t_cyl1 < 0 || !in_cylinder(cyl, ray_pos(ray, t_cyl1)))
    t_cyl1 = INFINITY;
  if (t_cyl2 < 0 || !in_cylinder(cyl, ray_pos(ray, t_cyl2)))
    t_cyl2 = INFINITY;

  double best_t = MIN(t_cyl1, t_cyl2);
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

int ray_intersects_object(Ray *ray, Object *obj, Intersection *out) {
  switch (obj->type) {
  case OBJECT_SPHERE:
    return ray_intersects_sphere(ray, &obj->sphere, out);
  case OBJECT_CYLINDER:
    return ray_intersects_cylinder(ray, &obj->cyl, out);
  }
  return 0;
}

// Find the best intersection of the ray with any object in the scene
// If no intersection found, return an intersection with t of INFINITY
Intersection scene_find_best_inter_ignore(Scene *scene, Ray *ray,
                                          Object *ignore) {
  Intersection best;
  best.t = INFINITY;

  for (int oid = 0; oid < scene->objects_len; oid++) {
    Object *obj = &scene->objects[oid];
    if (ignore == obj)
      continue;

    Intersection inter = {0};
    if (ray_intersects_object(ray, obj, &inter)) {
      if (inter.t > 0.01 && inter.t < best.t) {
        best = inter;
        best.obj = obj;
      }
    }
  }
  return best;
}

Intersection scene_find_best_inter(Scene *scene, Ray *ray) {
  return scene_find_best_inter_ignore(scene, ray, NULL);
}

static Color calc_diffuse_comp(Intersection *in, Vec3 L) {
  Color result = {0};
  double factor = MAX(dot(L, in->norm), 0);
  result = vecadd(result, vecmul(in->mat->diffuse_color, in->mat->kd * factor));
  return result;
}

static Color calc_specular_comp(Scene *scene, Intersection *in, Vec3 L) {
  Color result = {0};
  Vec3 H = norm(
      vecadd(L, norm(vecsub(scene->camera->eye_pos,
                            in->pos)))); // L and viewdir are always unit length
  double factor = pow(MAX(dot(in->norm, H), 0), in->mat->n);
  result = vecadd(result, vecmul(in->mat->spec_color, in->mat->ks * factor));
  return result;
}

static Color apply_atten(Light *l, Color c, double dist2) {
  double att_factor =
      1 / (l->att.x + l->att.y * sqrt(dist2) + l->att.z * dist2);
  if (att_factor > 1)
    return c; // don't make the light brighter if it's closer!
  return vecmul(c, att_factor);
}

static Color apply_depth_cueing(Color c, DepthCue *dc, double dist) {
  double a_dc;
  if (dist <= dc->dist_min)
    a_dc = dc->a_max;
  else if (dist >= dc->dist_max)
    a_dc = dc->a_min;
  else {
    a_dc = dc->a_min +
           (dc->a_max - dc->a_min) *
               ((dc->dist_max - dist) / (dc->dist_max - dc->dist_min));
  }

  return clamp(vecadd(vecmul(c, a_dc), vecmul(dc->color, 1 - a_dc)));
}

static double find_ray_param_for_point_on_ray(Ray *r, Vec3 p_on_ray) {
  if (r->dir.x != 0)
    return (p_on_ray.x - r->pos.x) / r->dir.x;
  if (r->dir.y != 0)
    return (p_on_ray.y - r->pos.y) / r->dir.y;
  if (r->dir.x != 0)
    return (p_on_ray.z - r->pos.z) / r->dir.z;
  return 0;
}

// Return 0 if object is in a shadow, 1 otherwise
static int calc_shadow_factor(Scene *scene, int is_positional, Vec3 L_pos,
                              Vec3 L, Intersection *in) {
  Ray shadow_ray;
  shadow_ray.pos = in->pos;
  shadow_ray.dir = L;
  Intersection shadow_in =
      scene_find_best_inter_ignore(scene, &shadow_ray, in->obj);
  if (shadow_in.t == INFINITY)
    return 1;

  if (is_positional) {
    double light_t = find_ray_param_for_point_on_ray(&shadow_ray, L_pos);
    if (shadow_in.t > light_t)
      return 1;
    return 0;
  }
  return 0;
}

static double rand_sphere(double radius) {
  return (-1 + 2 * ((double)rand() / RAND_MAX)) * radius;
}

static double calc_shadow_factor_smooth(Scene *scene, Light *light,
                                        Intersection *in) {
  // The radius of the spherical light source we sample from
  double light_r = 0.5;
  const int total_iters = 100;
  int sum = 0;
  for (int i = 0; i < total_iters; i++) {
    Vec3 random_offset = {rand_sphere(light_r), rand_sphere(light_r),
                          rand_sphere(light_r)};
    // We don't do soft shadows for directional lights (what does it mean?)
    Vec3 L_pos = light->w ? vecadd(light->pos, random_offset) : light->pos;
    Vec3 L = light->w ? norm(vecsub(L_pos, in->pos)) : vecinv(L_pos);
    sum += calc_shadow_factor(scene, light->w, L_pos, L, in);
  }
  return (double)sum / total_iters;
}

Color scene_shade_ray(Scene *scene, Ray *ray, Intersection *in) {
  Color result;
  result = vecmul(in->mat->diffuse_color, in->mat->ka);

  for (int i = 0; i < scene->lights_len; i++) {
    Light *light = &scene->lights[i];
    Vec3 L = light->w ? norm(vecsub(light->pos, in->pos)) : vecinv(light->pos);

    Color non_amb_color = {0};
    non_amb_color = clamp(vecadd(non_amb_color, calc_diffuse_comp(in, L)));
    non_amb_color =
        clamp(vecadd(non_amb_color, calc_specular_comp(scene, in, L)));
    non_amb_color = elemmul(non_amb_color, light->color);

    double shadow_factor = calc_shadow_factor_smooth(scene, light, in);
    non_amb_color = vecmul(non_amb_color, shadow_factor);

    if (light->is_attenuated && veclen2(light->att) > 0)
      non_amb_color =
          apply_atten(light, non_amb_color, dist2(light->pos, in->pos));

    result = clamp(vecadd(result, non_amb_color));
  }

  if (scene->depth_cueing_enabled) {
    result = apply_depth_cueing(result, &scene->depth_cueing,
                                sqrt(dist2(in->pos, scene->camera->eye_pos)));
  }

  return result;
}