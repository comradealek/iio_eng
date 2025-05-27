#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "iio_vulkan_api.h"
#include "iio_eng_errors.h"

/****************************************************************************************************
 *                                         Application State                                        *
 ****************************************************************************************************/

IIOVulkanState state = {0};

/****************************************************************************************************
 *                                             Constants                                            *
 ****************************************************************************************************/

const char * deviceExtensions [] = {
  "VK_KHR_swapchain",
};

/****************************************************************************************************
 *                           Functions for initializing the Vulkan API                              *
 ****************************************************************************************************/

IIOVulkanState * iio_init_vulkan_api() {
  atexit(iio_cleanup);
  return &state;
}

void iio_init_vulkan() {
  glfwInit();
  iio_create_instance();
  iio_create_window();
  iio_create_surface();
  iio_select_physical_device();


  iio_create_device();
  iio_create_swapchain();
  iio_create_command_pool();
  iio_create_command_buffer();
  iio_create_render_pass();
}

void iio_create_instance() {
  uint32_t extensionCount = 0;
  const char ** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
  uint32_t glfwcode = glfwGetError(NULL);
  if (glfwcode != GLFW_NO_ERROR){
    iio_glfw_error(glfwcode, __LINE__, __FILE__);
    exit(1);
  }

  VkInstanceCreateInfo createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.enabledExtensionCount = extensionCount;
  createInfo.ppEnabledExtensionNames = extensions;


  VkAllocationCallbacks * allocCallbacks = NULL;

  VkResult result = vkCreateInstance(&createInfo, allocCallbacks, &state.instance);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }
}

void iio_create_window() {
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  state.window = glfwCreateWindow(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, "IIO Vulkan", NULL, NULL);
  if (!state.window) {
    iio_glfw_error(glfwGetError(NULL), __LINE__, __FILE__);
    exit(1);
  }
  glfwSetWindowUserPointer(state.window, &state);
}

void iio_create_surface() {
  VkResult result = glfwCreateWindowSurface(state.instance, state.window, NULL, &state.surface);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }
  uint32_t glfwcode = glfwGetError(NULL);
  if (glfwcode != GLFW_NO_ERROR){
    iio_glfw_error(glfwcode, __LINE__, __FILE__);
    exit(1);
  }
}

