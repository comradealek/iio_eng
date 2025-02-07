// #define GLFW_INCLUDE_VULKAN
#ifndef GLFW_INCLUDE_VULKAN
#include "glad/vulkan.h"
#endif
#include "GLFW/glfw3.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "string.h"
#include <inttypes.h>

#define DEBUG

typedef struct AppStruct_S {
  GLFWwindow* window;
  VkInstance instance;

  VkPhysicalDevice physicalDevice;
} AppStruct;

typedef AppStruct * pAsobj;

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

#ifdef DEBUG
const char ** validationLayers = {
  "VK_LAYER_KHRONOS_validation",
};

int checkValidationLayerSupport(pAsobj obj) {
  uint32_t layerCount;
#ifdef GLAD_VULKAN
  glad_vkEnumerateInstanceLayerProperties(&layerCount, NULL);
  VkLayerProperties availableLayers [layerCount];
  glad_vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);
#else
  vkEnumerateInstanceLayerProperties(&layerCount, NULL);
  VkLayerProperties availableLayers [layerCount];
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);
#endif

  uint32_t validationLayerCount = sizeof(validationLayers) / sizeof(char *);

  for (int i = 0; i < validationLayerCount; i++) {
    int layerFound = 0;

    for (int j = 0; j < layerCount; j++) {

    }
  }

  return 0;
}
#endif

int initWindow(pAsobj obj) {
  int code = 1;
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  if (!(obj->window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", NULL, NULL))) {
    fprintf(stderr, "Failed to create window\n");
  }
  return code;
}

void initGLADLibraries(pAsobj obj) {
#ifdef GLAD_VULKAN
  #ifdef DEBUG
  fprintf(stdout, "initializing GLAD libraries with:\n");
  fprintf(stdout, "physical device: 0x%" PRIx64 "\ninstance:        0x%" PRIx64 "\n", obj->physicalDevice, obj->instance);
  //lol what is this
  int version = 
  #endif
  gladLoadVulkanUserPtr(obj->physicalDevice, (GLADuserptrloadfunc) glfwGetInstanceProcAddress, obj->instance);
  #ifdef DEBUG
  fprintf(stdout, "%d\n", version);
  #endif
#endif
}

int createVulkanInstance(pAsobj obj) {
  int code = 1;
  VkApplicationInfo appInfo = {0};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Vulkan Tutorial";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "unengine";
  appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
  appInfo.apiVersion = VK_API_VERSION_1_3;
  appInfo.pNext = NULL;

  VkInstanceCreateInfo createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.ppEnabledExtensionNames = glfwGetRequiredInstanceExtensions(&createInfo.enabledExtensionCount);
  createInfo.enabledExtensionCount;
  createInfo.ppEnabledLayerNames = NULL;
  createInfo.enabledLayerCount = 0;
  createInfo.flags = 0;
  createInfo.pNext = NULL;

  uint32_t extensionCount= 0;
  #ifdef GLAD_VULKAN
  glad_vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
  VkExtensionProperties extensions [extensionCount];
  glad_vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensions);
  #else
  vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
  VkExtensionProperties extensions [extensionCount];
  vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensions);
  #endif
  
  fprintf(stdout, "Available extensions:\n\n");
  for (int i = 0; i < extensionCount; i++) {
    fprintf(stdout, "\t%s\n", extensions[i].extensionName);
  }
  fprintf(stdout, "\n");

#ifdef GLAD_VULKAN
  if (glad_vkCreateInstance(&createInfo, NULL, &obj->instance) != VK_SUCCESS) {
    fprintf(stderr, "failed to create instance\n");
    code = 0;
  }
#else
  if (vkCreateInstance(&createInfo, NULL, &obj->instance) != VK_SUCCESS) {
    fprintf(stderr, "failed to create instance\n");
    code = 0;
  }
#endif
  return code;
}

int initVulkan(pAsobj obj) {
  int code = 1;
  if (code) code = createVulkanInstance(obj);
  return code;
}

int mainLoop(pAsobj obj) {
  int code = 1;
  while(!glfwWindowShouldClose(obj->window)) {
    glfwPollEvents();
  }
  return code;
}

void cleanup(pAsobj obj) {
#ifdef GLAD_VULKAN
  //If we're using glad, then we want to use the glad functions
  glad_vkDestroyInstance(obj->instance, NULL);
#else
  //Otherwise, we use the traditional function names
  vkDestroyInstance(obj->instance, NULL);
#endif
  if(obj->window) glfwDestroyWindow(obj->window);
  glfwTerminate();
}

int run(pAsobj obj) {
  int code = 1;
  if (code) code = initWindow(obj);
#ifdef GLAD_VULKAN
  if (code) initGLADLibraries(obj);
#endif
  if (code) code = initVulkan(obj);
  if (code) code = mainLoop(obj);
  cleanup(obj);
  return code;
}

int main(void) {
  AppStruct app = {0};
  int code = run(&app);
  return !code;
}