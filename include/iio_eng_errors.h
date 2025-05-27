#ifndef IIOENGERRORS
#define IIOENGERRORS

#define IIO_ERROR_MSG_MAX 2048

#define min(x,y) ((x) < (y) ? (x) : (y))
#define max(x,y) ((x) > (y) ? (x) : (y))

typedef struct IIOError_S {
  unsigned long int code;
  VkResult vulkancode;
  uint32_t glfwcode;
  char message [IIO_ERROR_MSG_MAX];
} IIOError;

void iio_init_error();

void iio_oom_error(void * d, int line, const char * file);

void iio_vk_error(VkResult vulkancode, int line, const char * file);

void iio_glfw_error(uint32_t glfwcode, int line, const char * file);

char * iio_error_message_from_vulkan_code(VkResult vulkancode, int line, const char * file);

char * iio_error_message_from_glfw_code(uint32_t glfwcode, int line, const char * file);

void iio_print_error();

int error_test(void);

#endif