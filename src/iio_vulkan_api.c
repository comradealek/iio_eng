#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined(__linux__) || defined(linux) || defined(__linux) || defined(__gnu_linux__)
#include <unistd.h>
#elif defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)
#include <windows.h>
#endif

#include <time.h>
#include <math.h>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "cglm/cglm.h"
#include "stb_image.h"
#include "iio_eng_typedef.h"
#include "iio_vulkan_api.h"
#include "iio_eng_errors.h"
#include "iio_resource_loaders.h"
#include "iio_pipeline.h"



/****************************************************************************************************
 *                                         Application State                                        *
 ****************************************************************************************************/

IIOVulkanState state = {0};

IIOVulkanCamera camera = {
  .position = {0.0f, 0.0f, 3.0f},
  .front = {0.0f, 0.0f, -1.0f},
  .up = {0.0f, 1.0f, 0.0f},
  .right = {1.0f, 0.0f, 0.0f},
  .forward = {0.0f, 0.0f, -1.0f},
  .yaw = -90.0f,
  .pitch = 0.0f,
  .fov = 45.0f
};

double deltaTime = 0.0;

/****************************************************************************************************
 *                                             Constants                                            *
 ****************************************************************************************************/

const char * deviceExtensions [] = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
};

const char * validationLayers [] = {
  "VK_LAYER_KHRONOS_validation",
};

const bool enableValidationLayers = true;

// int MAX_FRAMES_IN_FLIGHT = 2;

TestCubeData testCube = {
  .isInitialized = 0,
  .vertices = {
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, //  front bottom left
    {{ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}, //  front bottom right
    {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}, //  front top left
    {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}, //  front top right

    {{-0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, //  back bottom left
    {{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, //  back bottom right
    {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, //  back top left
    {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}, //  back top right
  },
  .indices = {
    0, 1, 3, // front face
    0, 3, 2,
    4, 7, 5, // back face
    4, 6, 7,
    0, 2, 4, // left face
    2, 6, 4,
    1, 5, 3, // right face
    5, 7, 3,
    0, 4, 1, // bottom face
    4, 5, 1,
    2, 3, 6, // top face
    3, 7, 6,
  }

};

const float cameraSpeed = 5.0f;

const char * testTexturePath = "src/textures/269670.png";
const char * testTextureFilename = "269670.png";

const bool doTestTriangle = false;
const bool doTestCube = !doTestTriangle;

/****************************************************************************************************
 *                           Functions for initializing the Vulkan API                              *
 ****************************************************************************************************/

IIOVulkanState * iio_init_vulkan_api() {
  memset(&state, 0, sizeof(IIOVulkanState));
  return &state;
}

void iio_init_vulkan() {
  glfwInit();
  //  requires GLFW
  iio_create_instance();
  iio_create_window();
  //  requires instance
  iio_create_surface();
  iio_select_physical_device();
  //  requires physical device
  iio_create_device();
  //  requires logical device
  iio_create_swapchain();
  iio_create_swapchain_image_views();
  iio_create_command_pool();
  iio_initialize_resource_loader();
  iio_create_depth_resources();
  iio_create_command_buffers();
  iio_create_synchronization_objects();

  if (doTestTriangle) {
    iio_create_graphics_pipeline_testtriangle();
  } else if (doTestCube) {
    iio_create_descriptor_pool_managers_testcube();
    iio_create_graphics_pipeline_testcube();
    iio_initialize_testcube();
    iio_initialize_camera();
  } else {
    iio_create_application_descriptor_pool_managers();
    iio_create_application_graphics_pipeline();
  }
}

void iio_create_instance() {
  fprintf(stdout, "Creating Vulkan instance.\n");
  uint32_t extensionCount = 0;
  const char ** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
  uint32_t glfwcode = glfwGetError(NULL);
  if (glfwcode != GLFW_NO_ERROR){
    iio_glfw_error(glfwcode, __LINE__, __FILE__);
    exit(1);
  }

  if (enableValidationLayers) {
    uint32_t propertyCount;
    vkEnumerateInstanceLayerProperties(&propertyCount, NULL);
    VkLayerProperties layersProperties [propertyCount];
    vkEnumerateInstanceLayerProperties(&propertyCount, layersProperties);
    bool layerFound = false;
    for (int i = 0; i < propertyCount; i++) {
      if (strcmp(validationLayers[0], layersProperties[i].layerName) == 0) {
        layerFound = true;
        break;
      }
    }
    if (!layerFound) {
      fprintf(stdout, "Validation layers requested but not found\n");
      exit(1);
    }
  }

  VkApplicationInfo appInfo = {0};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.apiVersion = VK_MAKE_API_VERSION(0, 1, 3, 0);
  appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 0, 0, 1);
  appInfo.pEngineName = "IIO";
  appInfo.engineVersion = VK_MAKE_API_VERSION(0, 0, 0, 1);
  appInfo.pApplicationName = "void";

  VkInstanceCreateInfo createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.enabledExtensionCount = extensionCount;
  createInfo.ppEnabledExtensionNames = extensions;
  createInfo.pApplicationInfo = &appInfo;
  if (enableValidationLayers == true) {
    createInfo.enabledLayerCount = 1;
    createInfo.ppEnabledLayerNames = validationLayers;
  }


  VkAllocationCallbacks * allocCallbacks = NULL;

  VkResult result = vkCreateInstance(&createInfo, allocCallbacks, &state.instance);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }
}

void iio_create_window() {
  fprintf(stdout, "Creating GLFW window.\n");
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  state.window = glfwCreateWindow(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, "IIO Vulkan Window", NULL, NULL);
  if (!state.window) {
    iio_glfw_error(glfwGetError(NULL), __LINE__, __FILE__);
    exit(1);
  }
  glfwSetWindowUserPointer(state.window, &state);
  glfwSetFramebufferSizeCallback(state.window, iio_framebuffer_size_callback);
  glfwSetMouseButtonCallback(state.window, iio_mouse_button_callback);
  glfwSetCursorPosCallback(state.window, iio_cursor_position_callback);
  glfwSetScrollCallback(state.window, iio_scroll_callback);
  // glfwSetKeyCallback(state.window, iio_key_callback);
  glfwSetInputMode(state.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void iio_create_surface() {
  fprintf(stdout, "Creating Vulkan surface.\n");
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
  fprintf(stdout, "Selecting physical device.\n");
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
  uint32_t presentQueueIndexPerDevice [deviceCount];
  uint32_t graphicsQueueIndexPerDevice [deviceCount];
  VkSurfaceFormatKHR preferredSurfaceFormatPerDevice [deviceCount];
  VkPresentModeKHR preferredPresentModePerDevice [deviceCount];
  VkSurfaceCapabilitiesKHR surfaceCapabilitiesPerDevice [deviceCount];
  VkPhysicalDeviceType deviceTypePerDevice [deviceCount];

  //  begin iterating over the device array
  for (uint32_t i = 0; i < deviceCount; i++) {
    iio_select_physical_device_properties(
      state.physicalDevices[i],
      &preferredSurfaceFormatPerDevice[i],
      &preferredPresentModePerDevice[i],
      &surfaceCapabilitiesPerDevice[i],
      &graphicsQueueIndexPerDevice[i],
      &presentQueueIndexPerDevice[i],
      &deviceTypePerDevice[i]
    );

    //  check if the device is a discrete GPU, integrated GPU or virtual GPU
    if (deviceTypePerDevice[i] == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      preferredDevice = i;
      preferredDeviceType = deviceTypePerDevice[i];
      break;
    } else if (deviceTypePerDevice[i] == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
      if (preferredDeviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
        preferredDevice = i;
        preferredDeviceType = deviceTypePerDevice[i];
      }
    } else if (deviceTypePerDevice[i] == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) {
      if (preferredDeviceType != VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU &&
          preferredDeviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
        preferredDevice = i;
        preferredDeviceType = deviceTypePerDevice[i];
      }
    }
  }
  //  finish iterating over the device array

  if (preferredDevice == -1) {
    fprintf(stderr, "No suitable physical device found\n");
    exit(1);
  }

  state.surfaceFormat = preferredSurfaceFormatPerDevice[preferredDevice];
  state.presentMode = preferredPresentModePerDevice[preferredDevice];
  state.surfaceCapabilities = surfaceCapabilitiesPerDevice[preferredDevice];
  state.graphicsQueueFamilyIndex = graphicsQueueIndexPerDevice[preferredDevice];
  state.presentQueueFamilyIndex = presentQueueIndexPerDevice[preferredDevice];
  state.selectedDevice = state.physicalDevices[preferredDevice];
}

void iio_create_device() {
  fprintf(stdout, "Creating logical device.\n");
  uint32_t queueCreateFamilyCount = 2;
  uint32_t queueFamilyIndices [queueCreateFamilyCount];
  queueFamilyIndices[0] = state.graphicsQueueFamilyIndex;
  if (state.graphicsQueueFamilyIndex == state.presentQueueFamilyIndex) {
    queueCreateFamilyCount = 1;
  }
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
  
  VkPhysicalDeviceFeatures deviceFeatures = {0};
  deviceFeatures.samplerAnisotropy = VK_TRUE; // Enable anisotropic filtering

  VkDeviceCreateInfo deviceCreateInfo = {0};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.queueCreateInfoCount = queueCreateInfoCount;
  deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
  deviceCreateInfo.enabledExtensionCount = 1;
  deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;
  deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

  VkPhysicalDeviceVulkan11Features vk11features = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
  };

  VkPhysicalDeviceVulkan12Features vk12features = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
    .pNext = &vk11features,
  };

  VkPhysicalDeviceVulkan13Features vk13features = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
    .pNext = &vk12features,
    .dynamicRendering = VK_TRUE,
  };

  VkPhysicalDeviceVulkan14Features vk14features = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
    .pNext = &vk13features,
  };

  deviceCreateInfo.pNext = &vk14features;

  VkResult result = vkCreateDevice(state.selectedDevice, &deviceCreateInfo, NULL, &state.device);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }
  vkGetDeviceQueue(state.device, state.graphicsQueueFamilyIndex, 0, &state.graphicsQueue);
  vkGetDeviceQueue(state.device, state.presentQueueFamilyIndex, 0, &state.presentQueue);
  uint32_t instanceVersion;
  vkEnumerateInstanceVersion(&instanceVersion);
  fprintf(stdout, "Vulkan version is %u.%u.%u\n", VK_VERSION_MAJOR(instanceVersion), VK_VERSION_MINOR(instanceVersion), VK_VERSION_PATCH(instanceVersion));
}

void iio_create_swapchain() {
  // fprintf(stdout, "Creating swapchain.\n");
  VkResult result;

  //  choose the minimum image count for the swapchain
  uint32_t minImageCount = 0;
  if (state.surfaceCapabilities.maxImageCount == 0) {
    //  if the max image count is 0, it means there is no limit, so we can use a minimum of 2 images
    minImageCount = max(2, state.surfaceCapabilities.minImageCount);
  } else {
    //  otherwise, we can clamp the minimum image count to the range of min and max image counts
    minImageCount = clamp(2, state.surfaceCapabilities.minImageCount, state.surfaceCapabilities.maxImageCount);
  }

  //  choose the image format and color space
  VkFormat imageFormat = state.surfaceFormat.format;
  VkColorSpaceKHR imageColorSpace = state.surfaceFormat.colorSpace;

  //  choose the image extent
  if (state.surfaceCapabilities.currentExtent.width == (uint32_t)-1) {
    int width, height;
    glfwGetFramebufferSize(state.window, &width, &height);
    state.swapChainImageExtent.width = clamp(width, state.surfaceCapabilities.minImageExtent.width, state.surfaceCapabilities.maxImageExtent.width);
    state.swapChainImageExtent.height = clamp(height, state.surfaceCapabilities.minImageExtent.height, state.surfaceCapabilities.maxImageExtent.height);
  } else {
    //  otherwise, use the current extent
    state.swapChainImageExtent = state.surfaceCapabilities.currentExtent;
  }

  VkSwapchainCreateInfoKHR swapchainCreateInfo = {0};
  swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchainCreateInfo.pNext = NULL;
  swapchainCreateInfo.flags = 0;
  swapchainCreateInfo.surface = state.surface;
  swapchainCreateInfo.minImageCount = minImageCount;
  swapchainCreateInfo.imageFormat = imageFormat; // TODO: make this configurable
  swapchainCreateInfo.imageColorSpace = imageColorSpace; // TODO: make this configurable
  swapchainCreateInfo.imageExtent.width = state.swapChainImageExtent.width; // TODO: make this configurable
  swapchainCreateInfo.imageExtent.height = state.swapChainImageExtent.height; // TODO: make this configurable
  swapchainCreateInfo.imageArrayLayers = 1;
  swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  if (state.graphicsQueueFamilyIndex != state.presentQueueFamilyIndex) {
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // Need concurrent access
    swapchainCreateInfo.queueFamilyIndexCount = 2; // Two queue families
    swapchainCreateInfo.pQueueFamilyIndices = (uint32_t []) {state.graphicsQueueFamilyIndex, state.presentQueueFamilyIndex}; // Two queue families
  } else {
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // No need for concurrent access
    swapchainCreateInfo.queueFamilyIndexCount = 0; // No need for concurrent access
    swapchainCreateInfo.pQueueFamilyIndices = NULL; // No need for concurrent access
  }
  swapchainCreateInfo.preTransform = state.surfaceCapabilities.currentTransform; // Use the current transform
  swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // No alpha blending
  swapchainCreateInfo.presentMode = state.presentMode; // Use the preferred present mode
  swapchainCreateInfo.clipped = VK_TRUE; // No need to clip
  swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE; // No previous swapchain

  result = vkCreateSwapchainKHR(state.device, &swapchainCreateInfo, NULL, &state.swapChain);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }

  vkGetSwapchainImagesKHR(state.device, state.swapChain, &state.swapChainImageCount, NULL);
  if (state.swapChainImageCount == 0) {
    fprintf(stderr, "No swapchain images found\n");
    exit(1);
  }
  
  state.swapChainImages = malloc(state.swapChainImageCount * sizeof(VkImage));
  if (!state.swapChainImages) {
    iio_oom_error(NULL, __LINE__, __FILE__);
    exit(1);
  }
  result = vkGetSwapchainImagesKHR(state.device, state.swapChain, &state.swapChainImageCount, state.swapChainImages);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }
}

