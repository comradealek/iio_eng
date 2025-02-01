#include "glad/vulkan.h"
#include "GLFW/glfw3.h"
#include "dynarr_3.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// #define DEBUG

#define clamp(value, min, max) ((value) < (min) ? (min) : ((value) > (max) ? (max) : (value)))

const uint32_t HEIGHT = 480;
const uint32_t WIDTH = 640;
const char* deviceExtensions [] = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

typedef struct SwapChainSupportDetails_S {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR * formats;
    uint32_t formatCount;
    VkPresentModeKHR * presentModes;
    uint32_t presentModeCount;
} SwapChainSupportDetails;

const char * validationLayers[] = {"VK_LAYER_KHRONOS_validation"};

const char * debug_utils_ext_name = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

#ifndef DEBUG
  const int enableValidationLayers = 0;
#else
  const int enableValidationLayers = 1;
#endif

typedef struct HelloTriangleObj_S {
  GLFWwindow * window;
  VkInstance instance;
  VkPhysicalDevice physicalDevice;
  VkDevice logicalDevice;
  VkQueue graphicsQueue;
  VkQueue presentQueue;
  VkSurfaceKHR surface;
  VkSwapchainKHR swapChain;
} HelloTriangleObj;

typedef struct QueueFamilyIndices_S {
  uint32_t graphicsFamily;
  int hasGraphics;
  uint32_t presentFamily;
  int hasPresent;
} QueueFamilyIndices;

typedef HelloTriangleObj * htobj;

int checkValidationLayerSupport() {
  int code = 1;
  uint32_t layerCount;
  glad_vkEnumerateInstanceLayerProperties(&layerCount, NULL);

  VkLayerProperties availableLayers[layerCount];
  glad_vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

  uint32_t validationLayerCount = sizeof(validationLayers) / sizeof(char *);

#ifdef DEBUG
  fprintf(stdout, "validation layer count: %u\n", validationLayerCount);
  fprintf(stdout, "available layer count:  %u\n", layerCount);
  for (int i = 0; i < layerCount; i++) {
    fprintf(stdout, "%s\n", availableLayers[i].layerName);
  }
#endif

  for (int i = 0; i < validationLayerCount; i++) {
    int layerFound = 0;

    for (int j = 0; j < layerCount; j++) {
      if (strcmp(validationLayers[i], availableLayers[j].layerName) == 0) {
        layerFound = 1;
        break;
      }
    }

    if (!layerFound) {
      code = 0;
    }
  }

  return code;
}

byteArr * getRequiredExtensions() {
  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  byteArr * extensions;
  dynarr_init_m(char *, extensions);
  dynarr_push_m(extensions, glfwExtensions, sizeof(char *) * glfwExtensionCount);

#ifdef DEBUG
  dynarr_push_m(extensions, &debug_utils_ext_name, sizeof(char *));

  for (int i = 0; i < dynarr_length_m(char *, extensions); i++) {
    fprintf(stdout, "%s\n", dynarr_get_m(char *, extensions)[i]);
  }
#endif

  return extensions;
}

void initWindow(htobj target) {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  target->window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Window", NULL, NULL);
}

void initGLADLibraries(htobj target) {
  int version = gladLoadVulkanUserPtr(NULL, (GLADuserptrloadfunc) glfwGetInstanceProcAddress, NULL);
#ifdef DEBUG
  fprintf(stdout, "%d\n", version);
#endif
}

int createInstance(htobj target) {
  int code = 1;

#ifdef DEBUG
  if (!checkValidationLayerSupport()) {
    fprintf(stderr, "validation layers requested but not found\n");
  }
#endif

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
  byteArr * extensions = getRequiredExtensions();
  uint32_t glfwExtensionCount = dynarr_length_m(char *, extensions);
  createInfo.enabledExtensionCount = glfwExtensionCount;
  createInfo.ppEnabledExtensionNames = (const char * const *) dynarr_get_m(char *, extensions);
  createInfo.enabledLayerCount = 0;

#ifdef DEBUG
  createInfo.enabledLayerCount = (uint32_t) (sizeof(validationLayers) / sizeof(char *));
  createInfo.ppEnabledLayerNames = validationLayers;
#endif

  if (glad_vkCreateInstance(&createInfo, NULL, &target->instance) != VK_SUCCESS) {
    printf("failed to create instance\n");
    code = 0;
  }
  return code;
}

