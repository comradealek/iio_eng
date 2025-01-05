#include "glad/vulkan.h"
#include "GLFW/glfw3.h"
#include <stdio.h>
#include <string.h>

int main(void) {
  if (!glfwInit()) {
    printf("glfw init failed\n");
  }
  int major;
  int minor;
  int rev;
  glfwGetVersion(&major, &minor, &rev);
  printf("GLFW Version: %d.%d.%d\n", major, minor, rev);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow * window = glfwCreateWindow(640, 480, "Window", NULL, NULL);
  if (!window) {
    fprintf(stderr, "Window creation failed\n");
  }
  if (glfwVulkanSupported()){
    printf("Vulkan supported\n");
  }
  PFN_vkCreateInstance pfnCreateInstance = (PFN_vkCreateInstance) glfwGetInstanceProcAddress(NULL, "vkCreateInstance");
  uint32_t count;
  const char ** extensions = glfwGetRequiredInstanceExtensions(&count);
  VkInstanceCreateInfo ici;
  memset(&ici, 0, sizeof(ici));
  ici.enabledExtensionCount = count;
  ici.ppEnabledExtensionNames = extensions;
  for (int i = 0; i < count; i++) {
    printf("%s\n", extensions[i]);
  }
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }
  glfwTerminate();
}