void iio_create_swapchain_image_views() {
  // fprintf(stdout, "Creating swapchain image views.\n");
  state.swapChainImageViews = malloc(state.swapChainImageCount * sizeof(VkImageView));
  if (!state.swapChainImageViews) {
    iio_oom_error(NULL, __LINE__, __FILE__);
    exit(1);
  }
  for (int i = 0; i < state.swapChainImageCount; i++) {
    iio_create_image_view(
      state.swapChainImages[i],
      &state.swapChainImageViews[i],
      state.surfaceFormat.format,
      VK_IMAGE_ASPECT_COLOR_BIT
    );
  }
}

void iio_create_application_descriptor_pool_managers() {
  fprintf(stdout, "creating descriptor pool managers for application\n");
  int descriptorPoolSetCount = 3;
  state.descriptorPoolManagerCount = descriptorPoolSetCount;
  state.descriptorPoolMangers = malloc(sizeof(IIODescriptorPoolManager) * descriptorPoolSetCount);

  iio_create_descriptor_pool_manager(
    state.device,
    1, (IIODescriptorLayoutElement []) {
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT}
    },
    MAX_FRAMES_IN_FLIGHT, 2,
    &state.descriptorPoolMangers[0]
  );

  iio_create_descriptor_pool_manager(
    state.device,
    2, (IIODescriptorLayoutElement []) {
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5, VK_SHADER_STAGE_VERTEX_BIT},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT}
    },
    MAX_FRAMES_IN_FLIGHT, 2,
    &state.descriptorPoolMangers[1]
  );

  iio_create_descriptor_pool_manager(
    state.device,
    1, (IIODescriptorLayoutElement []) {
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT}
    },
    MAX_FRAMES_IN_FLIGHT, 2,
    &state.descriptorPoolMangers[2]
  );

  // TODO remove this when the camera api is fully implemented
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    state.cameraDescriptorSets[i] = iio_allocate_descriptor_set(state.device, &state.descriptorPoolMangers[0]);
  }
}

void iio_create_descriptor_pool_managers_testcube() {
  fprintf(stdout, "creating descriptor pool managers\n");
  state.descriptorPoolManagerCount = 3;
  state.descriptorPoolMangers = malloc(sizeof(IIODescriptorPoolManager) * 3);

  fprintf(stdout, "creating descriptor pool 1\n");
  iio_create_descriptor_pool_manager(
    state.device,
    1, (IIODescriptorLayoutElement []) {
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT}
    },
    MAX_FRAMES_IN_FLIGHT, 2,
    &state.descriptorPoolMangers[0]
  );

  fprintf(stdout, "creating descriptor pool 2\n");
  iio_create_descriptor_pool_manager(
    state.device,
    1, (IIODescriptorLayoutElement []) {
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT}
    },
    MAX_FRAMES_IN_FLIGHT, 2,
    &state.descriptorPoolMangers[1]
  );

  fprintf(stdout, "creating descriptor pool 3\n");
  iio_create_descriptor_pool_manager(
    state.device,
    1, (IIODescriptorLayoutElement []) {
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT}
    },
    MAX_FRAMES_IN_FLIGHT, 2,
    &state.descriptorPoolMangers[2]
  );

  state.descriptorPoolMangers[1].descriptorSetLayout;

  fprintf(stdout, "creating descriptor set writer\n");
  iio_create_descriptor_set_writer(&state.descriptorSetWriter);

  fprintf(stdout, "allocating descriptor sets\n");
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    state.cameraDescriptorSets[i] = iio_allocate_descriptor_set(state.device, &state.descriptorPoolMangers[0]);
    testCube.texSamplerDescriptorSets[i] = iio_allocate_descriptor_set(state.device, &state.descriptorPoolMangers[1]);
    testCube.modelUniformBufferDescriptorSets[i] = iio_allocate_descriptor_set(state.device, &state.descriptorPoolMangers[2]);
  }
}

