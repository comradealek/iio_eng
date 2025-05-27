#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "GLFW/glfw3.h"
#include <vulkan/vulkan.h>
#include "iio_eng_errors.h"

IIOError iio_error = {0};

/**
 * error message headers and footers
 */

char IIO_VK_ERROR_HEADER [] = "\
Error detected in vulkan:\n\
\n\
";

char IIO_GLFW_ERROR_HEADER [] = "\
Error detected in glfw:\n\
\n\
";

char IIO_ERROR_FOOTER [] = "\
\n\
End of error.\n\
";

const char IIO_DEFAULT_MSG [] = "\
This error code has not been implemented\n\
";

/**
 * Generic Error Messages
 */

const char IIO_OOM_MSG [] = "\
Out of memory error reported\n\
";

/**
 * GLFW Error Messages
 */

const char IIO_GLFW_NO_ERROR_MSG [] = "\
Error handled but not detected\n\
";

const char IIO_GLFW_NOT_INITIALIZED_MSG [] = "\
GLFW not initialized\n\
";

const char IIO_GLFW_NO_CURRENT_CONTEXT_MSG [] = "\
GLFW no current context\n\
";

const char IIO_GLFW_INVALID_ENUM_MSG [] = "\
GLFW invalid enum\n\
";

const char IIO_GLFW_INVALID_VALUE_MSG [] = "\
GLFW invalid value\n\
";

const char IIO_GLFW_OUT_OF_MEMORY_MSG [] = "\
GLFW out of memory\n\
";

const char IIO_GLFW_API_UNAVAILABLE_MSG [] = "\
GLFW API unavailable\n\
";

const char IIO_GLFW_VERSION_UNAVAILABLE_MSG [] = "\
GLFW version unavailable\n\
";

const char IIO_GLFW_PLATFORM_ERROR_MSG [] = "\
GLFW platform error\n\
";

const char IIO_GLFW_FORMAT_UNAVAILABLE_MSG [] = "\
GLFW format unavailable\n\
";

const char IIO_GLFW_NO_WINDOW_CONTEXT_MSG [] = "\
GLFW no window context\n\
";

const char IIO_GLFW_CURSOR_UNAVAILABLE_MSG [] = "\
GLFW cursor unavailable\n\
";

const char IIO_GLFW_FEATURE_UNAVAILABLE_MSG [] = "\
GLFW feature unavailable\n\
";

const char IIO_GLFW_FEATURE_UNIMPLEMENTED_MSG [] = "\
GLFW feature unimplemented\n\
";

const char IIO_GLFW_PLATFORM_UNAVAILABLE_MSG [] = "\
GLFW platform unavailable\n\
";

/**
 * Vulkan Error Messages
 */


 
/**
 * executable code
 */

void iio_init_error() {
  atexit(iio_print_error);
}

void iio_oom_error(void * d, int line, const char * file) {
  iio_error.code = 1;
  int n = 0;
  char * message = calloc(IIO_ERROR_MSG_MAX, sizeof(char));
  sprintf(message, "%s:%d\n\n", file, line);
  n = strlen(message);
  memcpy(message + n, IIO_OOM_MSG, min(IIO_ERROR_MSG_MAX - n, sizeof(IIO_OOM_MSG)));
  n += sizeof(IIO_OOM_MSG) - 1;
  memcpy(message + n, IIO_ERROR_FOOTER, min(IIO_ERROR_MSG_MAX - n, sizeof(IIO_ERROR_FOOTER)));
  free(message);
}

void iio_vk_error(VkResult vulkancode, int line, const char * file) {
  iio_error.code = 1;
  iio_error.vulkancode = vulkancode;
  iio_error.glfwcode = glfwGetError(NULL);
  char * message = iio_error_message_from_vulkan_code(vulkancode, line, file);
  memcpy(iio_error.message, message, IIO_ERROR_MSG_MAX);
  free(message);
}

void iio_glfw_error(uint32_t glfwcode, int line, const char * file) {
  iio_error.code = 1;
  iio_error.vulkancode = VK_SUCCESS;
  iio_error.glfwcode = glfwcode;
  char * message = iio_error_message_from_glfw_code(glfwcode, line, file);
  memcpy(iio_error.message, message, IIO_ERROR_MSG_MAX);
  free(message);
}