void iio_select_physical_device() {
  //  count the number of devices. If there are no devices, exit
  uint32_t deviceCount = 0;
  VkResult result = vkEnumeratePhysicalDevices(state.instance, &deviceCount, NULL);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }
  if (deviceCount == 0) {
    fprintf(stderr, "No physical devices found\n");
    exit(1);
  }

  //  put the devices in an array. Save the array to the application state
  state.physicalDevices = malloc(deviceCount * sizeof(VkPhysicalDevice));
  if (!state.physicalDevices) {
    iio_oom_error(NULL, __LINE__, __FILE__);
    exit(1);
  }
  result = vkEnumeratePhysicalDevices(state.instance, &deviceCount, state.physicalDevices);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }

  //  initialize the data for the preferred device
  int preferredDevice = -1;
  VkPhysicalDeviceType preferredDeviceType = 0;
  // QueueFamilyIndices indicesPerDevice [deviceCount];
  uint32_t presentQueueIndexPerDevice [deviceCount];
  uint32_t graphicsQueueIndexPerDevice [deviceCount];
  VkSurfaceFormatKHR preferredSurfaceFormatPerDevice [deviceCount];
  VkPresentModeKHR preferredPresentModePerDevice [deviceCount];
  VkSurfaceCapabilitiesKHR surfaceCapabilitiesPerDevice [deviceCount];
  VkSurfaceFormatKHR preferredSurfaceFormatPerDevice [deviceCount];
  VkPresentModeKHR preferredPresentModePerDevice [deviceCount];

  //  begin iterating over the device array
  for (uint32_t i = 0; i < deviceCount; i++) {
    //  check if the device has a graphics and present family. If not, skip it

    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(state.physicalDevices[i], &queueFamilyCount, NULL);
    if (queueFamilyCount == 0) {
      continue;
    }
    VkQueueFamilyProperties queueFamilies [queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(state.physicalDevices[i], &queueFamilyCount, queueFamilies);
    char hasGraphicsFamily = 0;
    char hasPresentFamily = 0;

    for (int j = 0; j < queueFamilyCount; j++) {
      //  iterate through the queue families and check if the device has a graphics and present family
      if (queueFamilies[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        graphicsQueueIndexPerDevice[i] = j;
        hasGraphicsFamily = 1;
      }
      VkBool32 presentSupport = 0;
      vkGetPhysicalDeviceSurfaceSupportKHR(state.physicalDevices[i], j, state.surface, &presentSupport);
      if (presentSupport) {
        presentQueueIndexPerDevice[i] = j;
        hasPresentFamily = 1;
      }
      if (graphicsQueueIndexPerDevice[i] == presentQueueIndexPerDevice[i]) {
        break;
      }
    }
    if (!hasGraphicsFamily || !hasPresentFamily) {
      continue;
    }

    //  check if the device has the required extensions. If not, skip it
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(state.physicalDevices[i], NULL, &extensionCount, NULL);
    if (extensionCount == 0) {
      continue;
    }
    VkExtensionProperties extensions [extensionCount];
    vkEnumerateDeviceExtensionProperties(state.physicalDevices[i], NULL, &extensionCount, extensions);
    uint32_t requiredExtensionsPresent = 0;
    uint32_t bookmark = extensionCount - 1;
    for (int i = 0; ; i = (i + 1) % extensionCount) {
      if (strcmp(extensions[i].extensionName, deviceExtensions[requiredExtensionsPresent]) == 0) {
        requiredExtensionsPresent += 1;
        bookmark = i;
      } else if (bookmark == i) {
        break;
      }
      if (requiredExtensionsPresent >= sizeof(deviceExtensions) / sizeof(char *)) {
        break;
      }
    }
    if ((requiredExtensionsPresent != sizeof(deviceExtensions) / sizeof(char *))) {
      continue;
    }

    //  get the surface capabilities, formats and present modes

    //  get the surface capabilities for the device
    VkSurfaceCapabilitiesKHR surfaceCapabilities = {0};
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(state.physicalDevices[i], state.surface, &surfaceCapabilities);
    if (result != VK_SUCCESS) {
      iio_vk_error(result, __LINE__, __FILE__);
      exit(1);
    }
    surfaceCapabilitiesPerDevice[i] = surfaceCapabilities;

    //  get the surface formats for the device
    uint32_t surfaceFormatCount = 0;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(state.physicalDevices[i], state.surface, &surfaceFormatCount, NULL);
    if (result != VK_SUCCESS) {
      iio_vk_error(result, __LINE__, __FILE__);
      exit(1);
    }
    if (surfaceFormatCount == 0) {
      continue;
    }
    VkSurfaceFormatKHR surfaceFormats [surfaceFormatCount];
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(state.physicalDevices[i], state.surface, &surfaceFormatCount, surfaceFormats);
    if (result != VK_SUCCESS) {
      iio_vk_error(result, __LINE__, __FILE__);
      exit(1);
    }
    for (int j = 0; j < surfaceFormatCount; j++) {
      if (surfaceFormats[j].format == VK_FORMAT_B8G8R8A8_SRGB &&
          surfaceFormats[j].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        preferredSurfaceFormatPerDevice[i] = surfaceFormats[j];
        break;
      } else if (j == surfaceFormatCount - 1) {
        preferredSurfaceFormatPerDevice[i] = surfaceFormats[0];
      }
    }

    //  get the surface present modes for the device
    uint32_t presentModeCount = 0;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(state.physicalDevices[i], state.surface, &presentModeCount, NULL);
    if (result != VK_SUCCESS) {
      iio_vk_error(result, __LINE__, __FILE__);
      exit(1);
    }
    if (presentModeCount == 0) {
      continue;
    }
    VkPresentModeKHR presentModes [presentModeCount];
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(state.physicalDevices[i], state.surface, &presentModeCount, presentModes);
    if (result != VK_SUCCESS) {
      iio_vk_error(result, __LINE__, __FILE__);
      exit(1);
    }
    for (int j = 0; j < presentModeCount; j++) {
      if (presentModes[j] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
        preferredPresentModePerDevice[i] = presentModes[j];
        break;
      } else if (presentModes[j] == VK_PRESENT_MODE_MAILBOX_KHR) {
        preferredPresentModePerDevice[i] = presentModes[j];
      } else if (presentModes[j] == VK_PRESENT_MODE_FIFO_KHR &&
                 preferredPresentModePerDevice[i] != VK_PRESENT_MODE_MAILBOX_KHR) {
        preferredPresentModePerDevice[i] = presentModes[j];
      } else if (presentModes[j] == VK_PRESENT_MODE_FIFO_RELAXED_KHR &&
                 preferredPresentModePerDevice[i] != VK_PRESENT_MODE_MAILBOX_KHR &&
                 preferredPresentModePerDevice[i] != VK_PRESENT_MODE_FIFO_KHR) {
        preferredPresentModePerDevice[i] = presentModes[j];
      } else if (j == presentModeCount - 1) {
        preferredPresentModePerDevice[i] = presentModes[0];
      }
    }

    //  check if the device is a discrete GPU, integrated GPU or virtual GPU
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(state.physicalDevices[i], &properties);
    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      preferredDevice = i;
      preferredDeviceType = properties.deviceType;
      break;
    } else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
      if (preferredDeviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
        preferredDevice = i;
        preferredDeviceType = properties.deviceType;
      }
    } else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) {
      if (preferredDeviceType != VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU &&
          preferredDeviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
        preferredDevice = i;
        preferredDeviceType = properties.deviceType;
      }
    } else if (preferredDeviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
               preferredDeviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU &&
               preferredDeviceType != VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) {
      preferredDevice = i;
      preferredDeviceType = properties.deviceType;
    }
  }
  //  finish iterating over the device array

  if (preferredDevice == -1) {
    fprintf(stderr, "No suitable physical device found\n");
    exit(1);
  }

  state.graphicsQueueFamilyIndex = graphicsQueueIndexPerDevice[preferredDevice];
  state.presentQueueFamilyIndex = presentQueueIndexPerDevice[preferredDevice];
  state.selectedDevice = state.physicalDevices[preferredDevice];
}