void iio_create_application_graphics_pipeline() {
  fprintf(stdout, "Creating shader pipeline for test cube\n");
  DataBuffer * vertShaderCode = iio_read_shader_file_to_buffer("src/shaders/vertex.spv");
  if (!vertShaderCode) {
    fprintf(stderr, "Failed to read vertex shader file\n");
    exit(1);
  }
  DataBuffer * fragShaderCode = iio_read_shader_file_to_buffer("src/shaders/fragment.spv");
  if (!fragShaderCode) {
    fprintf(stderr, "Failed to read fragment shader file\n");
    free(vertShaderCode);
    exit(1);
  }
  VkShaderModule vertShaderModule = iio_create_shader_module(vertShaderCode);
  if (vertShaderModule == VK_NULL_HANDLE) {
    fprintf(stderr, "Failed to create vertex shader module\n");
    free(vertShaderCode);
    free(fragShaderCode);
    exit(1);
  }
  fprintf(stdout, "Vertex shader module created successfully.\n");
  VkShaderModule fragShaderModule = iio_create_shader_module(fragShaderCode);
  if (fragShaderModule == VK_NULL_HANDLE) {
    fprintf(stderr, "Failed to create fragment shader module\n");
    vkDestroyShaderModule(state.device, vertShaderModule, NULL);
    free(vertShaderCode);
    free(fragShaderCode);
    exit(1);
  }
  fprintf(stdout, "Fragment shader module created successfully.\n");
  free(vertShaderCode);
  free(fragShaderCode);

  fprintf(stdout, "creating graphics pipeline state\n");
  IIOGraphicsPipelineStates pipelineState = iio_create_graphics_pipeline_state();
  
  iio_create_shader_stage_create_info(
    2, 
    (VkShaderStageFlagBits [2]) {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT}, 
    (VkShaderModule [2]) {vertShaderModule, fragShaderModule},
    &pipelineState
  );
  
  iio_set_vertex_input_state_create_info(
    1, &(VkVertexInputBindingDescription) {.binding = 0, .stride = sizeof(Vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},
    3, (VkVertexInputAttributeDescription [3]) {
      {.binding = 0, .location = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, position)},
      {.binding = 0, .location = 1, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, color)},
      {.binding = 0, .location = 2, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, texCoord)}
    },
    &pipelineState
  );

  iio_set_input_assembly_state_create_info( VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false, &pipelineState);

  iio_set_tessellation_state_create_info(&pipelineState);

  iio_set_viewport_state_create_info(1, NULL, 1, NULL, &pipelineState);

  iio_set_rasterization_state_create_info(
    VK_FALSE, VK_FALSE, 
    VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE, 
    VK_FALSE, 0, 0, 0, 1.0f, 
    &pipelineState
  );
  
  iio_set_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, VK_FALSE, 1.0f, NULL, VK_FALSE, VK_FALSE, &pipelineState);

  iio_set_depth_stencil_state_create_info(0, VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS, VK_FALSE, VK_FALSE, (VkStencilOpState) {0}, (VkStencilOpState) {0}, 0, 0, &pipelineState);

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    .blendEnable = VK_FALSE,
  };
  iio_set_color_blend_state_create_info(0, VK_FALSE, VK_LOGIC_OP_COPY, 1, &colorBlendAttachment, (float [4]) {0, 0, 0, 0}, &pipelineState);

  iio_set_dynamic_state_create_info(2, (VkDynamicState [2]) {VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT}, &pipelineState);

  iio_set_rendering_info(&state.surfaceFormat.format, iio_find_depth_format(), 0, &pipelineState);

  

  uint32_t setLayoutCount = state.descriptorPoolManagerCount;
  VkDescriptorSetLayout setLayouts [setLayoutCount];
  for (uint32_t i = 0; i < setLayoutCount; i++) {
    setLayouts[i] = state.descriptorPoolMangers[i].descriptorSetLayout;
  }
  iio_create_graphics_pipeline_layout(
    state.device, 
    state.descriptorPoolManagerCount, setLayouts,
    0, NULL,
    &state.graphicsPipelineManger
  );

  iio_create_graphics_pipeline(state.device, &state.graphicsPipelineManger, true, VK_NULL_HANDLE, 0, &pipelineState);

  vkDestroyShaderModule(state.device, vertShaderModule, NULL);
  vkDestroyShaderModule(state.device, fragShaderModule, NULL);
}

void iio_create_graphics_pipeline_testtriangle() {
  fprintf(stdout, "Creating shader pipeline for test triangle\n");
  VkShaderModule vertShaderModule, fragShaderModule;
  {
    DataBuffer * vertShaderCode = iio_read_shader_file_to_buffer("src/shaders/testtrianglevert.spv");
    if (!vertShaderCode) {
      fprintf(stderr, "Failed to read vertex shader file\n");
      exit(1);
    }
    DataBuffer * fragShaderCode = iio_read_shader_file_to_buffer("src/shaders/testtrianglefrag.spv");
    if (!fragShaderCode) {
      fprintf(stderr, "Failed to read fragment shader file\n");
      free(vertShaderCode);
      exit(1);
    }
    vertShaderModule = iio_create_shader_module(vertShaderCode);
    if (vertShaderModule == VK_NULL_HANDLE) {
      fprintf(stderr, "Failed to create vertex shader module\n");
      free(vertShaderCode);
      free(fragShaderCode);
      exit(1);
    }
    fprintf(stdout, "Vertex shader module created successfully.\n");
    fragShaderModule = iio_create_shader_module(fragShaderCode);
    if (fragShaderModule == VK_NULL_HANDLE) {
      fprintf(stderr, "Failed to create fragment shader module\n");
      vkDestroyShaderModule(state.device, vertShaderModule, NULL);
      free(vertShaderCode);
      free(fragShaderCode);
      exit(1);
    }
    fprintf(stdout, "Fragment shader module created successfully.\n");
    free(vertShaderCode);
    free(fragShaderCode);
  }

  fprintf(stdout, "creating graphics pipeline state\n");
  IIOGraphicsPipelineStates pipelineState = iio_create_graphics_pipeline_state();
  
  iio_create_shader_stage_create_info(
    2, 
    (VkShaderStageFlagBits [2]) {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT}, 
    (VkShaderModule [2]) {vertShaderModule, fragShaderModule},
    &pipelineState
  );
  
  iio_set_vertex_input_state_create_info(
    0, NULL,
    0, NULL,
    &pipelineState
  );

  iio_set_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false, &pipelineState);

  iio_set_tessellation_state_create_info(&pipelineState);

  iio_set_viewport_state_create_info(1, NULL, 1, NULL, &pipelineState);

  iio_set_rasterization_state_create_info(
    VK_FALSE, VK_FALSE, 
    VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE, 
    VK_FALSE, 0, 0, 0, 1.0f, 
    &pipelineState
  );
  
  iio_set_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, VK_FALSE, 1.0f, NULL, VK_FALSE, VK_FALSE, &pipelineState);

  // iio_set_depth_stencil_state_create_info(0, VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS, VK_FALSE, VK_FALSE, (VkStencilOpState) {0}, (VkStencilOpState) {0}, 0, 0, &pipelineState);

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    .blendEnable = VK_FALSE
  };
  iio_set_color_blend_state_create_info(0, VK_FALSE, VK_LOGIC_OP_COPY, 1, &colorBlendAttachment, (float [4]) {0, 0, 0, 0}, &pipelineState);

  iio_set_dynamic_state_create_info(2, (VkDynamicState [2]) {VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT}, &pipelineState);

  iio_set_rendering_info(&state.surfaceFormat.format, 0, 0, &pipelineState);

  iio_create_graphics_pipeline_layout(
    state.device, 
    0, NULL,
    0, NULL,
    &state.graphicsPipelineManger
  );

  iio_create_graphics_pipeline(state.device, &state.graphicsPipelineManger, true, VK_NULL_HANDLE, 0, &pipelineState);

  vkDestroyShaderModule(state.device, vertShaderModule, NULL);
  vkDestroyShaderModule(state.device, fragShaderModule, NULL);
}

void iio_create_graphics_pipeline_testcube() {
  fprintf(stdout, "Creating shader pipeline for test cube\n");
  DataBuffer * vertShaderCode = iio_read_shader_file_to_buffer("src/shaders/testcubevert.spv");
  if (!vertShaderCode) {
    fprintf(stderr, "Failed to read vertex shader file\n");
    exit(1);
  }
  DataBuffer * fragShaderCode = iio_read_shader_file_to_buffer("src/shaders/testcubefrag.spv");
  if (!fragShaderCode) {
    fprintf(stderr, "Failed to read fragment shader file\n");
    free(vertShaderCode);
    exit(1);
  }
  VkShaderModule vertShaderModule = iio_create_shader_module(vertShaderCode);
  if (vertShaderModule == VK_NULL_HANDLE) {
    fprintf(stderr, "Failed to create vertex shader module\n");
    free(vertShaderCode);
    free(fragShaderCode);
    exit(1);
  }
  fprintf(stdout, "Vertex shader module created successfully.\n");
  VkShaderModule fragShaderModule = iio_create_shader_module(fragShaderCode);
  if (fragShaderModule == VK_NULL_HANDLE) {
    fprintf(stderr, "Failed to create fragment shader module\n");
    vkDestroyShaderModule(state.device, vertShaderModule, NULL);
    free(vertShaderCode);
    free(fragShaderCode);
    exit(1);
  }
  fprintf(stdout, "Fragment shader module created successfully.\n");
  free(vertShaderCode);
  free(fragShaderCode);

  fprintf(stdout, "creating graphics pipeline state\n");
  IIOGraphicsPipelineStates pipelineState = iio_create_graphics_pipeline_state();
  
  iio_create_shader_stage_create_info(
    2, 
    (VkShaderStageFlagBits [2]) {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT}, 
    (VkShaderModule [2]) {vertShaderModule, fragShaderModule},
    &pipelineState
  );
  
  iio_set_vertex_input_state_create_info(
    1, &(VkVertexInputBindingDescription) {.binding = 0, .stride = sizeof(Vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},
    3, (VkVertexInputAttributeDescription [3]) {
      {.binding = 0, .location = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, position)},
      {.binding = 0, .location = 1, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, color)},
      {.binding = 0, .location = 2, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, texCoord)}
    },
    &pipelineState
  );

  iio_set_input_assembly_state_create_info( VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false, &pipelineState);

  iio_set_tessellation_state_create_info(&pipelineState);

  iio_set_viewport_state_create_info(1, NULL, 1, NULL, &pipelineState);

  iio_set_rasterization_state_create_info(
    VK_FALSE, VK_FALSE, 
    VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE, 
    VK_FALSE, 0, 0, 0, 1.0f, 
    &pipelineState
  );
  
  iio_set_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, VK_FALSE, 1.0f, NULL, VK_FALSE, VK_FALSE, &pipelineState);

  iio_set_depth_stencil_state_create_info(0, VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS, VK_FALSE, VK_FALSE, (VkStencilOpState) {0}, (VkStencilOpState) {0}, 0, 0, &pipelineState);

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    .blendEnable = VK_FALSE,
  };
  iio_set_color_blend_state_create_info(0, VK_FALSE, VK_LOGIC_OP_COPY, 1, &colorBlendAttachment, (float [4]) {0, 0, 0, 0}, &pipelineState);

  iio_set_dynamic_state_create_info(2, (VkDynamicState [2]) {VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT}, &pipelineState);

  iio_set_rendering_info(&state.surfaceFormat.format, iio_find_depth_format(), 0, &pipelineState);

  

  uint32_t setLayoutCount = state.descriptorPoolManagerCount;
  VkDescriptorSetLayout setLayouts [setLayoutCount];
  for (uint32_t i = 0; i < setLayoutCount; i++) {
    setLayouts[i] = state.descriptorPoolMangers[i].descriptorSetLayout;
  }
  iio_create_graphics_pipeline_layout(
    state.device, 
    state.descriptorPoolManagerCount, setLayouts,
    0, NULL,
    &state.graphicsPipelineManger
  );

  iio_create_graphics_pipeline(state.device, &state.graphicsPipelineManger, true, VK_NULL_HANDLE, 0, &pipelineState);

  vkDestroyShaderModule(state.device, vertShaderModule, NULL);
  vkDestroyShaderModule(state.device, fragShaderModule, NULL);
}

