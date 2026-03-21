#include <assert.h>
#include <stdint.h>

#include "backend/output_utils.h"
#include "utils.h"

static void assert_output_geometry(const wm_output_info_t *output, int32_t x,
                                   int32_t y, int32_t width,
                                   int32_t height) {
  assert(output);
  assert(output->geometry.x == x);
  assert(output->geometry.y == y);
  assert(output->geometry.width == width);
  assert(output->geometry.height == height);
}

static void destroy_detect_result(wm_backend_detect_t *detect) {
  assert(detect);
  for (size_t i = 0; i < detect->output_count; i++) {
    p_delete(&detect->outputs[i].name);
  }
  p_delete(&detect->outputs);
  free(detect);
}

static void test_output_remove_duplication_invalid_input(void) {
  wm_output_info_t output = {0};

  assert(output_remove_duplication(nullptr, 0) == nullptr);
  assert(output_remove_duplication(&output, 0) == nullptr);
}

static void test_output_remove_duplication_keeps_non_nested_outputs(void) {
  wm_output_info_t outputs[] = {
    {
      .geometry = {.x = 0, .y = 0, .width = 3840, .height = 1080},
    },
    {
      .geometry = {.x = 0, .y = 0, .width = 1920, .height = 1080},
    },
    {
      .geometry = {.x = 1920, .y = 0, .width = 1920, .height = 1080},
    },
    {
      .geometry = {.x = 3840, .y = 0, .width = 1280, .height = 1024},
    },
  };

  wm_backend_detect_t *detect =
    output_remove_duplication(outputs, countof(outputs));
  assert(detect);
  assert(detect->output_count == 2);
  assert_output_geometry(&detect->outputs[0], 0, 0, 3840, 1080);
  assert_output_geometry(&detect->outputs[1], 3840, 0, 1280, 1024);

  destroy_detect_result(detect);
}

int main(void) {
  test_output_remove_duplication_invalid_input();
  test_output_remove_duplication_keeps_non_nested_outputs();
  return 0;
}