int createSurface(htobj target) {
  int code = 1;
  if (glfwCreateWindowSurface(target->instance, target->window, NULL, &target->surface) != VK_SUCCESS) {
    fprintf(stderr, "failed to create a window surface\n");
    code = 0;
  }
  return code;
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

int checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice) {
  uint32_t extensionCount;
  glad_vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &extensionCount, NULL);
  VkExtensionProperties availableExtensions [extensionCount];
  memset(availableExtensions, 0, sizeof(VkExtensionProperties) * extensionCount);

  glad_vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &extensionCount, availableExtensions);

  uint32_t deviceExtensionCount = sizeof(deviceExtensions) / sizeof(char*);

  uint32_t matchingExtensions = 0;
  // gross O(n^2) check
  for (int i = 0; i < extensionCount; i++) {
    for (int j = 0; j < deviceExtensionCount; j++) {
      if (strncmp(availableExtensions[i].extensionName, deviceExtensions[j], 30) == 0) {
        matchingExtensions++;
      }
    }
  }

  return matchingExtensions == deviceExtensionCount;
}

SwapChainSupportDetails querySwapChainSupport(htobj target, VkPhysicalDevice physicalDevice) {
  SwapChainSupportDetails details;

  glad_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, target->surface, &details.capabilities);

  glad_vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, target->surface, &details.formatCount, NULL);

  if (details.formatCount) {
    details.formats = malloc(sizeof(VkSurfaceFormatKHR) * details.formatCount);
    glad_vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, target->surface, &details.formatCount, details.formats);
  }

  glad_vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, target->surface, &details.presentModeCount, NULL);

  if (details.presentModeCount) {
    details.presentModes = malloc(sizeof(VkPresentModeKHR) * details.presentModeCount);
    glad_vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, target->surface, &details.presentModeCount, details.presentModes);
  }

  return details;
}

int isDeviceSuitable(htobj target, VkPhysicalDevice physicalDevice) {
  int code = 1;

  if (code) {
    QueueFamilyIndices indices = findQueueFamilies(target, physicalDevice);
    code = isComplete(&indices);
  }

  if (code) {
    code = checkDeviceExtensionSupport(physicalDevice);
  }

  if (code) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(target, physicalDevice);
    code = swapChainSupport.formats && swapChainSupport.presentModes;
    free(swapChainSupport.formats);
    free(swapChainSupport.presentModes);
  }

  return code;
}

int pickPhysicalDevice(htobj target) {
  int code = 1;
  target->physicalDevice = VK_NULL_HANDLE;
  uint32_t deviceCount = 0;
  glad_vkEnumeratePhysicalDevices(target->instance, &deviceCount, NULL);
  VkPhysicalDevice devices[deviceCount < 1 ? 1 : deviceCount]; // this prevents a zero length array getting made. Not sure how important that is.
  glad_vkEnumeratePhysicalDevices(target->instance, &deviceCount, devices);
  for (int i = 0; i < deviceCount; i++) {
    if (isDeviceSuitable(target, devices[i])) {
      target->physicalDevice = devices[i];
      break;
    }
  }

  if (target->physicalDevice == NULL) {
    fprintf(stderr, "failed to find a suitable GPU\n");
    code = 0;
  }
  return code;
}