void iio_initialize_testcube() {
  testCube.isInitialized = true;

  fprintf(stdout, "initiliazing testcube vaules\n");
  iio_create_vertex_buffer_testcube();
  iio_create_index_buffer_testcube();
  size_t bufferSize = sizeof(ModelUniformBufferData);
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    fprintf(stdout, "creating uniform buffer for testcube model uniform buffer\n");
    iio_create_uniform_buffer(state.device, bufferSize, &testCube.modelUniformBuffer[i], &testCube.modelUniformBufferMemory[i], &testCube.modelUniformBufferMapped[i]);
    fprintf(stdout, "writing model uniform buffer to shader buffer\n");
    iio_write_buffer_descriptor(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, testCube.modelUniformBuffer[i], 0, bufferSize, &state.descriptorSetWriter);
    iio_update_set(state.device, testCube.modelUniformBufferDescriptorSets[i], &state.descriptorSetWriter);
  }
  iio_load_image(&state.resourceManager, testTextureFilename, &testCube.textureImage, &defaultSamplerCreateInfo);
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    fprintf(stdout, "writing testcube image sampler to shader sampler\n");
    iio_write_image_descriptor(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, testCube.textureImage.sampler, testCube.textureImage.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, &state.descriptorSetWriter);
    iio_update_set(state.device, testCube.texSamplerDescriptorSets[i], &state.descriptorSetWriter);
  }
  fprintf(stdout, "testcube initialized\n\n");
  fprintf(stdout, "size of image hashmap: %llu\n", state.resourceManager.imageMap.size);
}

void iio_initialize_camera() {
  fprintf(stdout, "initializing camera values\n");
  size_t bufferSize = sizeof(CameraUniformBufferData);
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    iio_create_uniform_buffer(state.device, bufferSize, &state.globalUniformBuffers[i], &state.globalUniformBuffersMemory[i], &state.globalUniformBuffersMapped[i]);
    iio_write_buffer_descriptor(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, state.globalUniformBuffers[i], 0, bufferSize, &state.descriptorSetWriter);
    iio_update_set(state.device, state.cameraDescriptorSets[i], &state.descriptorSetWriter);
  }
  fprintf(stdout, "camera initialized\n\n");
}

void iio_create_command_pool() {
  fprintf(stdout, "Creating command pool.\n");
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

void iio_create_depth_resources() {
  VkFormat depthFormat = iio_find_depth_format();
  if (depthFormat == VK_FORMAT_UNDEFINED) {
    fprintf(stderr, "Failed to find suitable depth format\n");
    exit(1);
  }

  iio_create_image(
    state.swapChainImageExtent.width,
    state.swapChainImageExtent.height,
    &state.depthImage,
    &state.depthImageMemory,
    depthFormat,
    VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
  );

  iio_create_image_view(state.depthImage, &state.depthImageView, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

VkFormat iio_find_depth_format() {
  VkFormat format;
  const VkFormat candidates [3] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
  VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
  VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
  format = iio_find_supported_format(candidates, 3, tiling, features);
  return format;
}

void iio_create_vertex_buffer_testcube() {
  VkDeviceSize bufferSize = sizeof(testCube.vertices);
  
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  fprintf(stdout, "Creating staging buffer.\n");
  iio_create_buffer(
    bufferSize,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    &stagingBuffer,
    &stagingBufferMemory
  );
  fprintf(stdout, "Staging buffer created successfully.\n");

  fprintf(stdout, "Mapping vertex buffer memory and copying data.\n");
  void * data = NULL;
  vkMapMemory(state.device, stagingBufferMemory, 0, bufferSize, 0, &data);
  if (!data) {
    fprintf(stderr, "Failed to map vertex buffer memory\n");
    exit(1);
  }
  memcpy(data, testCube.vertices, sizeof(testCube.vertices));
  vkUnmapMemory(state.device, stagingBufferMemory);
  fprintf(stdout, "Data copied to staging buffer successfully.\n");
  fprintf(stdout, "Creating vertex buffer.\n");
  iio_create_buffer(
    bufferSize, 
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
    &testCube.vertexBuffer, 
    &testCube.vertexBufferMemory
  );
  fprintf(stdout, "Vertex buffer created successfully.\n");
  fprintf(stdout, "Copying data from staging buffer to vertex buffer.\n");
  iio_copy_buffer(stagingBuffer, testCube.vertexBuffer, bufferSize);
  fprintf(stdout, "Data copied successfully.\n");
  vkDestroyBuffer(state.device, stagingBuffer, NULL);
  vkFreeMemory(state.device, stagingBufferMemory, NULL);
}

void iio_create_index_buffer_testcube() {
  VkDeviceSize bufferSize = sizeof(testCube.indices);
  
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  fprintf(stdout, "Creating staging buffer for index buffer.\n");
  iio_create_buffer(
    bufferSize,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    &stagingBuffer,
    &stagingBufferMemory
  );
  fprintf(stdout, "Staging buffer for index buffer created successfully.\n");

  fprintf(stdout, "Mapping index buffer memory and copying data.\n");
  void * data = NULL;
  vkMapMemory(state.device, stagingBufferMemory, 0, bufferSize, 0, &data);
  if (!data) {
    fprintf(stderr, "Failed to map index buffer memory\n");
    exit(1);
  }
  memcpy(data, testCube.indices, sizeof(testCube.indices));
  vkUnmapMemory(state.device, stagingBufferMemory);
  fprintf(stdout, "Data copied to staging buffer for index buffer successfully.\n");

  fprintf(stdout, "Creating index buffer.\n");
  iio_create_buffer(
    bufferSize, 
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
    &testCube.indexBuffer, 
    &testCube.indexBufferMemory
  );
  fprintf(stdout, "Index buffer created successfully.\n");

  fprintf(stdout, "Copying data from staging buffer to index buffer.\n");
  iio_copy_buffer(stagingBuffer, testCube.indexBuffer, bufferSize);
  fprintf(stdout, "Data copied to index buffer successfully.\n");

  vkDestroyBuffer(state.device, stagingBuffer, NULL);
  vkFreeMemory(state.device, stagingBufferMemory, NULL);
}

void iio_create_uniform_buffer(VkDevice device, VkDeviceSize bufferSize, VkBuffer * buffer, VkDeviceMemory * bufferMemory, void ** bufferMapped) {
  iio_create_buffer(
    bufferSize,
    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    buffer, bufferMemory
  );
  vkMapMemory(device, *bufferMemory, 0, bufferSize, 0, bufferMapped);
}

void iio_create_command_buffers() {
  fprintf(stdout, "Creating command buffer.\n");
  state.commandBuffers = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkCommandBuffer));
  if (!state.commandBuffers) {
    iio_oom_error(NULL, __LINE__, __FILE__);
    exit(1);
  }
  VkCommandBufferAllocateInfo allocateInfo = {0};
  allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocateInfo.pNext = NULL;
  allocateInfo.commandPool = state.commandPool;
  allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocateInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

  VkResult result = vkAllocateCommandBuffers(state.device, &allocateInfo, state.commandBuffers);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }
}

void iio_create_synchronization_objects() {
  fprintf(stdout, "Creating synchronization objects.\n");
  state.imageAvailableSemaphores = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkSemaphore));
  state.renderFinishedSemaphores = malloc(state.swapChainImageCount * sizeof(VkSemaphore));
  state.inFlightFences = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkFence));
  if (!state.imageAvailableSemaphores || !state.renderFinishedSemaphores || !state.inFlightFences) {
    iio_oom_error(NULL, __LINE__, __FILE__);
    exit(1);
  }

  VkSemaphoreCreateInfo semaphoreCreateInfo = {0};
  semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  VkFenceCreateInfo fenceCreateInfo = {0};
  fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkResult result = vkCreateSemaphore(state.device, &semaphoreCreateInfo, NULL, &state.imageAvailableSemaphores[i]);
    if (result == VK_SUCCESS) {
      result = vkCreateFence(state.device, &fenceCreateInfo, NULL, &state.inFlightFences[i]);
    } else {
      iio_vk_error(result, __LINE__, __FILE__);
      exit(1);
    }
  }
  for (int i = 0; i < state.swapChainImageCount; i++) {
    VkResult result = vkCreateSemaphore(state.device, &semaphoreCreateInfo, NULL, &state.renderFinishedSemaphores[i]);
    if (result != VK_SUCCESS) {
      iio_vk_error(result, __LINE__, __FILE__);
      exit(1);
    }
  }
}

/****************************************************************************************************
 *                                Resource Loader Specific Functions                                *
 ****************************************************************************************************/

void iio_initialize_resource_loader() {
  iio_set_create_texture_image_func(iio_create_texture_image_func);
  iio_set_create_texture_image_from_memory_func(iio_create_texture_image_from_memory_func);
  iio_set_create_texture_image_from_pixels_func(iio_create_texture_image_from_pixels_func);
  iio_set_create_image_sampler_func(iio_create_image_sampler_func);

  iio_initialize_resource_manager(&state.resourceManager);

  iio_initialize_default_texture_resources(&state.resourceManager);
}

void iio_create_texture_image_func(const char * path, VkImage * image, VkDeviceMemory * imageMemory, VkImageView * imageView) {
  iio_create_texture_image(path, image, imageMemory);
  iio_create_texture_image_view(*image, imageView);
}

void iio_create_texture_image_from_memory_func(const uint8_t * data, size_t dataSize, VkImage * image, VkDeviceMemory * imageMemory, VkImageView * imageView) {
  iio_create_texture_image_from_memory(data, dataSize, image, imageMemory);
  iio_create_texture_image_view(*image, imageView);
}

