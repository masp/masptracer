#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "scene.h"

typedef struct Camera {
  Vec3 eye_pos;
  Vec3 u;
  Vec3 v;
  Vec3 viewdir;
  Vec3 up;

  int window_pixel_width, window_pixel_height; // pixel dims are stored for camera_trace_ray
  double window_width, window_height;
  // The bounds in world coordinates of the viewing window
  Vec3 vw_ul, vw_ur, vw_ll, vw_lr;

} Camera;

/**
 * Calculates a camera frame from a scene description (eye, viewdir, and updir)
 *
 * @param scene The scene to build the camera from
 * @param out The camera that will be initialized from the scene
 * @return 0 if successful, 1 if the scene has an invalid combination of updir and viewdir/eye
 */
int camera_create_from_scene(Scene *scene, Camera *out);

/**
 * Creates a new ray that goes from the eye through the pixel represented by x and y
 *
 * @return A new ray that has position "eye_pos" and runs through the pixel represented by (x, y).
 *          If x and y are larger than the pixel bounds of the image, the ray will still be valid but
 *          point to an imaginary pixel outside the bounds of the viewing window.
 */
Ray camera_trace_ray(Camera *camera, int x, int y);

#endif //_CAMERA_H_
