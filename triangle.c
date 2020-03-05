#include <math.h>
#include "scene.h"

struct Tri {
  Vec3 p0, p1, p2;
  double alph, beta, gamm;
};

static int intersects_tri_area_method(Vec3 p, struct Tri *tri, Vec3 e1, Vec3 e2) {
  // Determine if the ray at point t intersects the triangle using area
  // of subtriangles to determine the barycentric coordinates.
  // If alph + beta + gamm = 1 and  0 < alph, beta, gamm < 1
  Vec3 e3 = vecsub(tri->p1, p);
  Vec3 e4 = vecsub(tri->p2, p);

  double A = veclen(cross(e1, e2));
  double a = veclen(cross(e3, e4));
  tri->alph = a / A;
  double b = veclen(cross(e4, e2));
  tri->beta = b / A;
  double c = veclen(cross(e1, e3));
  tri->gamm = c / A;

  return fabs(1 - (tri->alph + tri->beta + tri->gamm)) < 0.001
         && tri->alph >= 0 && tri->alph <= 1
         && tri->beta >= 0 && tri->beta <= 1
         && tri->gamm >= 0 && tri->gamm <= 1;
}

int ray_intersects_triangle(Scene *scene, Ray *ray, Triangle *tri_def,
                            Intersection *out) {
  struct Tri tri;
  tri.p0 = scene->vertices[tri_def->p[0]];
  tri.p1 = scene->vertices[tri_def->p[1]];
  tri.p2 = scene->vertices[tri_def->p[2]];

  Vec3 e1 = vecsub(tri.p1, tri.p0);
  Vec3 e2 = vecsub(tri.p2, tri.p0);
  Vec3 n = cross(e1, e2);
  double D = -(dot(n, tri.p0));

  // Do plane intersection, making sure denominator is positive
  double denom = dot(n, ray->dir);
  if (denom == 0)
    return 0;
  double nom = -(dot(n, ray->pos) + D);
  double t = nom / denom;
  if (t <= 0)
    return 0;

  if (intersects_tri_area_method(ray_pos(ray, t), &tri, e1, e2))
  {
    if (tri_def->n[0] > 0)
    {
      out->norm = vecmul(scene->normals[tri_def->n[0]], tri.alph);
      out->norm = vecadd(out->norm, vecmul(scene->normals[tri_def->n[1]], tri.beta));
      out->norm = vecadd(out->norm, vecmul(scene->normals[tri_def->n[2]], tri.gamm));
      out->norm = norm(out->norm);
    }
    else {
      out->norm = norm(n);
    }
    if (tri_def->t[0] >= 0)
    {
      Vec2 t0 = scene->texs[tri_def->t[0]];
      Vec2 t1 = scene->texs[tri_def->t[1]];
      Vec2 t2 = scene->texs[tri_def->t[2]];
      out->tex_coords.x += tri.alph * t0.x + tri.beta * t1.x + tri.gamm * t2.x;
      out->tex_coords.y += tri.alph * t0.y + tri.beta * t1.y + tri.gamm * t2.y;
      out->has_tex_coords = 1;
    }
    out->pos = ray_pos(ray, t);
    out->t = t;
    out->mat = tri_def->mat;
    return 1;
  }
  return 0;
}