void iio_create_texture_image_from_pixels_func(const uint8_t * pixels, size_t width, size_t height, VkImage * image, VkDeviceMemory * imageMemory, VkImageView * imageView) {
  iio_create_texture_image_from_pixels(pixels, width, height, image, imageMemory);
  iio_create_texture_image_view(*image, imageView);
}

void iio_create_image_sampler_func(const VkSamplerCreateInfo * createInfo, VkSampler * sampler) {
  vkCreateSampler(state.device, createInfo, NULL, sampler);
  if (*sampler == VK_NULL_HANDLE) {
    fprintf(stderr, "Failed to create texture sampler\n");
    return;
  }
}

/****************************************************************************************************
 *                                    Vulkan API Helper Functions                                   *
 ****************************************************************************************************/

DataBuffer * iio_read_shader_file_to_buffer(const char * path) {
  fprintf(stdout, "Reading shader file: %s\n", path);
  FILE * file = fopen(path, "rb");
  if (!file) {
    fprintf(stderr, "Failed to open shader file: %s\n", path);
    return NULL;
  }
  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fseek(file, 0, SEEK_SET);
  uint32_t bufferSize = (size + 3) / 4;
  DataBuffer * buffer = malloc(sizeof(DataBuffer) + (bufferSize) * sizeof(uint32_t));
  if (!buffer) {
    fprintf(stderr, "Failed to allocate memory for shader file: %s\n", path);
    fclose(file);
    return NULL;
  }
  buffer->size = bufferSize;
  size_t readSize = fread((char *) buffer->data, 1, size, file);
  if (readSize != size) {
    fprintf(stderr, "Failed to read shader file: %s\n", path);
    free(buffer);
    fclose(file);
    return NULL;
  }
  if (size %4 != 0) {
    memset((uint8_t *) buffer->data + size, 0, 4 - (size % 4));
  }
  fclose(file);
  return buffer;
}

VkShaderModule iio_create_shader_module(const DataBuffer * shaderCode) {
  if (!shaderCode || shaderCode->size == 0) {
    fprintf(stderr, "Invalid shader code\n");
    return VK_NULL_HANDLE;
  }
  
  VkShaderModuleCreateInfo createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.pNext = NULL;
  createInfo.flags = 0;
  createInfo.codeSize = shaderCode->size * sizeof(uint32_t);
  createInfo.pCode = (const uint32_t *) shaderCode->data;

  VkShaderModule shaderModule;
  VkResult result = vkCreateShaderModule(state.device, &createInfo, NULL, &shaderModule);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }
  
  return shaderModule;
}

void iio_select_physical_device_properties (
  VkPhysicalDevice physicalDevice,
  VkSurfaceFormatKHR * preferredSurfaceFormat,
  VkPresentModeKHR * preferredPresentMode,
  VkSurfaceCapabilitiesKHR * surfaceCapabilities,
  uint32_t * graphicsQueueIndex,
  uint32_t * presentQueueIndex,
  VkPhysicalDeviceType * deviceType) 

{ //  iio_select_physical_device_properties
  //  initialize the output parameters to default values
  *preferredSurfaceFormat = (VkSurfaceFormatKHR) {0};
  *preferredPresentMode = 0;
  *surfaceCapabilities = (VkSurfaceCapabilitiesKHR) {0};
  *graphicsQueueIndex = 0;
  *presentQueueIndex = 0;
  *deviceType = VK_PHYSICAL_DEVICE_TYPE_OTHER; // Default to other. The system should test for this value and handle it accordingly
  uint32_t queueFamilyCount;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);
  if (queueFamilyCount == 0) {
    fprintf(stderr, "No queue families found for physical device\n");
    return;
  }
  VkQueueFamilyProperties queueFamilies [queueFamilyCount];
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);
  char hasGraphicsFamily = 0;
  char hasPresentFamily = 0;

  for (int i = 0; i < queueFamilyCount; i++) {
  //  iterate through the queue families and check if the device has a graphics and present family
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      *graphicsQueueIndex = i;
      hasGraphicsFamily = 1;
    }
    VkBool32 presentSupport = 0;
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, state.surface, &presentSupport);
    if (presentSupport) {
      *presentQueueIndex = i;
      hasPresentFamily = 1;
    }
    if (*graphicsQueueIndex == *presentQueueIndex) {
      break;
    }
  }

  if (!hasGraphicsFamily || !hasPresentFamily) {
    return;
  }

  //  check if the device supports the required device features
  VkPhysicalDeviceFeatures features;
  vkGetPhysicalDeviceFeatures(physicalDevice, &features);
  if (!features.samplerAnisotropy) {
    return;
  }

  //  check if the device has the required extensions. If not, skip it
  uint32_t extensionCount = 0;
  vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &extensionCount, NULL);
  if (extensionCount == 0) {
    return;
  }
  VkExtensionProperties extensions [extensionCount];
  vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &extensionCount, extensions);
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
  if (requiredExtensionsPresent != sizeof(deviceExtensions) / sizeof(char *)) {
    return; //  device does not have the required extensions
  }

  //  get the surface capabilities, formats and present modes

  //  get the surface capabilities for the device
  VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, state.surface, surfaceCapabilities);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    return;
  }

  //  get the surface formats for the device
  uint32_t surfaceFormatCount = 0;
  result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, state.surface, &surfaceFormatCount, NULL);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    return;
  }
  if (surfaceFormatCount == 0) {
    return;
  }
  VkSurfaceFormatKHR surfaceFormats [surfaceFormatCount];
  result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, state.surface, &surfaceFormatCount, surfaceFormats);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    return;
  }

  for (int i = 0; i < surfaceFormatCount; i++) {
    if (surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
        surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      *preferredSurfaceFormat = surfaceFormats[i];
      break;
    } else if (i == surfaceFormatCount - 1) {
      *preferredSurfaceFormat = surfaceFormats[0];
    }
  }

  //  get the surface present modes for the device
  uint32_t presentModeCount = 0;
  result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, state.surface, &presentModeCount, NULL);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    return;
  }
  if (presentModeCount == 0) {
    return;
  }
  VkPresentModeKHR presentModes [presentModeCount];
  result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, state.surface, &presentModeCount, presentModes);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    return;
  }
  VkPresentModeKHR presentModeHeirarchy [] = {
    VK_PRESENT_MODE_MAILBOX_KHR,
    VK_PRESENT_MODE_FIFO_KHR,
    VK_PRESENT_MODE_FIFO_RELAXED_KHR,
    VK_PRESENT_MODE_IMMEDIATE_KHR,
  };
  for (int i = 0; i < presentModeCount; i++) {
    if (presentModes[i] == presentModeHeirarchy[0]) {
      *preferredPresentMode = presentModes[i];
      break;
    } else if (presentModes[i] == presentModeHeirarchy[1]) {
      *preferredPresentMode = presentModes[i];
    } else if (presentModes[i] == presentModeHeirarchy[2] &&
                *preferredPresentMode != presentModeHeirarchy[1]) {
      *preferredPresentMode = presentModes[i];
    } else if (presentModes[i] == presentModeHeirarchy[3] &&
                *preferredPresentMode != presentModeHeirarchy[1] &&
                *preferredPresentMode != presentModeHeirarchy[2]) {
      *preferredPresentMode = presentModes[i];
    } else if (i == presentModeCount - 1) {
      *preferredPresentMode = presentModes[0];
    }
  }
  
  //  get the device type
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(physicalDevice, &properties);
  *deviceType = properties.deviceType;
}

static VkVertexInputBindingDescription iio_get_binding_description() {
  VkVertexInputBindingDescription bindingDescription = {0};

  bindingDescription.binding = 0; // Binding index
  bindingDescription.stride = sizeof(Vertex); // Size of each vertex
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Each vertex is a separate instance

  return bindingDescription;
}

static VkVertexInputAttributeDescription * iio_get_attribute_descriptions(int * count) {
  *count = 3;
  static VkVertexInputAttributeDescription attributeDescriptions[3] = {0};

  // Position attribute
  attributeDescriptions[0].binding = 0; // Binding index
  attributeDescriptions[0].location = 0; // Location index
  attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // Format of the attribute
  attributeDescriptions[0].offset = offsetof(Vertex, position); // Offset in the vertex structure

  // Color attribute
  attributeDescriptions[1].binding = 0; // Binding index
  attributeDescriptions[1].location = 1; // Location index
  attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // Format of the attribute
  attributeDescriptions[1].offset = offsetof(Vertex, color); // Offset in the vertex structure

  // Texture coordinate attribute
  attributeDescriptions[2].binding = 0; // Binding index
  attributeDescriptions[2].location = 2; // Location index
  attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT; // Format of the attribute
  attributeDescriptions[2].offset = offsetof(Vertex, texCoord); // Offset in the vertex structure

  return attributeDescriptions;
}

uint32_t iio_find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(state.selectedDevice, &memoryProperties);

  for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && 
        (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }
  
  fprintf(stderr, "Failed to find suitable memory type\n");
  exit(1);
}

void iio_create_buffer(
  VkDeviceSize size,
  VkBufferUsageFlags usage,
  VkMemoryPropertyFlags properties,
  VkBuffer * buffer,
  VkDeviceMemory * bufferMemory)

{
  VkBufferCreateInfo bufferCreateInfo = {0};
  bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCreateInfo.size = size;
  bufferCreateInfo.usage = usage;
  bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkResult result = vkCreateBuffer(state.device, &bufferCreateInfo, NULL, buffer);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }

  VkMemoryRequirements memoryRequirements;
  vkGetBufferMemoryRequirements(state.device, *buffer, &memoryRequirements);

  VkMemoryAllocateInfo allocInfo = {0};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memoryRequirements.size;
  allocInfo.memoryTypeIndex = iio_find_memory_type(memoryRequirements.memoryTypeBits, properties);

  result = vkAllocateMemory(state.device, &allocInfo, NULL, bufferMemory);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }

  vkBindBufferMemory(state.device, *buffer, *bufferMemory, 0);
}