void iio_create_device() {
  uint32_t queueCreateFamilyCount = 2;
  uint32_t queueFamilyIndices [queueCreateFamilyCount];
  queueFamilyIndices[0] = state.graphicsQueueFamilyIndex;
  if (queueCreateFamilyCount == 2) {
    queueFamilyIndices[1] = state.presentQueueFamilyIndex;
  }
  uint32_t queueCreateInfoCount = queueCreateFamilyCount;

  VkDeviceQueueCreateInfo queueCreateInfos [queueCreateInfoCount];
  for (int i = 0; i < queueCreateInfoCount; i++) {
    queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfos[i].pNext = NULL;
    queueCreateInfos[i].flags = 0;
    queueCreateInfos[i].queueFamilyIndex = queueFamilyIndices[i];
    queueCreateInfos[i].queueCount = 1;
    queueCreateInfos[i].pQueuePriorities = (float []) {1.0f};
  }
  

  VkDeviceCreateInfo deviceCreateInfo = {0};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.queueCreateInfoCount = queueCreateInfoCount;
  deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
  deviceCreateInfo.enabledExtensionCount = 1;
  deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;
  deviceCreateInfo.pEnabledFeatures = NULL;

  VkResult result = vkCreateDevice(state.selectedDevice, &deviceCreateInfo, NULL, &state.device);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }
  vkGetDeviceQueue(state.device, state.graphicsQueueFamilyIndex, 0, &state.graphicsQueue);
  vkGetDeviceQueue(state.device, state.presentQueueFamilyIndex, 0, &state.presentQueue);
}

void iio_create_swapchain() {
  VkResult result;

  //  get the surface capabilities, formats and present modes
  VkSurfaceCapabilitiesKHR surfaceCapabilities = {0};
  result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(state.selectedDevice, state.surface, &surfaceCapabilities);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }
  
  uint32_t surfaceFormatCount = 0;
  result = vkGetPhysicalDeviceSurfaceFormatsKHR(state.selectedDevice, state.surface, &surfaceFormatCount, NULL);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }
  if (surfaceFormatCount == 0) {
    fprintf(stderr, "No surface formats found\n");
    exit(1);
  }
  VkSurfaceFormatKHR surfaceFormats [surfaceFormatCount];
  result = vkGetPhysicalDeviceSurfaceFormatsKHR(state.selectedDevice, state.surface, &surfaceFormatCount, surfaceFormats);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }

  uint32_t presentModeCount = 0;
  result = vkGetPhysicalDeviceSurfacePresentModesKHR(state.selectedDevice, state.surface, &presentModeCount, NULL);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }
  if (presentModeCount == 0) {
    fprintf(stderr, "No present modes found\n");
    exit(1);
  }
  VkPresentModeKHR presentModes [presentModeCount];
  result = vkGetPhysicalDeviceSurfacePresentModesKHR(state.selectedDevice, state.surface, &presentModeCount, presentModes);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }

  //  

  VkSwapchainCreateInfoKHR swapchainCreateInfo = {0};
  swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchainCreateInfo.pNext = NULL;
  swapchainCreateInfo.flags = 0;
  swapchainCreateInfo.surface = state.surface;
  swapchainCreateInfo.minImageCount = 3; // Triple buffering
  swapchainCreateInfo.imageFormat = VK_FORMAT_B8G8R8A8_SRGB; // TODO: make this configurable
  swapchainCreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR; // TODO: make this configurable
  swapchainCreateInfo.imageExtent.width = DEFAULT_WINDOW_WIDTH; // TODO: make this configurable
  swapchainCreateInfo.imageExtent.height = DEFAULT_WINDOW_HEIGHT; // TODO: make this configurable
  swapchainCreateInfo.imageArrayLayers = 1;
  swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // No need for concurrent access
  swapchainCreateInfo.queueFamilyIndexCount = 0; // No need for concurrent access
  swapchainCreateInfo.pQueueFamilyIndices = NULL; // No need for concurrent access
  swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; // No transformation
  swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // No alpha blending
  swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; // VSync
  swapchainCreateInfo.clipped = VK_TRUE; // No need to clip
  swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE; // No previous swapchain

  result = vkCreateSwapchainKHR(state.device, &swapchainCreateInfo, NULL, &state.swapChain);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }
}

