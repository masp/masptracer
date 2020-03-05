#include "scene_config.h"
#include "scene.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum ParseLineResult {
  LINE_OK,
  UNRECOGNIZED_TAG,
  INVALID_FORMAT
} ParseLineResult;

static int isend(char c) { return (c == '\0' || c == '\n' || c == '\r'); }

static ParseLineResult read_vec3(const char *body, Vec3 *out) {
  int end;
  int rc = sscanf(body, "%lf %lf %lf%n", &out->x, &out->y, &out->z, &end);
  return rc == 3 && isend(body[end]) ? LINE_OK : INVALID_FORMAT;
}

static ParseLineResult read_color(const char *body, Color *out) {
  int end;
  int rc = sscanf(body, "%lf %lf %lf%n", &out->x, &out->y, &out->z, &end);
  return rc == 3 && isend(body[end]) ? LINE_OK : INVALID_FORMAT;
}

static int read_mat(const char *body, Material *out) {
  Material c = {0};
  int end;
  int rc = sscanf(body, "%lf %lf %lf %lf %lf %lf %lf %lf %lf %d%n",
                  &c.diffuse_color.x, &c.diffuse_color.y, &c.diffuse_color.z,
                  &c.spec_color.x, &c.spec_color.y, &c.spec_color.z, &c.ka,
                  &c.kd, &c.ks, &c.n, &end);
  if (rc == 10 && isend(body[end])) {
    *out = c;
    return LINE_OK;
  }
  return INVALID_FORMAT;
}

static int read_light(Scene *scene, const char *body) {
  Light light = {0};
  int end;
  int rc = sscanf(body, "%lf %lf %lf %d %lf %lf %lf%n\n", &light.pos.x,
                  &light.pos.y, &light.pos.z, &light.w, &light.color.x,
                  &light.color.y, &light.color.z, &end);

  if (rc != 7 || !isend(body[end]))
    return INVALID_FORMAT;

  Light *new_light = scene_add_light(scene);
  *new_light = light;
  if (!new_light->w)
    new_light->pos = norm(new_light->pos);
  return LINE_OK;
}

static int read_att_light(Scene *scene, const char *body) {
  Light light = {0};
  int end;
  int rc = sscanf(body, "%lf %lf %lf %d %lf %lf %lf %lf %lf %lf%n\n",
                  &light.pos.x, &light.pos.y, &light.pos.z, &light.w,
                  &light.color.x, &light.color.y, &light.color.z, &light.att.x,
                  &light.att.y, &light.att.z, &end);

  if (rc != 10 || !isend(body[end]))
    return INVALID_FORMAT;

  Light *new_light = scene_add_light(scene);
  *new_light = light;
  new_light->is_attenuated = 1;
  if (!new_light->w)
    new_light->pos = norm(new_light->pos);
  return LINE_OK;
}

static int read_sphere(Scene *scene, const char *body, Material *curr_color) {
  Vec3 center;
  double radius;
  int end;
  int rc = sscanf(body, "%lf %lf %lf %lf%n\n", &center.x, &center.y, &center.z,
                  &radius, &end);

  if (rc != 4 || !isend(body[end]))
    return INVALID_FORMAT;

  Object *new_obj = scene_add_object(scene);
  new_obj->type = OBJECT_SPHERE;
  Sphere *new_sphere = &new_obj->sphere;
  new_sphere->center = center;
  new_sphere->radius = radius;
  new_sphere->color = curr_color;
  return LINE_OK;
}

static int read_cylinder(Scene *scene, const char *body, Material *curr_color) {
  Vec3 center;
  Vec3 dir;
  double radius;
  double length;

  int end;
  int rc =
      sscanf(body, "%lf %lf %lf %lf %lf %lf %lf %lf%n", &center.x, &center.y,
             &center.z, &dir.x, &dir.y, &dir.z, &radius, &length, &end);
  if (rc != 8 || !isend(body[end]))
    return INVALID_FORMAT;

  Object *new_obj = scene_add_object(scene);
  new_obj->type = OBJECT_CYLINDER;
  Cylinder *new_cyl = &new_obj->cyl;
  new_cyl->center = center;
  new_cyl->dir = norm(dir);
  new_cyl->radius = radius;
  new_cyl->height = length;
  new_cyl->color = curr_color;
  return LINE_OK;
}

