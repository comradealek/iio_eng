#include "glad/vulkan.h"
#include "GLFW/glfw3.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

const uint32_t HEIGHT = 480;
const uint32_t WIDTH = 640;

const char * validationLayers = "VK_LAYER_KHRONOS_validation";

#ifndef DEBUG
  const int enableValidationLayers = 0;
#else
  const int enableValidationLayers = 1;
#endif

typedef struct {
  GLFWwindow * window;
  VkInstance instance;
  VkPhysicalDevice physicalDevice;
} HelloTriangleObj;

typedef HelloTriangleObj * htobj;

void initWindow(htobj target) {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  target->window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Window", NULL, NULL);
}

void createInstance(htobj target) {
  VkApplicationInfo appInfo;
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = NULL;
  appInfo.applicationVersion = 0;
  appInfo.pEngineName = NULL;
  appInfo.engineVersion = 0;
  appInfo.apiVersion = VK_API_VERSION_1_0;
  appInfo.pNext = NULL;

  VkInstanceCreateInfo createInfo;
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  uint32_t glfwExtensionCount = 0;
  const char ** glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  createInfo.enabledExtensionCount = glfwExtensionCount;
  createInfo.ppEnabledExtensionNames = glfwExtensions;
  createInfo.enabledLayerCount = 0;

  PFN_vkCreateInstance pfnCreateInstance = (PFN_vkCreateInstance) glfwGetInstanceProcAddress(NULL, "vkCreateInstance");
  if (pfnCreateInstance(&createInfo, NULL, &target->instance) != VK_SUCCESS) {
    printf("Failed to create instance\n");
  }
}

int isDeviceSuitable(VkPhysicalDevice physicalDevice) {
  VkPhysicalDeviceProperties deviceProperties;
  VkPhysicalDeviceFeatures deviceFeatures;
  PFN_vkGetPhysicalDeviceProperties pfnGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties) glfwGetInstanceProcAddress(NULL, "vkGetPhysicalDeviceProperties");
  PFN_vkGetPhysicalDeviceFeatures pfnGetPhysicalDeviceFeatures = (PFN_vkGetPhysicalDeviceFeatures) glfwGetInstanceProcAddress(NULL, "vkGetPhysicalDeviceFeatures");
  pfnGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
  pfnGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
  int major = VK_API_VERSION_MAJOR(deviceProperties.apiVersion);
  int minor = VK_API_VERSION_MINOR(deviceProperties.apiVersion);
  fprintf(stdout, "Vulkan API Version %d.%d\n", major, minor);
  if (!deviceFeatures.geometryShader) {
    return 0;
  }
  if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
    return 1;
  }
  // Get more in depth later
  return 0;
}

void pickPhysicalDevice(htobj target) {
  target->physicalDevice = VK_NULL_HANDLE;
  uint32_t deviceCount = 0;
  PFN_vkEnumeratePhysicalDevices pfnEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices) glfwGetInstanceProcAddress(NULL, "vkEnumeratePhysicalDevices");
  pfnEnumeratePhysicalDevices(target->instance, &deviceCount, NULL);
  if (deviceCount == 0) {
    fprintf(stderr, "No devices found that support Vulkan\n");
    return;
  }
  VkPhysicalDevice devices[deviceCount];
  pfnEnumeratePhysicalDevices(target->instance, &deviceCount, devices);
  for (int i = 0; i < deviceCount; i++) {
    if (isDeviceSuitable(devices[i])) {
      target->physicalDevice = devices[i];
      break;
    }
  }

  if (target->physicalDevice == NULL) {
    fprintf(stderr, "failed to find a suitable GPU\n");
  }
}

void initVulkan(htobj target) {
  createInstance(target);
  pickPhysicalDevice(target);
}

void initGLAD(htobj target) {
  int version = gladLoadVulkanUserPtr(target->physicalDevice, (GLADuserptrloadfunc) glfwGetInstanceProcAddress, target->instance);
  fprintf(stdout, "%d\n", version);
}

void mainLoop(htobj target) {
  while(!glfwWindowShouldClose(target->window)){
    glfwPollEvents();
  }
}

void cleanup(htobj target) {
  glad_vkDestroyInstance(target->instance, NULL);
  glfwDestroyWindow(target->window);
  glfwTerminate();
}

void run(htobj target) {
  initWindow(target);
  initVulkan(target);
  initGLAD(target);
  mainLoop(target);
  cleanup(target);
}

int main(void) {
  HelloTriangleObj object;
  run(&object);
  return 0;
}