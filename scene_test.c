#include "scene.h"
#include <CUnit/Basic.h>

#define ASSERT_COLOR_EQUAL(actual, ex1, ex2, ex3)                              \
  do {                                                                         \
    CU_assertImplementation(                                                   \
        ((actual).r == (ex1) && (actual).g == (ex2) && (actual).b == (ex3)),   \
        __LINE__,                                                              \
        ("ASSERT_COLOR_EQUAL(" #actual ", " #ex1 ", " #ex2 ", " #ex3 ")"),      \
        __FILE__, "", CU_FALSE);                                               \
  } while (0)

#define ASSERT_VEC3_EQUAL(actual, ex1, ex2, ex3)                               \
  do {                                                                         \
    CU_assertImplementation(                                                   \
        ((actual).x == (ex1) && (actual).y == (ex2) && (actual).z == (ex3)),   \
        __LINE__,                                                              \
        ("ASSERT_VEC3_EQUAL(" #actual ", " #ex1 ", " #ex2 ", " #ex3 ")"),       \
        __FILE__, "", CU_FALSE);                                               \
  } while (0)

void test_scene() {
  Scene *s = scene_create_from_file("../scenes/test.scene");
  CU_ASSERT_NOT_EQUAL_FATAL(s, NULL);
  ASSERT_VEC3_EQUAL(s->eye, 0, 0, 5);
  ASSERT_VEC3_EQUAL(s->viewdir, 0, 0, -1);
  ASSERT_VEC3_EQUAL(s->updir, 0, 1, 0);
  CU_ASSERT_EQUAL(s->fov_h, 45);
  CU_ASSERT_EQUAL(s->pixel_width, 800);
  CU_ASSERT_EQUAL(s->pixel_height, 600);
  ASSERT_COLOR_EQUAL(s->bg_color, 0, 0, 0.1);

  CU_ASSERT_EQUAL_FATAL(s->objects_len, 1);
  CU_ASSERT_EQUAL(s->objects[0].type, OBJECT_SPHERE);
  Sphere *sphere = &s->objects[0].sphere;
  ASSERT_VEC3_EQUAL(sphere->center, 0, 0, 1);
  CU_ASSERT_EQUAL(sphere->radius, 2);
  ASSERT_COLOR_EQUAL(sphere->color, 1, 0, 0);
}

void test_empty_scene() {
  CU_ASSERT_EQUAL(scene_create_from_file("../scenes/empty.scene"), NULL);
}

void test_invalid_scene() {
  CU_ASSERT_EQUAL(scene_create_from_file("../scenes/invalid_args.scene"), NULL);
}

int main(int argc, char **argv) {
  if (CU_initialize_registry() != CUE_SUCCESS)
    return CU_get_error();

  CU_basic_set_mode(CU_BRM_VERBOSE);

  CU_pSuite pSuite = NULL;
  pSuite = CU_add_suite("scene_files", 0, 0);
  if (NULL == pSuite)
    goto cleanup;

  if (NULL == CU_add_test(pSuite, "test_scene", test_scene) ||
      NULL == CU_add_test(pSuite, "test_invalid_scene", test_invalid_scene) ||
      NULL == CU_add_test(pSuite, "test_empty_scene", test_empty_scene))
    goto cleanup;

  CU_basic_run_tests();
cleanup:
  CU_cleanup_registry();
  return CU_get_error();
}