static int read_depth_cue(const char *body, DepthCue *out) {
  DepthCue cue;
  int end;
  int rc = sscanf(body, "%lf %lf %lf %lf %lf %lf %lf%n", &cue.color.x,
                  &cue.color.y, &cue.color.z, &cue.a_max, &cue.a_min,
                  &cue.dist_max, &cue.dist_min, &end);
  if (rc != 7 || !isend(body[end]))
    return INVALID_FORMAT;

  if (out)
    *out = cue;
  return LINE_OK;
}

static int read_vertex(Scene *scene, const char *body) {
  Vec3 v;
  int end;
  int rc = sscanf(body, "%lf %lf %lf%n", &v.x, &v.y, &v.z, &end);
  if (rc != 3 || !isend(body[end]))
    return INVALID_FORMAT;

  *scene_add_vertex(scene) = v;
  return LINE_OK;
}

static int read_vertex_normal(Scene *scene, const char *body) {
  Vec3 v;
  int end;
  int rc = sscanf(body, "%lf %lf %lf%n", &v.x, &v.y, &v.z, &end);
  if (rc != 3 || !isend(body[end]))
    return INVALID_FORMAT;

  *scene_add_norm(scene) = v;
  return LINE_OK;
}

static int read_vertex_texture(Scene *scene, const char *body) {
  Vec2 v;
  int end;
  int rc = sscanf(body, "%lf %lf%n", &v.x, &v.y, &end);
  if (rc != 2 || !isend(body[end]))
    return INVALID_FORMAT;

  *scene_add_tex(scene) = v;
  return LINE_OK;
}

static int read_triangle_simple(const char *body, Triangle *tri)
{
  int end;
  int rc = sscanf(body, "%d %d %d%n", &tri->p[0], &tri->p[1], &tri->p[2], &end);
  if (rc != 3 || !isend(body[end]))
    return INVALID_FORMAT;
  return LINE_OK;
}

static int read_triangle_normal(const char *body, Triangle *tri)
{
  int end;
  int rc = sscanf(body, "%d//%d %d//%d %d//%d%n", &tri->p[0], &tri->n[0], &tri->p[1], &tri->n[1], &tri->p[2], &tri->n[2], &end);
  if (rc != 6 || !isend(body[end]))
    return INVALID_FORMAT;
  return LINE_OK;
}

static int read_triangle_texture(const char *body, Triangle *tri)
{
  int end;
  int rc = sscanf(body, "%d/%d %d/%d %d/%d%n", &tri->p[0], &tri->t[0], &tri->p[1], &tri->t[1], &tri->p[2], &tri->t[2], &end);
  if (rc != 6 || !isend(body[end]))
    return INVALID_FORMAT;
  return LINE_OK;
}


static int read_triangle(Scene *scene, const char *body, Material *mat) {
  Triangle tri;
  int rc = read_triangle_simple(body, &tri);
  if (rc != LINE_OK)
    rc = read_triangle_normal(body, &tri);
  if (rc != LINE_OK)
    rc = read_triangle_texture(body, &tri);
  if (rc != LINE_OK)
    rc = read_triangle_normal_texture(body, &tri);
  if (rc != LINE_OK)
    return rc;

  for (int i = 0; i < 3; i++)
  {
    tri.p[i]--;
    if (tri.p[i] >= scene->vert_len)
    {
      fprintf(stderr, "invalid vertex index %d, must be less than number of vertices %zu\n", tri.p[i] + 1, scene->vert_len);
      return INVALID_FORMAT;
    }
    tri.n[i]--;
    if (tri.n[i] >= scene->norm_len)
    {
      fprintf(stderr, "invalid normal vertex index %d, must be less than number of vertices %zu\n", tri.n[i] + 1, scene->norm_len);
      return INVALID_FORMAT;
    }
  }
  Object *obj = scene_add_object(scene);
  obj->tri = tri;
  obj->type = OBJECT_TRIANGLE;
  obj->tri.mat = mat;
  return LINE_OK;
}

