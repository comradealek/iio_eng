#include "glad/vulkan.h"
#include "GLFW/glfw3.h"
#include "dynarr.h"
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
  VkDevice logicalDevice;
  VkQueue graphicsQueue;
  VkQueue presentQueue;
  VkSurfaceKHR surface;
} HelloTriangleObj;

typedef struct {
  uint32_t graphicsFamily;
  int hasGraphics;
  uint32_t presentFamily;
  int hasPresent;
} QueueFamilyIndices;

typedef HelloTriangleObj * htobj;

void initWindow(htobj target) {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  target->window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Window", NULL, NULL);
}

void initGLADLibraries(htobj target) {
  int version = gladLoadVulkanUserPtr(NULL, (GLADuserptrloadfunc) glfwGetInstanceProcAddress, NULL);
  // fprintf(stdout, "%d\n", version);
}

void createInstance(htobj target) {
  VkApplicationInfo appInfo;
  memset(&appInfo, 0, sizeof(VkApplicationInfo));
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = NULL;
  appInfo.applicationVersion = 0;
  appInfo.pEngineName = NULL;
  appInfo.engineVersion = 0;
  appInfo.apiVersion = VK_API_VERSION_1_0;
  appInfo.pNext = NULL;

  VkInstanceCreateInfo createInfo;
  memset(&createInfo, 0, sizeof(VkInstanceCreateInfo));
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  uint32_t glfwExtensionCount = 0;
  const char ** glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  createInfo.enabledExtensionCount = glfwExtensionCount;
  createInfo.ppEnabledExtensionNames = glfwExtensions;
  createInfo.enabledLayerCount = 0;

  if (glad_vkCreateInstance(&createInfo, NULL, &target->instance) != VK_SUCCESS) {
    printf("failed to create instance\n");
  }
}

void createSurface(htobj target) {
  if (glfwCreateWindowSurface(target->instance, target->window, NULL, &target->surface) != VK_SUCCESS) {
    fprintf(stderr, "failed to create a window surface\n");
  }
}

int isComplete(QueueFamilyIndices * queue) {
  return queue->hasGraphics && queue->hasPresent;
}

QueueFamilyIndices findQueueFamilies(htobj target, VkPhysicalDevice physicalDevice) {
  QueueFamilyIndices indices;
  memset(&indices, 0, sizeof(QueueFamilyIndices));

  uint32_t queueFamilyCount;
  glad_vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);
  VkQueueFamilyProperties queueFamilies[queueFamilyCount];
  memset(queueFamilies, 0, sizeof(VkQueueFamilyProperties) * queueFamilyCount);
  glad_vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);

  for (int i = 0; i < queueFamilyCount; i++) {
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphicsFamily = i;
      indices.hasGraphics = 1;
    }
    VkBool32 presentSupport = VK_FALSE;
    glad_vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, target->surface, &presentSupport);
    if (presentSupport) {
      indices.presentFamily = i;
      indices.hasPresent = 1;
    }
    if (isComplete(&indices)) break;
  }

  return indices;
}

int isDeviceSuitable(htobj target, VkPhysicalDevice physicalDevice) {
  QueueFamilyIndices indices = findQueueFamilies(target, physicalDevice);

  return indices.hasGraphics;
}

void pickPhysicalDevice(htobj target) {
  target->physicalDevice = VK_NULL_HANDLE;
  uint32_t deviceCount = 0;
  glad_vkEnumeratePhysicalDevices(target->instance, &deviceCount, NULL);
  if (deviceCount == 0) {
    fprintf(stderr, "No devices found that support Vulkan\n");
    return;
  }
  VkPhysicalDevice devices[deviceCount];
  glad_vkEnumeratePhysicalDevices(target->instance, &deviceCount, devices);
  for (int i = 0; i < deviceCount; i++) {
    if (isDeviceSuitable(target, devices[i])) {
      target->physicalDevice = devices[i];
      break;
    }
  }

  if (target->physicalDevice == NULL) {
    fprintf(stderr, "failed to find a suitable GPU\n");
  }
}

void createLogicalDevice(htobj target) {
  QueueFamilyIndices indices = findQueueFamilies(target, target->physicalDevice);

  byteArr * queueCreateInfos;
  dynarr_init_m(VkDeviceQueueCreateInfo, queueCreateInfos);

  uint32_t queueFamilies[] = {indices.graphicsFamily, indices.presentFamily};
  uint32_t uniqueQueueFamilies[sizeof(queueFamilies) / sizeof(uint32_t)];
  int l = 0;
  for (int i = 0; i < (sizeof(queueFamilies) / sizeof(uint32_t)); i++) {
    // check if the uniqueQueueFamilies array contains queueFamilies[i]
    int contains = 0;
    for (int j = 0; j < l; j++) {
      if (uniqueQueueFamilies[j] == queueFamilies[i]) {
        contains = 1;
        break;
      }
    }
    // end check

    // if uniqueQueueFamilies doesn't have queueFamilies[i], we add it at position l and increment l
    if (!contains) {
      uniqueQueueFamilies[l++] = queueFamilies[i];
    }
  }

  // Go through each value in uniqueQueueFamilies and create 'VkDeviceQueueCreateInfo's for each one
  float queuePriority = 1.0f;
  for (int i = 0; i < l; i++) {
    VkDeviceQueueCreateInfo queueCreateInfo;
    memset(&queueCreateInfo, 0, sizeof(VkDeviceQueueCreateInfo));
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = uniqueQueueFamilies[i];
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos = dynarr_push(queueCreateInfos, &queueCreateInfo, sizeof(VkDeviceQueueCreateInfo));
  }

  VkPhysicalDeviceFeatures deviceFeatures;
  memset(&deviceFeatures, 0, sizeof(VkPhysicalDeviceFeatures));
  VkDeviceCreateInfo createInfo;
  memset(&createInfo, 0, sizeof(VkDeviceCreateInfo));
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pQueueCreateInfos = (VkDeviceQueueCreateInfo*) queueCreateInfos->dat;
  createInfo.queueCreateInfoCount = (uint32_t)(arr_length(queueCreateInfos, sizeof(VkDeviceQueueCreateInfo)));
  createInfo.pEnabledFeatures = &deviceFeatures;

  createInfo.enabledExtensionCount = 0;
  createInfo.enabledLayerCount = 0;

  if (glad_vkCreateDevice(target->physicalDevice, &createInfo, NULL, &target->logicalDevice) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create logical device\n");
  }
  free(queueCreateInfos);
}

void initVulkan(htobj target) {
  createInstance(target);
  createSurface(target);
  pickPhysicalDevice(target);
  createLogicalDevice(target);
}

void initGLAD(htobj target) {
  int version = gladLoadVulkanUserPtr(target->physicalDevice, (GLADuserptrloadfunc) glfwGetInstanceProcAddress, target->instance);
  // fprintf(stdout, "%d\n", version);
}

void mainLoop(htobj target) {
  while(!glfwWindowShouldClose(target->window)){
    glfwPollEvents();
  }
}

void cleanup(htobj target) {
  glad_vkDestroyDevice(target->logicalDevice, NULL);
  glad_vkDestroySurfaceKHR(target->instance, target->surface, NULL);
  glad_vkDestroyInstance(target->instance, NULL);
  glfwDestroyWindow(target->window);
  glfwTerminate();
}

void run(htobj target) {
  initWindow(target);
  initGLADLibraries(target);
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