#include "scene.h"
#include "camera.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>

int ray_intersects_object(Scene *scene, Ray *ray, Object *obj, Intersection *out) {
  switch (obj->type) {
    case OBJECT_SPHERE:
      return ray_intersects_sphere(scene, ray, &obj->sphere, out);
    case OBJECT_CYLINDER:
      return ray_intersects_cylinder(scene, ray, &obj->cyl, out);
    case OBJECT_TRIANGLE:
      return ray_intersects_triangle(scene, ray, &obj->tri, out);
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
    if (ray_intersects_object(scene, ray, obj, &inter)) {
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
  return (-1 + 2 * ((double) rand() / RAND_MAX)) * radius;
}

static double calc_shadow_factor_smooth(Scene *scene, Light *light,
                                        Intersection *in) {
  // The radius of the spherical light source we sample from
  double light_r = 0.5;
  const int total_iters = 1;
  int sum = 0;
  if (total_iters > 1)
  {
    for (int i = 0; i < total_iters; i++) {
      Vec3 random_offset = {rand_sphere(light_r), rand_sphere(light_r),
                            rand_sphere(light_r)};
      // We don't do soft shadows for directional lights (what does it mean?)
      Vec3 L_pos = light->w ? vecadd(light->pos, random_offset) : light->pos;
      Vec3 L = light->w ? norm(vecsub(L_pos, in->pos)) : vecinv(L_pos);
      sum += calc_shadow_factor(scene, light->w, L_pos, L, in);
    }
  }
  else {
    Vec3 L = light->w ? norm(vecsub(light->pos, in->pos)) : vecinv(light->pos);
    sum = calc_shadow_factor(scene, light->w, light->pos, L, in);
  }
  return (double) sum / total_iters;
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