void iio_create_command_pool() {
  VkCommandPoolCreateInfo poolCreateInfo = {0};
  poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolCreateInfo.pNext = NULL;
  poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolCreateInfo.queueFamilyIndex = state.graphicsQueueFamilyIndex;

  VkResult result = vkCreateCommandPool(state.device, &poolCreateInfo, NULL, &state.commandPool);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }
}

void iio_create_command_buffer() {
  VkCommandBufferAllocateInfo allocateInfo = {0};
  allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocateInfo.pNext = NULL;
  allocateInfo.commandPool = state.commandPool;
  allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocateInfo.commandBufferCount = 1;

  VkResult result = vkAllocateCommandBuffers(state.device, &allocateInfo, &state.commandBuffer);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }
}

void iio_create_render_pass() {
  VkAttachmentDescription attachmentDescription = {0};
  attachmentDescription.flags = 0;
  attachmentDescription.format = VK_FORMAT_UNDEFINED; // No attachments for now
  attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
  attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // No attachments for now
  attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // No attachments for now
  attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // No attachments for now
  attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // No attachments for now
  attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // No attachments for now
  attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_UNDEFINED; // No attachments for now

  VkSubpassDescription subpassDescription = {0};
  subpassDescription.flags = 0;
  subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassDescription.inputAttachmentCount = 0; // No input attachments for now
  subpassDescription.pInputAttachments = NULL; // No input attachments for now
  subpassDescription.colorAttachmentCount = 0; // No color attachments for now
  subpassDescription.pColorAttachments = NULL; // No color attachments for now
  subpassDescription.pResolveAttachments = NULL; // No resolve attachments for now
  subpassDescription.pDepthStencilAttachment = NULL; // No depth/stencil attachments for now
  subpassDescription.preserveAttachmentCount = 0; // No preserve attachments for now
  subpassDescription.pPreserveAttachments = NULL; // No preserve attachments for now

  VkSubpassDependency subpassDependency = {0};

  VkRenderPassCreateInfo renderPassCreateInfo = {0};
  renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassCreateInfo.pNext = NULL;
  renderPassCreateInfo.flags = 0;
  renderPassCreateInfo.attachmentCount = 0; // No attachments for now
  renderPassCreateInfo.pAttachments = NULL;
  renderPassCreateInfo.subpassCount = 0; // No subpasses for now
  renderPassCreateInfo.pSubpasses = NULL;
  renderPassCreateInfo.dependencyCount = 0; // No dependencies for now
  renderPassCreateInfo.pDependencies = NULL;

  VkResult result = vkCreateRenderPass(state.device, &renderPassCreateInfo, NULL, &state.renderPass);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }
}

/****************************************************************************************************
 *                                      Vulkan Helper Functions                                     *
 ****************************************************************************************************/



/****************************************************************************************************
 *                                      GLFW specific callbacks                                     *
 ****************************************************************************************************/



/****************************************************************************************************
 *                                              Runtime                                             *
 ****************************************************************************************************/

void iio_run() {
  while (!glfwWindowShouldClose(state.window)) {
    glfwPollEvents();
  }
}

void iio_record_command_buffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags flags) {
  VkResult result;
  VkCommandBufferBeginInfo beginInfo = {0};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.pNext = NULL;
  beginInfo.flags = flags;
  beginInfo.pInheritanceInfo = NULL;
  
  result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }

  

  result = vkEndCommandBuffer(commandBuffer);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }
}

/****************************************************************************************************
 *                                      Cleanup the Vulkan API                                      *
 ****************************************************************************************************/

void iio_cleanup() {
  if (state.renderPass) vkDestroyRenderPass(state.device, state.renderPass, NULL);
  if (state.commandBuffer) vkFreeCommandBuffers(state.device, state.commandPool, 1, &state.commandBuffer);
  if (state.commandPool) vkDestroyCommandPool(state.device, state.commandPool, NULL);
  if (state.swapChain) vkDestroySwapchainKHR(state.device, state.swapChain, NULL);
  if (state.device) vkDestroyDevice(state.device, NULL);
  if (state.physicalDevices) free(state.physicalDevices);
  if (state.surface) vkDestroySurfaceKHR(state.instance, state.surface, NULL);
  if (state.window) glfwDestroyWindow(state.window);
  glfwTerminate();
  if (state.instance) vkDestroyInstance(state.instance, NULL);
}