char * iio_error_message_from_vulkan_code(VkResult vulkancode, int line, const char * file) {
  char * message = calloc(IIO_ERROR_MSG_MAX, sizeof(char));
  memcpy(message, IIO_VK_ERROR_HEADER, min(sizeof(IIO_VK_ERROR_HEADER), IIO_ERROR_MSG_MAX));
  int n = sizeof(IIO_VK_ERROR_HEADER) - 1;
  switch(vulkancode) {
    case VK_ERROR_OUT_OF_HOST_MEMORY:
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
    case VK_ERROR_OUT_OF_POOL_MEMORY:
      memcpy(message + n, IIO_OOM_MSG, min(IIO_ERROR_MSG_MAX - n, sizeof(IIO_OOM_MSG)));
      n += sizeof(IIO_OOM_MSG) - 1;
      break;
    default:
      memcpy(message + n, IIO_DEFAULT_MSG, min(IIO_ERROR_MSG_MAX - n, sizeof(IIO_DEFAULT_MSG)));
      n += sizeof(IIO_DEFAULT_MSG) - 1;
  }
  memcpy(message + n, IIO_ERROR_FOOTER, min(IIO_ERROR_MSG_MAX - n, sizeof(IIO_ERROR_FOOTER)));
  return message;
}

char * iio_error_message_from_glfw_code(uint32_t glfwcode, int line, const char * file) {
  char * message = calloc(IIO_ERROR_MSG_MAX, sizeof(char));
  int n = 0;
  if (message == NULL) {
    fprintf(stderr, "Error allocating memory for error message\n");
    return NULL;
  }
  if (strlen(file) > IIO_ERROR_MSG_MAX / 2) {
    fprintf(stderr, "Error: file name suspiciously long\n");
    return NULL;
  }
  sprintf(message, "%s:%d\n", file, line);
  n = strlen(message);
  memcpy(message + n, IIO_GLFW_ERROR_HEADER, min(sizeof(IIO_GLFW_ERROR_HEADER), IIO_ERROR_MSG_MAX - n));
  n += sizeof(IIO_GLFW_ERROR_HEADER) - 1;
  switch(glfwcode) {
    case GLFW_NO_ERROR:
      memcpy(message + n, IIO_GLFW_NO_ERROR_MSG, min(IIO_ERROR_MSG_MAX - n, sizeof(IIO_GLFW_NO_ERROR_MSG)));
      n += sizeof(IIO_GLFW_NO_ERROR_MSG) - 1;
      break;
    case GLFW_NOT_INITIALIZED:
      memcpy(message + n, IIO_GLFW_NOT_INITIALIZED_MSG, min(IIO_ERROR_MSG_MAX - n, sizeof(IIO_GLFW_NOT_INITIALIZED_MSG)));
      n += sizeof(IIO_GLFW_NOT_INITIALIZED_MSG) - 1;
      break;
    case GLFW_NO_CURRENT_CONTEXT:
      memcpy(message + n, IIO_GLFW_NO_CURRENT_CONTEXT_MSG, min(IIO_ERROR_MSG_MAX - n, sizeof(IIO_GLFW_NO_CURRENT_CONTEXT_MSG)));
      n += sizeof(IIO_GLFW_NO_CURRENT_CONTEXT_MSG) - 1;
      break;
    case GLFW_INVALID_ENUM:
      memcpy(message + n, IIO_GLFW_INVALID_ENUM_MSG, min(IIO_ERROR_MSG_MAX - n, sizeof(IIO_GLFW_INVALID_ENUM_MSG)));
      n += sizeof(IIO_GLFW_INVALID_ENUM_MSG) - 1;
      break;
    case GLFW_INVALID_VALUE:
      memcpy(message + n, IIO_GLFW_INVALID_VALUE_MSG, min(IIO_ERROR_MSG_MAX - n, sizeof(IIO_GLFW_INVALID_VALUE_MSG)));
      n += sizeof(IIO_GLFW_INVALID_VALUE_MSG) - 1;
      break;
    case GLFW_OUT_OF_MEMORY:
      memcpy(message + n, IIO_GLFW_OUT_OF_MEMORY_MSG, min(IIO_ERROR_MSG_MAX - n, sizeof(IIO_GLFW_OUT_OF_MEMORY_MSG)));
      n += sizeof(IIO_GLFW_OUT_OF_MEMORY_MSG) - 1;
      break;
    case GLFW_API_UNAVAILABLE:
      memcpy(message + n, IIO_GLFW_API_UNAVAILABLE_MSG, min(IIO_ERROR_MSG_MAX - n, sizeof(IIO_GLFW_API_UNAVAILABLE_MSG)));
      n += sizeof(IIO_GLFW_API_UNAVAILABLE_MSG) - 1;
      break;
    case GLFW_VERSION_UNAVAILABLE:
      memcpy(message + n, IIO_GLFW_VERSION_UNAVAILABLE_MSG, min(IIO_ERROR_MSG_MAX - n, sizeof(IIO_GLFW_VERSION_UNAVAILABLE_MSG)));
      n += sizeof(IIO_GLFW_VERSION_UNAVAILABLE_MSG) - 1;
      break;
    case GLFW_PLATFORM_ERROR:
      memcpy(message + n, IIO_GLFW_PLATFORM_ERROR_MSG, min(IIO_ERROR_MSG_MAX - n, sizeof(IIO_GLFW_PLATFORM_ERROR_MSG)));
      n += sizeof(IIO_GLFW_PLATFORM_ERROR_MSG) - 1;
      break;
    case GLFW_FORMAT_UNAVAILABLE:
      memcpy(message + n, IIO_GLFW_FORMAT_UNAVAILABLE_MSG, min(IIO_ERROR_MSG_MAX - n, sizeof(IIO_GLFW_FORMAT_UNAVAILABLE_MSG)));
      n += sizeof(IIO_GLFW_FORMAT_UNAVAILABLE_MSG) - 1;
      break;
    case GLFW_NO_WINDOW_CONTEXT:
      memcpy(message + n, IIO_GLFW_NO_WINDOW_CONTEXT_MSG, min(IIO_ERROR_MSG_MAX - n, sizeof(IIO_GLFW_NO_WINDOW_CONTEXT_MSG)));
      n += sizeof(IIO_GLFW_NO_WINDOW_CONTEXT_MSG) - 1;
      break;
    case GLFW_CURSOR_UNAVAILABLE:
      memcpy(message + n, IIO_GLFW_CURSOR_UNAVAILABLE_MSG, min(IIO_ERROR_MSG_MAX - n, sizeof(IIO_GLFW_CURSOR_UNAVAILABLE_MSG)));
      n += sizeof(IIO_GLFW_CURSOR_UNAVAILABLE_MSG) - 1;
      break;
    case GLFW_FEATURE_UNAVAILABLE:
      memcpy(message + n, IIO_GLFW_FEATURE_UNAVAILABLE_MSG, min(IIO_ERROR_MSG_MAX - n, sizeof(IIO_GLFW_FEATURE_UNAVAILABLE_MSG)));
      n += sizeof(IIO_GLFW_FEATURE_UNAVAILABLE_MSG) - 1;
      break;
    case GLFW_FEATURE_UNIMPLEMENTED:
      memcpy(message + n, IIO_GLFW_FEATURE_UNIMPLEMENTED_MSG, min(IIO_ERROR_MSG_MAX - n, sizeof(IIO_GLFW_FEATURE_UNIMPLEMENTED_MSG)));
      n += sizeof(IIO_GLFW_FEATURE_UNIMPLEMENTED_MSG) - 1;
      break;
    case GLFW_PLATFORM_UNAVAILABLE:
      memcpy(message + n, IIO_GLFW_PLATFORM_UNAVAILABLE_MSG, min(IIO_ERROR_MSG_MAX - n, sizeof(IIO_GLFW_PLATFORM_UNAVAILABLE_MSG)));
      n += sizeof(IIO_GLFW_PLATFORM_UNAVAILABLE_MSG) - 1;
      break;
    default:
      memcpy(message + n, IIO_DEFAULT_MSG, min(IIO_ERROR_MSG_MAX - n, sizeof(IIO_DEFAULT_MSG)));
      n += sizeof(IIO_DEFAULT_MSG) - 1;
  }
  memcpy(message + n, IIO_ERROR_FOOTER, min(IIO_ERROR_MSG_MAX - n, sizeof(IIO_ERROR_FOOTER)));
  return message;
}

void iio_print_error() {
  fprintf(stderr, iio_error.message);
}

int error_test(void) {
  iio_vk_error(VK_ERROR_OUT_OF_HOST_MEMORY, __LINE__, __FILE__);
  fprintf(stdout, iio_error.message);
}