void iio_copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
  VkCommandBuffer commandBuffer = iio_begin_single_time_commands();
  VkBufferCopy copyRegion = {0};
  copyRegion.srcOffset = 0; // Optional
  copyRegion.dstOffset = 0; // Optional
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
  iio_end_single_time_commands(commandBuffer);
}

VkCommandBuffer iio_begin_single_time_commands() {
  VkCommandBufferAllocateInfo allocInfo = {0};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = state.commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;
  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(state.device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo = {0};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);
  
  return commandBuffer;
}

void iio_end_single_time_commands(VkCommandBuffer commandBuffer) {
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo = {0};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(state.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(state.graphicsQueue);

  vkFreeCommandBuffers(state.device, state.commandPool, 1, &commandBuffer);
}

void iio_update_camera_uniform_buffer(uint32_t currentFrame) {
  float fov = 45.0f;
  CameraUniformBufferData ubo = {0};
  glm_mat4_identity(ubo.view);
  vec3 target = {0};
  glm_vec3_add(camera.position, camera.front, target);
  glm_lookat(camera.position, target, camera.up, ubo.view);
  glm_mat4_identity(ubo.projection);
  float fovy = state.swapChainImageExtent.width > state.swapChainImageExtent.height ? 
               glm_rad(fov) :
               2.0f * atanf((float) state.swapChainImageExtent.height / (float) state.swapChainImageExtent.width * tanf(glm_rad(fov / 2.0f)));
  glm_perspective(fovy, (float) state.swapChainImageExtent.width / (float) state.swapChainImageExtent.height, 0.1f, 10.0f, ubo.projection);
  ubo.projection[1][1] *= -1.0f; // Invert Y axis for Vulkan
  memcpy(state.globalUniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}

void iio_create_texture_image(const char * path, VkImage * textureImage, VkDeviceMemory * textureImageMemory) {
  int width, height, channels;
  stbi_uc * pixels = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);
  VkDeviceSize imageSize = width * height * 4; // Assuming 4 bytes per pixel (RGBA)

  if (!pixels) {
    fprintf(stderr, "Failed to load texture image: %s\n", path);
    *textureImage = VK_NULL_HANDLE;
    *textureImageMemory = VK_NULL_HANDLE;
    return;
  }

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  fprintf(stdout, "Creating staging buffer for texture image.\n");
  iio_create_buffer(
    imageSize,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    &stagingBuffer,
    &stagingBufferMemory
  );
  fprintf(stdout, "Staging buffer for texture image created successfully.\n");
  fprintf(stdout, "Mapping texture image memory and copying data.\n");
  void * data = NULL;
  vkMapMemory(state.device, stagingBufferMemory, 0, imageSize, 0, &data);
  memcpy(data, pixels, imageSize);
  vkUnmapMemory(state.device, stagingBufferMemory);
  fprintf(stdout, "Data copied to staging buffer for texture image successfully.\n");
  stbi_image_free(pixels);
  fprintf(stdout, "Creating texture image.\n");
  iio_create_image(
    (uint32_t) width,
    (uint32_t) height,
    textureImage,
    textureImageMemory,
    VK_FORMAT_R8G8B8A8_SRGB, // Assuming RGBA format
    VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
  );
  iio_transition_image_layout(*textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  iio_copy_buffer_to_image(stagingBuffer, *textureImage, (uint32_t) width, (uint32_t) height);
  iio_transition_image_layout(*textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  vkDestroyBuffer(state.device, stagingBuffer, NULL);
  vkFreeMemory(state.device, stagingBufferMemory, NULL);
}

void iio_create_texture_image_from_memory(const uint8_t * pData, int size, VkImage * textureImage, VkDeviceMemory * textureImageMemory) {
  int width, height, channels;
  stbi_uc * pixels = stbi_load_from_memory(pData, size, &width, &height, &channels, STBI_rgb_alpha);
  VkDeviceSize imageSize = width * height * 4;

  if (!pixels) {
    fprintf(stderr, "Failed to load texture image from memory: ptr 0x%08x %08x\n", (uint64_t) pData >> 32, (uint64_t) pData | 0x00000000ffffffff);
    *textureImage = VK_NULL_HANDLE;
    *textureImageMemory = VK_NULL_HANDLE;
    return;
  }

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  fprintf(stdout, "Creating staging buffer for texture image.\n");
  iio_create_buffer(
    imageSize,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    &stagingBuffer,
    &stagingBufferMemory
  );
  fprintf(stdout, "Staging buffer for texture image created successfully.\n");
  fprintf(stdout, "Mapping texture image memory and copying data.\n");
  void * data = NULL;
  vkMapMemory(state.device, stagingBufferMemory, 0, imageSize, 0, &data);
  memcpy(data, pixels, imageSize);
  vkUnmapMemory(state.device, stagingBufferMemory);
  fprintf(stdout, "Data copied to staging buffer for texture image successfully.\n");
  stbi_image_free(pixels);
  fprintf(stdout, "Creating texture image.\n");
  iio_create_image(
    (uint32_t) width,
    (uint32_t) height,
    textureImage,
    textureImageMemory,
    VK_FORMAT_R8G8B8A8_SRGB, // Assuming RGBA format
    VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
  );
  iio_transition_image_layout(*textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  iio_copy_buffer_to_image(stagingBuffer, *textureImage, (uint32_t) width, (uint32_t) height);
  iio_transition_image_layout(*textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  vkDestroyBuffer(state.device, stagingBuffer, NULL);
  vkFreeMemory(state.device, stagingBufferMemory, NULL);
}

void iio_create_texture_image_from_pixels(const uint8_t * pixels, int width, int height, VkImage * textureImage, VkDeviceMemory * textureImageMemory) {
  // TODO : adjust this to take a modular amount of channels
  VkDeviceSize imageSize = width * height * 4;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  fprintf(stdout, "Creating staging buffer for texture image.\n");
  iio_create_buffer(
    imageSize,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    &stagingBuffer,
    &stagingBufferMemory
  );
  fprintf(stdout, "Staging buffer for texture image created successfully.\n");
  fprintf(stdout, "Mapping texture image memory and copying data.\n");
  void * data = NULL;
  vkMapMemory(state.device, stagingBufferMemory, 0, imageSize, 0, &data);
  memcpy(data, pixels, imageSize);
  vkUnmapMemory(state.device, stagingBufferMemory);
  fprintf(stdout, "Data copied to staging buffer for texture image successfully.\n");
  fprintf(stdout, "Creating texture image.\n");
  iio_create_image(
    (uint32_t) width,
    (uint32_t) height,
    textureImage,
    textureImageMemory,
    VK_FORMAT_R8G8B8A8_SRGB, // Assuming RGBA format
    VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
  );
  iio_transition_image_layout(*textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  iio_copy_buffer_to_image(stagingBuffer, *textureImage, (uint32_t) width, (uint32_t) height);
  iio_transition_image_layout(*textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  vkDestroyBuffer(state.device, stagingBuffer, NULL);
  vkFreeMemory(state.device, stagingBufferMemory, NULL);
}

void iio_create_texture_image_view(VkImage textureImage, VkImageView * textureImageView) {
  iio_create_image_view(textureImage, textureImageView, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void iio_create_texture_sampler(VkSampler * textureSampler) {
  iio_create_image_sampler(textureSampler);
}

void iio_create_image(
  uint32_t                                  width,
  uint32_t                                  height,
  VkImage *                                 textureImage,
  VkDeviceMemory *                          textureImageMemory,
  VkFormat                                  format,
  VkImageTiling                             tiling,
  VkImageUsageFlags                         usage,
  VkMemoryPropertyFlags                     properties) 

{ //  iio_create_image
  VkImageCreateInfo imageCreateInfo = {0};
  imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  imageCreateInfo.format = format; 
  imageCreateInfo.extent.width = width;
  imageCreateInfo.extent.height = height;
  imageCreateInfo.extent.depth = 1;
  imageCreateInfo.mipLevels = 1;
  imageCreateInfo.arrayLayers = 1;
  imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageCreateInfo.tiling = tiling;
  imageCreateInfo.usage = usage;
  imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  VkResult result = vkCreateImage(state.device, &imageCreateInfo, NULL, textureImage);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }

  VkMemoryRequirements memoryRequirements;
  vkGetImageMemoryRequirements(state.device, *textureImage, &memoryRequirements);

  VkMemoryAllocateInfo allocInfo = {0};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memoryRequirements.size;
  allocInfo.memoryTypeIndex = iio_find_memory_type(memoryRequirements.memoryTypeBits, properties);

  result = vkAllocateMemory(state.device, &allocInfo, NULL, textureImageMemory);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }

  vkBindImageMemory(state.device, *textureImage, *textureImageMemory, 0);
}

void iio_create_image_view(VkImage image, VkImageView * imageView, VkFormat format, VkImageAspectFlags aspectMask) {
  VkImageViewCreateInfo viewCreateInfo = {0};
  viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewCreateInfo.image = image;
  viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewCreateInfo.format = format;
  viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewCreateInfo.subresourceRange.aspectMask = aspectMask;
  viewCreateInfo.subresourceRange.baseMipLevel = 0;
  viewCreateInfo.subresourceRange.levelCount = 1;
  viewCreateInfo.subresourceRange.baseArrayLayer = 0;
  viewCreateInfo.subresourceRange.layerCount = 1;

  VkResult result = vkCreateImageView(state.device, &viewCreateInfo, NULL, imageView);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }
}

void iio_create_image_sampler(VkSampler * sampler) {
  VkSamplerCreateInfo samplerCreateInfo = {0};
  samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
  samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
  samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerCreateInfo.anisotropyEnable = VK_TRUE;
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(state.selectedDevice, &properties);
  samplerCreateInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
  samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
  samplerCreateInfo.compareEnable = VK_FALSE;
  samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerCreateInfo.minLod = 0.0f;
  samplerCreateInfo.maxLod = 0.0f; // No mipmaps
  samplerCreateInfo.mipLodBias = 0.0f;

  VkResult result = vkCreateSampler(state.device, &samplerCreateInfo, NULL, sampler);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }
}

void iio_transition_image_layout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
  VkCommandBuffer commandBuffer = iio_begin_single_time_commands();

  VkImageMemoryBarrier barrier = {0};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
  } else {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  }

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    fprintf(stderr, "Unsupported layout transition!\n");
    exit(1);
  }

  vkCmdPipelineBarrier(
    commandBuffer,
    sourceStage,
    destinationStage,
    0,
    0, NULL,
    0, NULL,
    1, &barrier
  );

  iio_end_single_time_commands(commandBuffer);
}

void iio_transition_swapchain_image_layout(
  VkCommandBuffer                           commandBuffer,
  uint32_t                                  index, 
  VkImageLayout                             oldLayout, 
  VkImageLayout                             newLayout, 
  VkAccessFlags                             srcAccessMask, 
  VkAccessFlags                             dstAccessMask, 
  VkPipelineStageFlags                      srcStageMask, 
  VkPipelineStageFlags                      dstStageMask, 
  IIOVulkanState *                          state) 

{
  const VkImageMemoryBarrier barrier = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .srcAccessMask = srcAccessMask,
    .dstAccessMask = dstAccessMask,
    .oldLayout = oldLayout,
    .newLayout = newLayout,
    .image = state->swapChainImages[index],
    .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1
    }
  };

  vkCmdPipelineBarrier(
    commandBuffer, 
    srcStageMask, 
    dstStageMask, 
    0, 
    0, NULL, 
    0, NULL, 
    1, &barrier 
  );
}

