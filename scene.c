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

  out->pos = ray_pos_along_line(ray, t);
  out->norm = vecsub(out->pos, sphere->center);
  out->mat = sphere->color;
  return 1;
}

static int ray_intersects_cylinder(Ray *ray, Cylinder *cyl, Intersection *out) {
	// First, we find the distance that the ray shoots from the center of the cylinder (test body) and then we test height.

	// We do this by finding a frame orientated according to the cylinder (u_c and v_c)
	// u_c points orthogonal to cyl->dir and the ray
	Vec3 u_c = norm(cross(cyl->dir, ray->dir));
	Vec3 u_dist = vecmul(u_c, dot(vecsub(cyl->center, ray->pos), u_c)); // u_dist is a vector connecting a point on cyl->dir to ray->dir
	if (veclen2(u_dist) > cyl->radius * cyl->radius)
	  return 0; // If the distance is farther than the radius of the infinite cylinder, no collision possible

	
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
	  if (!ret || dist2(inter.pos, ray->pos) < dist2(best.pos, ray->pos))
	  {
		best = inter;
		ret = &best;
	  }
	}
  }
  return ret;
}