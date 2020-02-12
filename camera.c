#define _USE_MATH_DEFINES
#include "camera.h"
#include <math.h>

static double rads(double deg) { return deg * (M_PI / 180); }

int camera_create_from_scene(Scene *scene, Camera *out) {
  Camera new_camera;
  new_camera.eye_pos = scene->eye;
  new_camera.viewdir = norm(scene->viewdir);
  new_camera.up = norm(scene->updir);

  // We check if the two vectors are approximately identical (the cross product
  // will yield close to zero).
  //  If they are, we need to notify the user that it's an invalid updir for the
  //  viewdir provided.
  if (1 - fabs(dot(new_camera.viewdir, new_camera.up)) < 0.01)
	return 1;

  // We don't support zero directions
  if (veclen2(new_camera.viewdir) == 0 || veclen2(new_camera.up) == 0)
	return 1;

  new_camera.u = norm(cross(new_camera.viewdir, new_camera.up));
  new_camera.v = cross(
	  new_camera.u, new_camera.viewdir); // norm is unnecessary since u and
  // viewdir are orthogonal unit length

  // Calculate the viewing window dimensions
  //  assume distance from eye to viewing plane is an arbitrary 1, which
  //  simplifies expressions
  new_camera.window_width = 2 * tan(rads(scene->fov_h) / 2);
  double inv_asp_ratio =
	  (double) scene->pixel_height / (double) scene->pixel_width;
  new_camera.window_height = new_camera.window_width * inv_asp_ratio;
  new_camera.window_pixel_width = scene->pixel_width;
  new_camera.window_pixel_height = scene->pixel_height;

  Vec3 vw_center = vecadd(new_camera.eye_pos, new_camera.viewdir);
  Vec3 half_width = vecmul(new_camera.u, new_camera.window_width / 2);
  Vec3 half_height = vecmul(new_camera.v, new_camera.window_height / 2);
  new_camera.vw_ul = vecadd(vecsub(vw_center, half_width), half_height);
  new_camera.vw_ur = vecadd(vecadd(vw_center, half_width), half_height);
  new_camera.vw_ll = vecsub(vecsub(vw_center, half_width), half_height);
  new_camera.vw_lr = vecsub(vecadd(vw_center, half_width), half_height);
  if (out)
	*out = new_camera;
  return 0;
}

Ray camera_trace_ray(Camera *camera, int x, int y) {
  // The window starts from ul and goes to lr: (0, 0) is ul, (w, h) is lr
  Vec3 width_v = vecsub(camera->vw_ur, camera->vw_ul);
  Vec3 height_v = vecsub(camera->vw_ll, camera->vw_ul);
  Vec3 du = vecmul(vecdiv(width_v, camera->window_pixel_width), x);
  Vec3 dv = vecmul(vecdiv(height_v, camera->window_pixel_height), y);
  Vec3 pad_u = vecdiv(width_v, camera->window_pixel_width * 2);
  Vec3 pad_v = vecdiv(height_v, camera->window_pixel_height * 2);

  Vec3 vw_pos = vecadd(camera->vw_ul, du);
  vw_pos = vecadd(vw_pos, dv);
  vw_pos = vecadd(vw_pos, pad_u);
  vw_pos = vecadd(vw_pos, pad_v);

  return ray_from_line(camera->eye_pos, vw_pos);
}
