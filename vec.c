#include "vec.h"
#include <math.h>

double dot(Vec3 l, Vec3 r) {
  return l.x * r.x + l.y * r.y + l.z * r.z;
}

Vec3 cross(Vec3 l, Vec3 r) {
  Vec3 result;
  result.x = l.y * r.z - l.z * r.y;
  result.y = l.z * r.x - l.x * r.z;
  result.z = l.x * r.y - l.y * r.x;
  return result;
}

double veclen(Vec3 v) {
  return sqrt(veclen2(v));
}

double veclen2(Vec3 v) { return dot(v, v); }

double dist2(Vec3 start, Vec3 end) {
  return veclen2(vecsub(end, start));
}

Vec3 norm(Vec3 v) {
  Vec3 result;
  double mag = veclen(v);
  result.x = v.x / mag;
  result.y = v.y / mag;
  result.z = v.z / mag;
  return result;
}

Vec3 vecinv(Vec3 v) {
  Vec3 result;
  result.x = -v.x;
  result.y = -v.y;
  result.z = -v.z;
  return result;
}

Vec3 vecadd(Vec3 l, Vec3 r) {
  Vec3 result;
  result.x = l.x + r.x;
  result.y = l.y + r.y;
  result.z = l.z + r.z;
  return result;
}

Vec3 vecsub(Vec3 l, Vec3 r) {
  Vec3 result;
  result.x = l.x - r.x;
  result.y = l.y - r.y;
  result.z = l.z - r.z;
  return result;
}
Vec3 vecmul(Vec3 v, double s) {
  Vec3 result = v;
  result.x *= s;
  result.y *= s;
  result.z *= s;
  return result;
}

Vec3 vecdiv(Vec3 v, double s) {
  Vec3 result = v;
  result.x /= s;
  result.y /= s;
  result.z /= s;
  return result;
}

Ray ray_from_line(Vec3 p0, Vec3 p1) {
  Ray result;
  result.pos = p0;
  result.dir = norm(vecsub(p1, p0));
  return result;
}

Vec3 ray_pos(Ray *ray, double t) {
  Vec3 result;
  result.x = ray->pos.x + ray->dir.x * t;
  result.y = ray->pos.y + ray->dir.y * t;
  result.z = ray->pos.z + ray->dir.z * t;
  return result;
}