int createLogicalDevice(htobj target) {
  int code = 1;
  QueueFamilyIndices indices = findQueueFamilies(target, target->physicalDevice);

  byteArr * queueCreateInfos;
  dynarr_init_m(VkDeviceQueueCreateInfo, queueCreateInfos);

  uint32_t queueFamilies[] = {indices.graphicsFamily, indices.presentFamily};
  uint32_t uniqueQueueFamilies[sizeof(queueFamilies) / sizeof(uint32_t)];
  // bulid a set from queueFamilies to uinqueQueueFamilies
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
    dynarr_push_m(queueCreateInfos, &queueCreateInfo, sizeof(VkDeviceQueueCreateInfo));
  }

  VkPhysicalDeviceFeatures deviceFeatures;
  memset(&deviceFeatures, 0, sizeof(VkPhysicalDeviceFeatures));
  VkDeviceCreateInfo createInfo;
  memset(&createInfo, 0, sizeof(VkDeviceCreateInfo));
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pQueueCreateInfos = (VkDeviceQueueCreateInfo*) queueCreateInfos->dat;
  createInfo.queueCreateInfoCount = (uint32_t)(dynarr_length_m(VkDeviceQueueCreateInfo, queueCreateInfos));
  createInfo.pEnabledFeatures = &deviceFeatures;

  uint32_t deviceExtensionCount = sizeof(deviceExtensions) / sizeof(char *);
  createInfo.enabledExtensionCount = deviceExtensionCount;
  createInfo.ppEnabledExtensionNames = deviceExtensions;
  createInfo.enabledLayerCount = 0;

  if (glad_vkCreateDevice(target->physicalDevice, &createInfo, NULL, &target->logicalDevice) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create logical device\n");
    code = 0;
  }
  free(queueCreateInfos);
  return code;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(SwapChainSupportDetails * details) {
  VkSurfaceFormatKHR selectedFormat = details->formats[0];
  for (int i = 0; i < details->formatCount; i++) {
    if (details->formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && details->formats[i].format == VK_FORMAT_B8G8R8A8_SRGB) {
      selectedFormat = details->formats[i];
      break;
    }
  }

  return selectedFormat;
}

VkPresentModeKHR chooseSwapPresentMode (SwapChainSupportDetails * details) {
  VkPresentModeKHR selectedPresentMode = VK_PRESENT_MODE_FIFO_KHR;
  for (int i = 0; i < details->presentModeCount; i++) {
    if (details->presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      selectedPresentMode = details->presentModes[i];
      break;
    }
  }

  return selectedPresentMode;
}

VkExtent2D chooseSwapExtent(htobj target, VkSurfaceCapabilitiesKHR * capabilites) {
  VkExtent2D extent;
  if (capabilites->currentExtent.width != 0xffffffff) {
    extent = capabilites->currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(target->window, &width, &height);
    extent.width = (uint32_t) width;
    extent.height = (uint32_t) height;
    extent.width = clamp(extent.width, capabilites->minImageExtent.width, capabilites->maxImageExtent.width);
    extent.height = clamp(extent.height, capabilites->minImageExtent.height, capabilites->maxImageExtent.height);
  }
  return extent;
}

int createSwapChain(htobj target) {
  int code = 1;
  SwapChainSupportDetails swapChainSupport = querySwapChainSupport(target, target->physicalDevice);

  VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(&swapChainSupport);
  VkPresentModeKHR presentMode = chooseSwapPresentMode(&swapChainSupport);
  VkExtent2D swapExtent = chooseSwapExtent(target, &swapChainSupport.capabilities);

  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

  if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo;
  memset(&createInfo, 0, sizeof(VkSwapchainCreateInfoKHR));
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = target->surface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = swapExtent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilyIndices indices = findQueueFamilies(target, target->physicalDevice);
  uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};

  if (indices.graphicsFamily != indices.presentFamily) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = NULL;
  }

  createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;

  createInfo.oldSwapchain = VK_NULL_HANDLE;

  memset(&target->swapChain, 0, sizeof(VkSwapchainKHR));

  if (glad_vkCreateSwapchainKHR(target->logicalDevice, &createInfo, NULL, &target->swapChain) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create swap chain\n");
    code = 0;
  }

  if (swapChainSupport.formats) free(swapChainSupport.formats);
  if (swapChainSupport.presentModes) free(swapChainSupport.presentModes);

  return code;
}

int initVulkan(htobj target) {
  int code = 1;
  if (code == 1) code = createInstance(target);
  if (code == 1) code = createSurface(target);
  if (code == 1) code = pickPhysicalDevice(target);
  if (code == 1) ;
  if (code == 1) code = createLogicalDevice(target);
  if (code == 1) code = createSwapChain(target);
  return code;
}

void initGLAD(htobj target) {
  int version = gladLoadVulkanUserPtr(target->physicalDevice, (GLADuserptrloadfunc) glfwGetInstanceProcAddress, target->instance);
#ifdef DEBUG
  fprintf(stdout, "%d\n", version);
#endif
}

void mainLoop(htobj target) {
  while (!glfwWindowShouldClose(target->window)) {
    glfwPollEvents();
  }
}

void cleanup(htobj target) {
  glad_vkDestroySwapchainKHR(target->logicalDevice, target->swapChain, NULL);
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
  HelloTriangleObj object = {0};
  run(&object);
  return 0;
}