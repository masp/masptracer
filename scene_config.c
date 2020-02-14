#include "scene_config.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_file_contents(FILE *f) {
  char *file;
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);

  file = calloc(sizeof(char), len + 1);
  if (fread(file, 1, len, f) < 0)
	return NULL;

  file[len] = '\0';
  return file;
}

// Takes in a continuous new line separated string as input, outputs
// a null-terminated string containing the line starting at "start"
//
// Returns length of line, or -1 if eof.
static void read_desc_line(char *start, char *out, size_t out_sz) {
  char *c;
  int line_len = 0;
  for (c = start; *c != '\0' && *c != '\n' && line_len < out_sz;
	   ++c, line_len++)
	out[line_len] = *c;

  assert(c - start == line_len);
  out[line_len] = '\0';
}

typedef enum ParseLineResult {
  LINE_OK,
  UNRECOGNIZED_TAG,
  INVALID_FORMAT
} ParseLineResult;

static int isend(char c) {
  return (c == '\0' || c == '\n');
}

static ParseLineResult read_vec3(const char *body, Vec3 *out) {
  int end;
  int rc = sscanf(body, "%lf %lf %lf%n", &out->x, &out->y, &out->z, &end);
  return rc == 3 && isend(body[end])
		 ? LINE_OK
		 : INVALID_FORMAT;
}

static int read_mat(const char *body, Material *out) {
  Material c = {0};
  int end;
  int rc = sscanf(body, "%lf %lf %lf%n", &c.r, &c.g, &c.b, &end);
  if (rc == 3 && isend(body[end])) {
	*out = c;
	return LINE_OK;
  }
  return INVALID_FORMAT;
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
  int rc = sscanf(body, "%lf %lf %lf %lf %lf %lf %lf %lf%n",
				  &center.x, &center.y, &center.z,
				  &dir.x, &dir.y, &dir.z,
				  &radius,
				  &length,
				  &end);
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

// Bool variables to check if parameters were passed
typedef struct SceneConfig {
  char eye;
  char viewdir;
  char updir;
  char hfov;
  char imsize;
  char bkgcolor;
  char mltcolor;
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
  VERIFY_CONFIG(config, object);

  if (scene->pixel_width <= 0 || scene->pixel_height <= 0) {
	fprintf(stderr, "invalid scene file: width and height must be positive non-zero integers (given %d x %d)\n", scene->pixel_width, scene->pixel_height);
	return 0;
  }

  return 1;
}

static int parse_desc_line(Scene *scene, SceneConfig *config, const char *tag,
						   const char *body) {
  static Material *curr_mlt_color = NULL;
  char sent;

  int rc = 0;
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
	rc = sscanf(body, "%lf%c", &scene->fov_h, &sent) == 1 ? LINE_OK
														  : INVALID_FORMAT;
	config->hfov = 1;
  } else if (strcmp(tag, "imsize") == 0) {
	rc = sscanf(body, "%d %d%c", &scene->pixel_width, &scene->pixel_height,
				&sent) == 2
		 ? LINE_OK
		 : INVALID_FORMAT;
	config->imsize = 1;
  } else if (strcmp(tag, "bkgcolor") == 0) {
	rc = read_mat(body, &scene->bg_color);
	config->bkgcolor = 1;
  } else if (strcmp(tag, "mltcolor") == 0) {
	curr_mlt_color = scene_add_material(scene);
	rc = read_mat(body, curr_mlt_color);
	config->mltcolor = 1;
  } else if (strcmp(tag, "sphere") == 0) {
	rc = read_sphere(scene, body, curr_mlt_color);
	config->object = 1;
  } else if (strcmp(tag, "cylinder") == 0) {
	rc = read_cylinder(scene, body, curr_mlt_color);
	config->object = 1;
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
  char *file_contents = read_file_contents(fdesc_file);

  SceneConfig config = {0};
  char *file_pos = file_contents;
  char line[256];
  while (1) {
	read_desc_line(file_pos, line, sizeof(line));
	size_t line_len = strlen(line);
	if (line_len > 0) {
	  char tag[32];
	  rc = sscanf(file_pos, "%31s", tag);
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
		default:break;
	  }
	}
	line_no++;
	if (file_pos[line_len] == '\0')
	  break;
	file_pos += strlen(line) + 1;
  }

  if (!scene_verify_valid(scene, &config))
	rc = INVALID_FORMAT;

  cleanup:
  if (rc != LINE_OK) {
	scene_destroy(scene);
	scene = NULL;
  }
  if (file_contents)
	free(file_contents);
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
	scene->objects_cap = 8;
	scene->objects_len = 0;
	scene->objects = malloc(sizeof(Object) * scene->objects_cap);
  } else if (scene->objects_len == scene->objects_cap) {
	scene->objects_cap = scene->objects_cap * 2;
	scene->objects =
		realloc(scene->objects, sizeof(Object) * scene->objects_cap);
  }
  return &scene->objects[scene->objects_len++];
}

Material *scene_add_material(Scene *scene) {
  assert(scene->palette_len <= scene->palette_cap);
  if (!scene->palette) {
	scene->palette_cap = 8;
	scene->palette_len = 0;
	scene->palette = malloc(sizeof(Object) * scene->palette_cap);
  } else if (scene->palette_len == scene->palette_cap) {
	scene->palette_cap = scene->palette_cap * 2;
	scene->palette =
		realloc(scene->palette, sizeof(Object) * scene->palette_cap);
  }
  return &scene->palette[scene->palette_len++];
}

