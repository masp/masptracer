//
// Created by masp on 2/12/2020.
//

#ifndef _VEC_H_
#define _VEC_H_

typedef struct Vec2 {
  float x, y;
} Vec2;

typedef struct Vec3 {
  float x, y, z;
} Vec3;

float dot(Vec3 l, Vec3 r);
Vec3 cross(Vec3 l, Vec3 r);
float veclen(Vec3 v);
float veclen2(Vec3 v);
Vec3 norm(Vec3 v);
float dist2(Vec3 start, Vec3 end);

Vec3 vecadd(Vec3 l, Vec3 r);
Vec3 vecsub(Vec3 l, Vec3 r);
Vec3 vecmul(Vec3 v, float s);
Vec3 vecdiv(Vec3 v, float s);
Vec3 vecinv(Vec3 v);

Vec3 elemmul(Vec3 l, Vec3 r);

typedef struct Ray {
  Vec3 pos;
  Vec3 dir; // unit length
} Ray;

Ray ray_from_line(Vec3 p0, Vec3 p1);
Vec3 ray_pos(Ray *ray, float t);


#endif //_VEC_H_