VkFormat iio_find_supported_format(const VkFormat * candidates, uint32_t count, VkImageTiling tiling, VkFormatFeatureFlags features) {
  for (uint32_t i = 0; i < count; i++) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(state.selectedDevice, candidates[i], &props);
    if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
      return candidates[i];
    } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
      return candidates[i];
    }
  }

  return VK_FORMAT_UNDEFINED; // No suitable format found
}

void iio_copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
  VkCommandBuffer commandBuffer = iio_begin_single_time_commands();

  VkBufferImageCopy region = {0};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset = (VkOffset3D) {0, 0, 0};
  region.imageExtent = (VkExtent3D) {width, height, 1};

  vkCmdCopyBufferToImage(
    commandBuffer,
    buffer,
    image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &region
  );

  iio_end_single_time_commands(commandBuffer);
}

void iio_sleep(double ms) {
  //  Sleep for the specified number of milliseconds
  if (ms > 0) {
    struct timespec ts = {0};
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms - ts.tv_sec) * 1000000;
    nanosleep(&ts, NULL);
  }
}

/****************************************************************************************************
 *                               GLFW specific callbacks and Handlers                               *
 ****************************************************************************************************/

void iio_framebuffer_size_callback(GLFWwindow * window, int width, int height) {
  // IIOVulkanState * stateObj = (IIOVulkanState *) glfwGetWindowUserPointer(window);
  state.framebufferResized = 1;
}

void iio_mouse_button_callback(GLFWwindow * window, int button, int action, int mods) {
  // Handle mouse button events here if needed
}

void iio_scroll_callback(GLFWwindow * window, double xoffset, double yoffset) {
  // Handle scroll events here if needed
}

void iio_key_callback(GLFWwindow * window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  }
  // Handle other key events here if needed
}

void iio_cursor_position_callback(GLFWwindow * window, double xpos, double ypos) {
  // Handle mouse position events here if needed
  static double lastX = 0.0, lastY = 0.0;
  if (lastX == 0.0 && lastY == 0.0) {
    lastX = xpos;
    lastY = ypos;
  }
  float xoffset = xpos - lastX;
  float yoffset = lastY - ypos; // Invert Y axis for Vulkan
  lastX = xpos;
  lastY = ypos;
  float sensitivity = 0.1f; // Adjust sensitivity as needed
  xoffset *= sensitivity;
  yoffset *= sensitivity;
  camera.yaw += xoffset;
  camera.pitch += yoffset;
  if (camera.pitch > 89.0f) {
    camera.pitch = 89.0f;
  }
  if (camera.pitch < -89.0f) {
    camera.pitch = -89.0f;
  }
  camera.front[0] = cos(glm_rad(camera.yaw)) * cos(glm_rad(camera.pitch));
  camera.front[1] = sin(glm_rad(camera.pitch));
  camera.front[2] = sin(glm_rad(camera.yaw)) * cos(glm_rad(camera.pitch));
  glm_vec3_normalize(camera.front);
  glm_vec3_crossn(camera.front, camera.up, camera.right);
  glm_vec3_normalize(camera.right);
  glm_vec3_crossn(camera.up, camera.right, camera.forward);
  glm_vec3_normalize(camera.forward);
}

void iio_process_input(GLFWwindow * window, int * outputCode) {
  float cameraChange = cameraSpeed * deltaTime;
  // Process input events here if needed
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    glm_vec3_muladds(camera.forward, cameraChange, camera.position);
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    glm_vec3_muladds(camera.forward, -cameraChange, camera.position);
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    glm_vec3_muladds(camera.right, -cameraChange, camera.position);
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    glm_vec3_muladds(camera.right, cameraChange, camera.position);
  }
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  }
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
    camera.position[1] += cameraChange;
  }
  if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
    camera.position[1] -= cameraChange;
  }
  if (glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS) {
    *outputCode = 1;
  }
}

/****************************************************************************************************
 *                                              Runtime                                             *
 ****************************************************************************************************/

void iio_run() {
  int code;
  double nextCheck = 0.0f;
  double checkInterval = 10.0f;
  double currentTime;
  double lastTime = 0.0f;
  fprintf(stdout, "camera details:\n\n\n\n\n\n");
  while (!glfwWindowShouldClose(state.window)) {
    currentTime = glfwGetTime();
    deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    // prints out the camera information
    fprintf(stdout, "\r\033[5A");
    fprintf(stdout, "\033[2K\tposition:\t%.2f, %.2f, %.2f\n", camera.position[0], camera.position[1], camera.position[2]);
    fprintf(stdout, "\033[2K\tfront:   \t%.2f, %.2f, %.2f\n", camera.front[0], camera.front[1], camera.front[2]);
    fprintf(stdout, "\033[2K\tyaw:     \t%.2f\n", camera.yaw);
    fprintf(stdout, "\033[2K\tpitch:   \t%.2f\n", camera.pitch);
    fprintf(stdout, "\033[2KFPS: %10.0f\n", 1.0f/deltaTime);

    code = 0;
    iio_process_input(state.window, &code);
    glfwPollEvents();
    draw_frame();
    // double sleepTime = 1000.0f * ((1.0f / 400) - (glfwGetTime() - currentTime));
    // iio_sleep(sleepTime > 0 ? sleepTime : 0);
  }
  vkDeviceWaitIdle(state.device);
  iio_cleanup();
}

void draw_frame() {
  vkWaitForFences(state.device, 1, &state.inFlightFences[state.currentFrame], VK_TRUE, UINT64_MAX);
  
  uint32_t imageIndex;
  VkResult result = vkAcquireNextImageKHR(state.device, state.swapChain, UINT64_MAX, state.imageAvailableSemaphores[state.currentFrame], VK_NULL_HANDLE, &imageIndex);
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    iio_recreate_swapchain();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }

  if (!doTestTriangle) {
    iio_update_camera_uniform_buffer(state.currentFrame);
  }
  
  vkResetFences(state.device, 1, &state.inFlightFences[state.currentFrame]);
  vkResetCommandBuffer(state.commandBuffers[state.currentFrame], 0);


  if (doTestTriangle) {
    iio_record_testtriangle_command_buffer(state.commandBuffers[state.currentFrame], imageIndex, state.currentFrame);
  } else if (doTestCube) {
    iio_record_testcube_command_buffer(state.commandBuffers[state.currentFrame], imageIndex, state.currentFrame);
  } else {
    iio_record_render_to_command_buffer(state.commandBuffers[state.currentFrame], imageIndex, state.currentFrame);
  }

  VkPipelineStageFlags flags [] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSubmitInfo submitInfo = {0};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &state.imageAvailableSemaphores[state.currentFrame];
  submitInfo.pWaitDstStageMask = flags;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &state.commandBuffers[state.currentFrame];
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &state.renderFinishedSemaphores[imageIndex];
  result = vkQueueSubmit(state.graphicsQueue, 1, &submitInfo, state.inFlightFences[state.currentFrame]);

  VkPresentInfoKHR presentInfo = {0};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &state.renderFinishedSemaphores[imageIndex];
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &state.swapChain;
  presentInfo.pImageIndices = &imageIndex;
  presentInfo.pResults = NULL;

  result = vkQueuePresentKHR(state.presentQueue, &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || state.framebufferResized) {
    state.framebufferResized = 0;
    iio_recreate_swapchain();
  } else if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }

  state.currentFrame = (state.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void iio_record_render_to_command_buffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame) {
  VkResult result;
  VkCommandBufferBeginInfo beginInfo = {0};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.pNext = NULL;
  beginInfo.flags = 0;
  beginInfo.pInheritanceInfo = NULL;
  
  result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }

  iio_transition_swapchain_image_layout(
    commandBuffer,
    imageIndex,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    0,
    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    &state
  );

  VkRenderingAttachmentInfo colorAttachment = {
    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
    .clearValue = {
      .color.float32 = {0.5, 0.0, 0.5, 1.0},
    },
    .imageView = state.swapChainImageViews[imageIndex],
    .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
  };

  VkRenderingInfo renderingInfo = {
    .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
    .pNext = NULL,
    .flags = 0,
    .renderArea = {
      .offset = {0, 0},
      .extent = state.swapChainImageExtent
    },
    .layerCount = 1,
    .colorAttachmentCount = 1,
    .pColorAttachments = &colorAttachment,
    .viewMask = 0
  };
  
  vkCmdBeginRendering(commandBuffer, &renderingInfo);

  //record the draw commands here
  
  // for each pipeline, bind the pipelines, set the dynamic state, and draw the associated objects
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state.graphicsPipelineManger.pipeline);

  VkViewport viewport = {0};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.height = (float) state.swapChainImageExtent.height;
  viewport.width = (float) state.swapChainImageExtent.width;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor = {0};
  scissor.offset = (VkOffset2D) {0, 0};
  scissor.extent = state.swapChainImageExtent;
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  // for each render target group by texture, bind the texture descriptor sets

  

  //end of recording draw commands

  vkCmdEndRendering(commandBuffer);

  iio_transition_swapchain_image_layout(
    commandBuffer,
    imageIndex,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    0,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
    &state
  );

  result = vkEndCommandBuffer(commandBuffer);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }
}