// Bool variables to check if parameters were passed
typedef struct SceneConfig {
  char eye;
  char viewdir;
  char updir;
  char hfov;
  char imsize;
  char bkgcolor;
  char object;
} SceneConfig;

#define VERIFY_CONFIG(cfg, param)                                              \
  do {                                                                         \
    if ((cfg)->param == 0) {                                                   \
      fprintf(stderr, "required scene parameter '" #param "' not specified");  \
      return 0;                                                                \
    }                                                                          \
  } while (0)

static int scene_verify_valid(Scene *scene, SceneConfig *config) {
  VERIFY_CONFIG(config, eye);
  VERIFY_CONFIG(config, viewdir);
  VERIFY_CONFIG(config, updir);
  VERIFY_CONFIG(config, hfov);
  VERIFY_CONFIG(config, imsize);
  VERIFY_CONFIG(config, bkgcolor);

  if (scene->pixel_width <= 0 || scene->pixel_height <= 0) {
    fprintf(stderr,
            "invalid scene file: width and height must be positive non-zero "
            "integers (given %d x %d)\n",
            scene->pixel_width, scene->pixel_height);
    return 0;
  }

  return 1;
}

static int parse_desc_line(Scene *scene, SceneConfig *config, const char *tag,
                           const char *body) {
  static Material *curr_mtl_color = NULL;
  int end;

  int rc;
  if (strcmp(tag, "eye") == 0) {
    rc = read_vec3(body, &scene->eye);
    config->eye = 1;
  } else if (strcmp(tag, "viewdir") == 0) {
    rc = read_vec3(body, &scene->viewdir);
    config->viewdir = 1;
  } else if (strcmp(tag, "updir") == 0) {
    rc = read_vec3(body, &scene->updir);
    config->updir = 1;
  } else if (strcmp(tag, "hfov") == 0) {
    rc = sscanf(body, "%lf%n", &scene->fov_h, &end) == 1 && isend(body[end])
             ? LINE_OK
             : INVALID_FORMAT;
    config->hfov = 1;
  } else if (strcmp(tag, "imsize") == 0) {
    rc = sscanf(body, "%d %d%n", &scene->pixel_width, &scene->pixel_height,
                &end);
    rc = rc == 2 && isend(body[end]) ? LINE_OK : INVALID_FORMAT;
    config->imsize = 1;
  } else if (strcmp(tag, "bkgcolor") == 0) {
    rc = read_color(body, &scene->bg_color);
    config->bkgcolor = 1;
  } else if (strcmp(tag, "mtlcolor") == 0) {
    curr_mtl_color = scene_add_material(scene);
    rc = read_mat(body, curr_mtl_color);
  } else if (strcmp(tag, "sphere") == 0) {
    rc = read_sphere(scene, body, curr_mtl_color);
    config->object = 1;
  } else if (strcmp(tag, "cylinder") == 0) {
    rc = read_cylinder(scene, body, curr_mtl_color);
    config->object = 1;
  } else if (strcmp(tag, "light") == 0) {
    rc = read_light(scene, body);
  } else if (strcmp(tag, "attlight") == 0) {
    rc = read_att_light(scene, body);
  } else if (strcmp(tag, "depthcueing") == 0) {
    rc = read_depth_cue(body, &scene->depth_cueing);
    scene->depth_cueing_enabled = 1;
  } else if (strcmp(tag, "v") == 0) {
    rc = read_vertex(scene, body);
  } else if (strcmp(tag, "vn") == 0) {
    rc = read_vertex_normal(scene, body);
  } else if (strcmp(tag, "vt") == 0) {
    rc = read_vertex_texture(scene, body);
  } else if (strcmp(tag, "f") == 0) {
    rc = read_triangle(scene, body, curr_mtl_color);
  } else {
    rc = UNRECOGNIZED_TAG;
  }
  return rc;
}

Scene *scene_create_from_file(const char *scene_desc_file_path) {
  FILE *fdesc_file = fopen(scene_desc_file_path, "r");
  if (!fdesc_file) {
    fprintf(stderr, "failed to read scene description file: %s\n",
            strerror(errno));
    return NULL;
  }

  ParseLineResult rc = LINE_OK;
  Scene *scene = calloc(1, sizeof(Scene));
  size_t line_no = 1;

  SceneConfig config = {0};
  char line[256];
  while (fgets(line, sizeof(line), fdesc_file)) {
    if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
    {
      line_no++;
      continue; // line is a comment, ignore
    }
    size_t line_len = strlen(line);
    if (line_len > 0) {
      char tag[32];
      rc = sscanf(line, "%31s", tag);
      if (rc <= 0) {
        fprintf(stderr,
                "invalid scene description file (line %zu): expected line tag "
                "found invalid string\n",
                line_no);
        goto cleanup;
      }

      const char *args = line + strlen(tag);
      rc = parse_desc_line(scene, &config, tag, args);
      switch (rc) {
      case UNRECOGNIZED_TAG: {
        fprintf(stderr,
                "invalid scene description file (line %zu): unrecognized tag "
                "'%s'\n",
                line_no, tag);
        goto cleanup;
      }
      case INVALID_FORMAT:
        fprintf(stderr,
                "invalid scene description file (line %zu): invalid format for "
                "tag '%s'\n",
                line_no, tag);
        goto cleanup;
      default:
        break;
      }
    }
    line_no++;
  }

  if (!scene_verify_valid(scene, &config))
    rc = INVALID_FORMAT;

cleanup:
  if (rc != LINE_OK) {
    scene_destroy(scene);
    scene = NULL;
  }

  fclose(fdesc_file);
  return scene;
}

void scene_destroy(Scene *s) {
  free(s->objects);
  free(s->palette);
  free(s);
}

Object *scene_add_object(Scene *scene) {
  assert(scene->objects_len <= scene->objects_cap);
  if (!scene->objects) {
    scene->objects_cap = 1024;
    scene->objects_len = 0;
    scene->objects = malloc(sizeof(Object) * scene->objects_cap);
  }
  return &scene->objects[scene->objects_len++];
}

Material *scene_add_material(Scene *scene) {
  assert(scene->palette_len <= scene->palette_cap);
  if (!scene->palette) {
    scene->palette_cap = 1024;
    scene->palette_len = 0;
    scene->palette = malloc(sizeof(Material) * scene->palette_cap);
  }
  return &scene->palette[scene->palette_len++];
}

Light *scene_add_light(Scene *scene) {
  assert(scene->lights_len <= scene->lights_cap);
  if (!scene->lights) {
    scene->lights_cap = 1024;
    scene->lights_len = 0;
    scene->lights = malloc(sizeof(Light) * scene->lights_cap);
  }
  return &scene->lights[scene->lights_len++];
}

Vec3 *scene_add_vertex(Scene *scene) {
  assert(scene->vert_len <= scene->vert_cap);
  if (!scene->vertices) {
    scene->vert_cap = 4096;
    scene->vert_len = 0;
    scene->vertices = malloc(sizeof(Vec3) * scene->vert_cap);
  }
  return &scene->vertices[scene->vert_len++];
}

Vec3 *scene_add_norm(Scene *scene) {
  assert(scene->norm_len <= scene->norm_cap);
  if (!scene->normals) {
    scene->norm_cap = 4096;
    scene->norm_len = 0;
    scene->normals = malloc(sizeof(Vec3) * scene->norm_cap);
  }
  return &scene->normals[scene->norm_len++];
}

Vec2 *scene_add_tex(Scene *scene) {
  assert(scene->texs_len <= scene->texs_cap);
  if (!scene->texs) {
    scene->texs_cap = 4096;
    scene->texs_len = 0;
    scene->texs = malloc(sizeof(Vec2) * scene->texs_cap);
  }
  return &scene->texs[scene->vert_len++];
}
