#include "scene.h"
#include <assert.h>
#include <math.h>

static int ray_intersects_sphere(Ray *ray, Sphere *sphere, Intersection *out) {
  double A = 1;
  double B =
	  2 * ((ray->dir.x * (ray->pos.x - sphere->center.x))
		  + (ray->dir.y * (ray->pos.y - sphere->center.y))
		  + (ray->dir.z * (ray->pos.z - sphere->center.z)));
  double C =
	  (ray->pos.x - sphere->center.x) * (ray->pos.x - sphere->center.x)
		  + (ray->pos.y - sphere->center.y) * (ray->pos.y - sphere->center.y)
		  + (ray->pos.z - sphere->center.z) * (ray->pos.z - sphere->center.z)
		  - (sphere->radius * sphere->radius);

  double discriminant = B * B - 4 * A * C;
  if (discriminant < 0)
	return 0; // only evaluate position if collision happens

  double t = (-B - sqrt(discriminant)) / (2 * A);
  // double t_bigger = (-B + sqrt(discriminant)) / (2 * A);
  // the only time we'd want t_bigger is if t was negative and t_bigger was not.
  // If t is negative, it means t_bigger is also negative unless the ray is inside
  // the sphere. We ignore this case and return no intersection if t is negative (behind eye).
  if (t <= 0)
	return 0;

  out->pos = ray_pos(ray, t);
  out->norm = vecsub(out->pos, sphere->center);
  out->mat = sphere->color;
  return 1;
}

static Vec3 cyl_top(Cylinder *cyl) {
  return vecadd(cyl->center, vecmul(cyl->dir, cyl->height));
}

static int in_cylinder(Cylinder *cyl, Vec3 p) {
  return dot(cyl->dir, vecsub(p, cyl->center)) > 0
	  && dot(cyl->dir, vecsub(p, cyl_top(cyl))) < 0;
}

static int cylinder_cap_inter(Cylinder *cyl, Ray *ray, int is_top, Intersection *in) {
  Vec3 center = cyl->center;
  Vec3 dir = cyl->dir;
  // Cap intersection is cyl->dir dot (p - cyl->center) = 0 (the point is on the same plane as the cap) and that distance is within the radius of the cylinder's cap
  if (is_top)
  {
	center = cyl_top(cyl);
	dir = vecinv(dir);
  }

  // Cap-ray intersection
  // Form a plane from the normal vector (dir) and point (center) and find the ray intersection of that plane
  // A point intersection of a plane is A(x - x0) + B(y - y0) + C(z - z0) = 0, where N = <A, B, C>
  // = N dot (ray->pos + ray->dir * t - center) = 0
  // = cyl->dir dot (ray->pos - center + t * ray->dir) = 0
  // = (cyl->dir dot (ray->pos - center)) + t (cyl->dir dot ray->dir) = 0
  //
  double a1 = -dot(dir, vecsub(ray->pos, center));
  double t = a1 / dot(dir, ray->dir);
  if (t < 0)
	return 0;
  double dist = veclen2(vecsub(ray_pos(ray, t), center));
  if (dist > cyl->radius * cyl->radius)
	return 0;

  if (t < in->t || in->t < 0)
  {
    static Material custom_color = {0.1, 1, 0.1};
	in->mat = &custom_color;
	in->pos = ray_pos(ray, t);
	in->t = t;
	in->norm = vecinv(dir);
  }
  return 1;
}

#define MAX(a, b) (a) > (b) ? (a) : (b)

static int ray_intersects_cylinder(Ray *ray, Cylinder *cyl, Intersection *out) {
  // We detect collsion with a cylinder with a few steps following the math described here (https://mrl.nyu.edu/~dzorin/rend05/lecture2.pdf):
  // - Collision with an infinite cylinder, where cyl->center is aligned to the origin with cyl->dir representing +z

  // To solve if a point P is within an arbitrary cylinder, you can find where:
  // (P - cyl->center) - ((P - cyl->center) dot cyl->dir)cyl->dir <= cyl->radius^2
  //       1                               2
  // First (1) we map P to the cylinder's frame of reference. We remove the component of P that is in the direction of cyl->dir (2),
  // so that we have a orthogonal vector to cyl->dir that we can compare against the radius. If it's less than radius, it's within the infinite cylinder.

  // Below we use the notation "flat" to describe a vector that has had the its component aligned to cyl->dir removed (subtracted)
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
	t_cyl1 = -1;
  if (t_cyl2 < 0 || !in_cylinder(cyl, ray_pos(ray, t_cyl2)))
	t_cyl2 = -1;

  double best_t = MAX(t_cyl1, t_cyl2);
  if (best_t > 0)
  {
	out->pos = ray_pos(ray, best_t);
	Vec3 dist_from_center = vecsub(out->pos, cyl->center);
	Vec3 vert_comp = vecmul(cyl->dir, dot(dist_from_center, cyl->dir));
	Vec3 axis_pos = vecadd(cyl->center, vert_comp);
	out->norm = vecsub(out->pos, axis_pos);
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
	case OBJECT_SPHERE: return ray_intersects_sphere(ray, &obj->sphere, out);
	case OBJECT_CYLINDER: return ray_intersects_cylinder(ray, &obj->cyl, out);
  }
  return 0;
}

Intersection *scene_find_best_inter(Scene *scene, Ray *ray) {
  static Intersection best;
  Intersection *ret = NULL;

  for (int oid = 0; oid < scene->objects_len; oid++) {
	Object *obj = &scene->objects[oid];
	Intersection inter;
	if (ray_intersects_object(ray, obj, &inter)) {
	  if (!ret || dist2(inter.pos, ray->pos) < dist2(best.pos, ray->pos)) {
		best = inter;
		ret = &best;
	  }
	}
  }
  return ret;
}