#include "yyjson.h"
#include "glad/vulkan.h"
#include "GLFW/glfw3.h"

int main(void) {
  if(!glfwInit()) {
    printf("glfw init failed\n");
  }
  int major;
  int minor;
  int rev;
  glfwGetVersion(&major, &minor, &rev);
  printf("GLFW Version: %d.%d.%d\n", major, minor, rev);
}
