#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include <stdio.h>
#include <string.h>
#include "iio_vulkan_api.h"
#include "iio_eng_typedef.h"
#include "iio_eng_errors.h"
#include "iio_resource_loaders.h"

int main(void) {
  IIOVulkanState * state = iio_init_vulkan_api();
  iio_init_error();
  iio_init_vulkan();
  fprintf(stdout, "Vulkan initialized successfully.\n");
  iio_run();
}
