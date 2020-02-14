//
// Created by masp on 2/12/2020.
//

#ifndef _VEC_H_
#define _VEC_H_

typedef struct Vec2 {
  double x, y;
} Vec2;

typedef struct Vec3 {
  double x, y, z;
} Vec3;

double dot(Vec3 l, Vec3 r);
Vec3 cross(Vec3 l, Vec3 r);
double veclen(Vec3 v);
double veclen2(Vec3 v);
Vec3 norm(Vec3 v);
double dist2(Vec3 start, Vec3 end);

Vec3 vecadd(Vec3 l, Vec3 r);
Vec3 vecsub(Vec3 l, Vec3 r);
Vec3 vecmul(Vec3 v, double s);
Vec3 vecdiv(Vec3 v, double s);
Vec3 vecinv(Vec3 v);

typedef struct Ray {
  Vec3 pos;
  Vec3 dir; // unit length
} Ray;

Ray ray_from_line(Vec3 p0, Vec3 p1);
Vec3 ray_pos(Ray *ray, double t);


#endif //_VEC_H_