void iio_record_testtriangle_command_buffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame) {
  VkResult result;
  VkCommandBufferBeginInfo beginInfo = {0};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.pNext = NULL;
  beginInfo.flags = 0;
  beginInfo.pInheritanceInfo = NULL;
  
  result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }

  iio_transition_swapchain_image_layout(
    commandBuffer,
    imageIndex,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    0,
    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    &state
  );

  VkRenderingAttachmentInfo colorAttachment = {
    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
    .clearValue = {
      .color.float32 = {0.5, 0.0, 0.5, 1.0},
    },
    .imageView = state.swapChainImageViews[imageIndex],
    .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
  };

  VkRenderingInfo renderingInfo = {
    .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
    .pNext = NULL,
    .flags = 0,
    .renderArea = {
      .offset = {0, 0},
      .extent = state.swapChainImageExtent
    },
    .layerCount = 1,
    .colorAttachmentCount = 1,
    .pColorAttachments = &colorAttachment,
    .viewMask = 0
  };
  
  vkCmdBeginRendering(commandBuffer, &renderingInfo);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state.graphicsPipelineManger.pipeline);

  VkViewport viewport = {0};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.height = (float) state.swapChainImageExtent.height;
  viewport.width = (float) state.swapChainImageExtent.width;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor = {0};
  scissor.offset = (VkOffset2D) {0, 0};
  scissor.extent = state.swapChainImageExtent;
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  vkCmdDraw(commandBuffer, 3, 1, 0, 0);

  vkCmdEndRendering(commandBuffer);

  iio_transition_swapchain_image_layout(
    commandBuffer,
    imageIndex,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    0,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
    &state
  );

  result = vkEndCommandBuffer(commandBuffer);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }
}

void iio_record_testcube_command_buffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame) {
  VkResult result;
  VkCommandBufferBeginInfo beginInfo = {0};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.pNext = NULL;
  beginInfo.flags = 0;
  beginInfo.pInheritanceInfo = NULL;
  
  result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }

  iio_transition_swapchain_image_layout(
    commandBuffer,
    imageIndex,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    0,
    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    &state
  );

  VkRenderingAttachmentInfo colorAttachment = {
    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
    .clearValue = {
      .color.float32 = {0.5, 0.0, 0.5, 1.0},
    },
    .imageView = state.swapChainImageViews[imageIndex],
    .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
  };

  VkRenderingAttachmentInfo depthAttachment = {
    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
    .clearValue = {
      .depthStencil.depth = 1.0f,
      .depthStencil.stencil = 0
    },
    .imageView = state.depthImageView,
    .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE
  };

  VkRenderingInfo renderingInfo = {
    .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
    .pNext = NULL,
    .flags = 0,
    .renderArea = {
      .offset = {0, 0},
      .extent = state.swapChainImageExtent
    },
    .layerCount = 1,
    .colorAttachmentCount = 1,
    .pColorAttachments = &colorAttachment,
    .pDepthAttachment = &depthAttachment,
    .viewMask = 0
  };
  
  vkCmdBeginRendering(commandBuffer, &renderingInfo);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state.graphicsPipelineManger.pipeline);

  VkViewport viewport = {0};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.height = (float) state.swapChainImageExtent.height;
  viewport.width = (float) state.swapChainImageExtent.width;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor = {0};
  scissor.offset = (VkOffset2D) {0, 0};
  scissor.extent = state.swapChainImageExtent;
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  VkBuffer vertexBuffers[] = {testCube.vertexBuffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
  vkCmdBindIndexBuffer(commandBuffer, testCube.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
  uint32_t vertexCount = sizeof(testCube.vertices) / sizeof(Vertex);
  uint32_t indexCount = sizeof(testCube.indices) / sizeof(uint32_t);

  glm_mat4_identity(testCube.modelUniformBufferData[currentFrame].position);
  glm_rotate(testCube.modelUniformBufferData[currentFrame].position, glm_rad(sin(glfwGetTime() *  5.0)), (vec3){1.0, 0.0, 0.0});
  glm_rotate(testCube.modelUniformBufferData[currentFrame].position, glm_rad(sin(glfwGetTime() *  7.0)), (vec3){0.0, 1.0, 0.0});
  glm_rotate(testCube.modelUniformBufferData[currentFrame].position, glm_rad(sin(glfwGetTime() * 19.0)), (vec3){0.0, 0.0, 1.0});

  memcpy(testCube.modelUniformBufferMapped[currentFrame], &testCube.modelUniformBufferData[currentFrame], sizeof(ModelUniformBufferData));

  VkDescriptorSet descriptorSets [3] = {
    state.cameraDescriptorSets[currentFrame],
    testCube.texSamplerDescriptorSets[currentFrame],
    testCube.modelUniformBufferDescriptorSets[currentFrame]
  };
  vkCmdBindDescriptorSets(
    commandBuffer, 
    VK_PIPELINE_BIND_POINT_GRAPHICS, 
    state.graphicsPipelineManger.layout, 
    0, 
    3, 
    descriptorSets, 
    0, 
    NULL
  );
  vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);

  vkCmdEndRendering(commandBuffer);

  iio_transition_swapchain_image_layout(
    commandBuffer,
    imageIndex,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    0,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
    &state
  );

  result = vkEndCommandBuffer(commandBuffer);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }
}

void iio_record_primitive_command_buffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame, IIOPrimitive * primitive) {

}

void iio_change_physical_device(VkPhysicalDevice physicalDevice) {

}

void iio_recreate_swapchain() {
  // fprintf(stdout, "recreating swapchain\n");
  int width, height;
  glfwGetFramebufferSize(state.window, &width, &height);
  while (width == 0 || height == 0) {
    glfwWaitEvents();
    glfwGetFramebufferSize(state.window, &width, &height);
  }
  vkDeviceWaitIdle(state.device);

  iio_cleanup_swapchain();
  
  iio_create_swapchain();
  iio_create_swapchain_image_views();
  iio_create_depth_resources();
}

/****************************************************************************************************
 *                                      Cleanup the Vulkan API                                      *
 ****************************************************************************************************/

void iio_cleanup() {
  fprintf(stdout, "Cleaning up Vulkan API.\n");
  iio_cleanup_swapchain();
  //  Clean up the default textures
  iio_destroy_resources(state.device);

  iio_destroy_image(state.device, testTextureFilename, &state.resourceManager);
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (state.globalUniformBuffers) vkDestroyBuffer(state.device, state.globalUniformBuffers[i], NULL);
    if (state.globalUniformBuffersMemory) vkFreeMemory(state.device, state.globalUniformBuffersMemory[i], NULL);
  }

  if (testCube.indexBuffer) vkDestroyBuffer(state.device, testCube.indexBuffer, NULL);
  if (testCube.indexBufferMemory) vkFreeMemory(state.device, testCube.indexBufferMemory, NULL);
  if (testCube.vertexBuffer) vkDestroyBuffer(state.device, testCube.vertexBuffer, NULL);
  if (testCube.vertexBufferMemory) vkFreeMemory(state.device, testCube.vertexBufferMemory, NULL);
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (testCube.modelUniformBuffer[i]) vkDestroyBuffer(state.device, testCube.modelUniformBuffer[i], NULL);
    if (testCube.modelUniformBufferMemory[i]) vkFreeMemory(state.device, testCube.modelUniformBufferMemory[i], NULL);
  }

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (state.imageAvailableSemaphores) vkDestroySemaphore(state.device, state.imageAvailableSemaphores[i], NULL);
    if (state.inFlightFences) vkDestroyFence(state.device, state.inFlightFences[i], NULL);
  }
  for (int i = 0; i < state.swapChainImageCount; i++) {
    if (state.renderFinishedSemaphores) vkDestroySemaphore(state.device, state.renderFinishedSemaphores[i], NULL);
  }
  if (state.descriptorPoolMangers) {
    for (uint32_t i = 0; i < state.descriptorPoolManagerCount; i++) {
      iio_destroy_descriptor_pool_manager(state.device, &state.descriptorPoolMangers[i]);
    }
  }
  if (state.imageAvailableSemaphores) free(state.imageAvailableSemaphores);
  if (state.renderFinishedSemaphores) free(state.renderFinishedSemaphores);
  if (state.inFlightFences) free(state.inFlightFences);
  if (state.commandPool) vkDestroyCommandPool(state.device, state.commandPool, NULL);
  iio_destroy_graphics_pipeline(state.device, &state.graphicsPipelineManger);
  if (state.device) vkDestroyDevice(state.device, NULL);
  if (state.physicalDevices) free(state.physicalDevices);
  if (state.surface) vkDestroySurfaceKHR(state.instance, state.surface, NULL);
  if (state.instance) vkDestroyInstance(state.instance, NULL);
  if (state.window) glfwDestroyWindow(state.window);
  glfwTerminate();
}

void iio_cleanup_device() {
  
  iio_cleanup_swapchain();

}

void iio_cleanup_swapchain() {
  if (state.depthImageView) vkDestroyImageView(state.device, state.depthImageView, NULL);
  if (state.depthImage) vkDestroyImage(state.device, state.depthImage, NULL);
  if (state.depthImageMemory) vkFreeMemory(state.device, state.depthImageMemory, NULL);
  if (state.swapChainImageViews) {
    for (uint32_t i = 0; i < state.swapChainImageCount; i++) {
      vkDestroyImageView(state.device, state.swapChainImageViews[i], NULL);
    }
    free(state.swapChainImageViews);
  }
  if (state.swapChainImages) free(state.swapChainImages);
  if (state.swapChain) vkDestroySwapchainKHR(state.device, state.swapChain, NULL);
}