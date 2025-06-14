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
#include "iio_model.h"



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

// static double startTime = 0.0;

double deltaTime = 0.0;

/****************************************************************************************************
 *                                             Constants                                            *
 ****************************************************************************************************/

const char * deviceExtensions [] = {
  "VK_KHR_swapchain",
};

const int MAX_FRAMES_IN_FLIGHT = 2;

const Vertex testCube [] = {
  //  positions          //  colors          //  uv
  {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}}, //  front bottom left
  {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, //  front bottom right
  {{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}, //  front top left
  {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, //  front top right

  {{-0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, //  back bottom left
  {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, //  back bottom right
  {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, //  back top left
  {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}, //  back top right
};

const uint32_t testCubeIndices [] = {
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
};

const float cameraSpeed = 5.0f;

const char * testTexturePath = "src/textures/269670.png";

/****************************************************************************************************
 *                           Functions for initializing the Vulkan API                              *
 ****************************************************************************************************/

IIOVulkanState * iio_init_vulkan_api() {
  atexit(iio_cleanup);
  state.currentFrame = 0;
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
  iio_create_render_pass();
  iio_create_descriptor_set_layout();
  iio_create_graphics_pipeline();
  iio_create_command_pool();
  iio_create_depth_resources();
  iio_create_framebuffers();

  iio_create_texture_image(testTexturePath, &state.textureImage, &state.textureImageMemory);
  iio_create_texture_image_view(state.textureImage, &state.textureImageView);
  iio_create_texture_sampler(&state.textureSampler);
  iio_create_vertex_buffer();
  iio_create_index_buffer();

  iio_create_uniform_buffers();
  iio_create_descriptor_pool();
  iio_create_descriptor_sets();
  iio_create_command_buffers();
  iio_create_synchronization_objects();

  iio_load_test_model();
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
  // glfwSetInputMode(state.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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

  VkResult result = vkCreateDevice(state.selectedDevice, &deviceCreateInfo, NULL, &state.device);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }
  vkGetDeviceQueue(state.device, state.graphicsQueueFamilyIndex, 0, &state.graphicsQueue);
  vkGetDeviceQueue(state.device, state.presentQueueFamilyIndex, 0, &state.presentQueue);
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
  swapchainCreateInfo.imageFormat = VK_FORMAT_B8G8R8A8_SRGB; // TODO: make this configurable
  swapchainCreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR; // TODO: make this configurable
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

void iio_create_render_pass() {
  fprintf(stdout, "Creating render pass.\n");
  VkAttachmentDescription colorAttachmentDescription = {0};
  colorAttachmentDescription.flags = 0;
  colorAttachmentDescription.format = state.surfaceFormat.format;
  colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentReference = {0};
  colorAttachmentReference.attachment = 0;
  colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription depthAttachmentDescription = {0};
  depthAttachmentDescription.flags = 0;
  depthAttachmentDescription.format = iio_find_depth_format();
  depthAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthAttachmentReference = {0};
  depthAttachmentReference.attachment = 1; // This will be set later
  depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpassDescription = {0};
  subpassDescription.flags = 0;
  subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassDescription.inputAttachmentCount = 0;
  subpassDescription.pInputAttachments = NULL;
  subpassDescription.colorAttachmentCount = 1;
  subpassDescription.pColorAttachments = &colorAttachmentReference;
  subpassDescription.pResolveAttachments = NULL;
  subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;
  subpassDescription.preserveAttachmentCount = 0;
  subpassDescription.pPreserveAttachments = NULL;

  VkSubpassDependency subpassDependency = {0};
  subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  subpassDependency.dstSubpass = 0;
  subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | 
                                   VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | 
                                   VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  subpassDependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  subpassDependency.dependencyFlags;

  VkRenderPassCreateInfo renderPassCreateInfo = {0};
  renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassCreateInfo.pNext = NULL;
  renderPassCreateInfo.flags = 0;
  renderPassCreateInfo.attachmentCount = 2;
  renderPassCreateInfo.pAttachments = (VkAttachmentDescription []) {colorAttachmentDescription, depthAttachmentDescription};
  renderPassCreateInfo.subpassCount = 1;
  renderPassCreateInfo.pSubpasses = &subpassDescription;
  renderPassCreateInfo.dependencyCount = 1;
  renderPassCreateInfo.pDependencies = &subpassDependency;

  VkResult result = vkCreateRenderPass(state.device, &renderPassCreateInfo, NULL, &state.renderPass);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }
}

void iio_create_descriptor_set_layout() {
  VkDescriptorSetLayoutBinding uboLayoutBinding = {0};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  uboLayoutBinding.pImmutableSamplers = NULL;

  VkDescriptorSetLayoutBinding samplerLayoutBinding = {0};
  samplerLayoutBinding.binding = 1;
  samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.descriptorCount = 1;
  samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  samplerLayoutBinding.pImmutableSamplers = NULL;

  VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {0};
  layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutCreateInfo.pNext = NULL;
  layoutCreateInfo.bindingCount = 2;
  layoutCreateInfo.pBindings = (VkDescriptorSetLayoutBinding []) {uboLayoutBinding, samplerLayoutBinding};

  VkResult result = vkCreateDescriptorSetLayout(state.device, &layoutCreateInfo, NULL, &state.descriptorSetLayout);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }
}

void iio_create_graphics_pipeline() {
  fprintf(stdout, "Creating shader pipeline.\n");
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

  VkPipelineShaderStageCreateInfo shaderStages[2] = {0};
  shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  shaderStages[0].module = vertShaderModule;
  shaderStages[0].pName = "main";
  shaderStages[0].pNext = NULL;
  shaderStages[0].flags = 0;
  shaderStages[0].pSpecializationInfo = NULL;
  shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  shaderStages[1].module = fragShaderModule;
  shaderStages[1].pName = "main";
  shaderStages[1].pNext = NULL;
  shaderStages[1].flags = 0;
  shaderStages[1].pSpecializationInfo = NULL;

  fprintf(stdout, "Shader stages created successfully.\n");

  VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {0};
  dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicStateCreateInfo.pNext = NULL;
  dynamicStateCreateInfo.flags = 0;
  dynamicStateCreateInfo.dynamicStateCount = 2;
  VkDynamicState dynamicStates[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
  };
  dynamicStateCreateInfo.pDynamicStates = dynamicStates;

  fprintf(stdout, "Dynamic state created successfully.\n");

  int vertexAttributeCount = 0;
  VkVertexInputBindingDescription bindingDescription = iio_get_binding_description();
  VkVertexInputAttributeDescription * attributeDescriptions = iio_get_attribute_descriptions(&vertexAttributeCount);

  VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {0};
  vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputStateCreateInfo.pNext = NULL;
  vertexInputStateCreateInfo.flags = 0;
  vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
  vertexInputStateCreateInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputStateCreateInfo.vertexAttributeDescriptionCount = vertexAttributeCount;
  vertexInputStateCreateInfo.pVertexAttributeDescriptions = attributeDescriptions;

  VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {0};
  inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssemblyStateCreateInfo.pNext = NULL;
  inputAssemblyStateCreateInfo.flags = 0;
  inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

  VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {0};
  viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportStateCreateInfo.pNext = NULL;
  viewportStateCreateInfo.flags = 0;
  viewportStateCreateInfo.viewportCount = 1;
  viewportStateCreateInfo.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {0};
  rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizationStateCreateInfo.pNext = NULL;
  rasterizationStateCreateInfo.flags = 0;
  rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
  rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
  rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizationStateCreateInfo.lineWidth = 1.0f;
  rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
  rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
  rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
  rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;

  VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {0};
  multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampleStateCreateInfo.pNext = NULL;
  multisampleStateCreateInfo.flags = 0;
  multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
  multisampleStateCreateInfo.minSampleShading = 1.0f;
  multisampleStateCreateInfo.pSampleMask = NULL;
  multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
  multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

  VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {0};
  colorBlendAttachmentState.blendEnable = VK_FALSE;
  colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                             VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {0};
  colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlendStateCreateInfo.pNext = NULL;
  colorBlendStateCreateInfo.flags = 0;
  colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
  colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
  colorBlendStateCreateInfo.attachmentCount = 1;
  colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;
  colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
  colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
  colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
  colorBlendStateCreateInfo.blendConstants[3] = 0.0f;

  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {0};
  pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutCreateInfo.pNext = NULL;
  pipelineLayoutCreateInfo.flags = 0;
  pipelineLayoutCreateInfo.setLayoutCount = 1;
  pipelineLayoutCreateInfo.pSetLayouts = &state.descriptorSetLayout;
  pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
  pipelineLayoutCreateInfo.pPushConstantRanges = NULL;

  VkResult result = vkCreatePipelineLayout(state.device, &pipelineLayoutCreateInfo, NULL, &state.pipelineLayout);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }

  fprintf(stdout, "Pipeline layout created successfully.\n");

  VkPipelineDepthStencilStateCreateInfo depthStencilState = {0};
  depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencilState.pNext = NULL;
  depthStencilState.flags = 0;
  depthStencilState.depthTestEnable = VK_TRUE;
  depthStencilState.depthWriteEnable = VK_TRUE;
  depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencilState.depthBoundsTestEnable = VK_FALSE;
  depthStencilState.minDepthBounds = 0.0f;
  depthStencilState.maxDepthBounds = 1.0f;
  depthStencilState.stencilTestEnable = VK_FALSE;
  depthStencilState.front = (VkStencilOpState){0};
  depthStencilState.back = (VkStencilOpState){0};

  VkGraphicsPipelineCreateInfo pipelineCreateInfo = {0};
  pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineCreateInfo.pNext = NULL;
  pipelineCreateInfo.flags = 0;
  pipelineCreateInfo.stageCount = 2;
  pipelineCreateInfo.pStages = shaderStages;
  pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
  pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
  pipelineCreateInfo.pTessellationState = NULL;
  pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
  pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
  pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
  pipelineCreateInfo.pDepthStencilState = &depthStencilState;
  pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
  pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
  pipelineCreateInfo.layout = state.pipelineLayout;
  pipelineCreateInfo.renderPass = state.renderPass;
  pipelineCreateInfo.subpass = 0;
  pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineCreateInfo.basePipelineIndex = -1;

  fprintf(stdout, "Pipeline create info set up successfully.\n");

  result = vkCreateGraphicsPipelines(state.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, NULL, &state.graphicsPipeline);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }

  fprintf(stdout, "Graphics pipeline created successfully.\n");

  vkDestroyShaderModule(state.device, vertShaderModule, NULL);
  vkDestroyShaderModule(state.device, fragShaderModule, NULL);
}

void iio_create_framebuffers() {
  state.framebuffers = malloc(state.swapChainImageCount * sizeof(VkFramebuffer));
  for (int i = 0; i < state.swapChainImageCount; i++) {
    VkImageView attachments[] = {
      state.swapChainImageViews[i],
      state.depthImageView
    };

    VkFramebufferCreateInfo framebufferCreateInfo = {0};
    framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCreateInfo.renderPass = state.renderPass;
    framebufferCreateInfo.attachmentCount = sizeof(attachments) / sizeof(VkImageView);
    framebufferCreateInfo.pAttachments = attachments;
    framebufferCreateInfo.width = state.swapChainImageExtent.width;
    framebufferCreateInfo.height = state.swapChainImageExtent.height;
    framebufferCreateInfo.layers = 1;

    VkResult result = vkCreateFramebuffer(state.device, &framebufferCreateInfo, NULL, &state.framebuffers[i]);
    if (result != VK_SUCCESS) {
      iio_vk_error(result, __LINE__, __FILE__);
      exit(1);
    }
  }
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

static VkFormat iio_find_depth_format() {
  static VkFormat format;
  static uint8_t initialized = 0;
  if (initialized) {
    return format;
  }
  const VkFormat candidates [3] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
  VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
  VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
  format = iio_find_supported_format(candidates, 3, tiling, features);
  if (format != VK_FORMAT_UNDEFINED) {
    initialized = 1;
  }
  return format;
}

void iio_create_vertex_buffer() {
  VkDeviceSize bufferSize = sizeof(testCube);
  
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
  memcpy(data, testCube, sizeof(testCube));
  vkUnmapMemory(state.device, stagingBufferMemory);
  fprintf(stdout, "Data copied to staging buffer successfully.\n");
  fprintf(stdout, "Creating vertex buffer.\n");
  iio_create_buffer(
    bufferSize, 
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
    &state.vertexBuffer, 
    &state.vertexBufferMemory
  );
  fprintf(stdout, "Vertex buffer created successfully.\n");
  fprintf(stdout, "Copying data from staging buffer to vertex buffer.\n");
  iio_copy_buffer(stagingBuffer, state.vertexBuffer, bufferSize);
  fprintf(stdout, "Data copied successfully.\n");
  vkDestroyBuffer(state.device, stagingBuffer, NULL);
  vkFreeMemory(state.device, stagingBufferMemory, NULL);
}

void iio_create_index_buffer() {
  VkDeviceSize bufferSize = sizeof(testCubeIndices);
  
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
  memcpy(data, testCubeIndices, sizeof(testCubeIndices));
  vkUnmapMemory(state.device, stagingBufferMemory);
  fprintf(stdout, "Data copied to staging buffer for index buffer successfully.\n");

  fprintf(stdout, "Creating index buffer.\n");
  iio_create_buffer(
    bufferSize, 
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
    &state.indexBuffer, 
    &state.indexBufferMemory
  );
  fprintf(stdout, "Index buffer created successfully.\n");

  fprintf(stdout, "Copying data from staging buffer to index buffer.\n");
  iio_copy_buffer(stagingBuffer, state.indexBuffer, bufferSize);
  fprintf(stdout, "Data copied to index buffer successfully.\n");

  vkDestroyBuffer(state.device, stagingBuffer, NULL);
  vkFreeMemory(state.device, stagingBufferMemory, NULL);
}

void iio_create_uniform_buffers() {
  fprintf(stdout, "Creating uniform buffers.\n");
  VkDeviceSize bufferSize = sizeof(UniformBufferObject);
  state.uniformBuffers = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkBuffer));
  state.uniformBuffersMemory = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkDeviceMemory));
  state.uniformBuffersMapped = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(void *));
  if (!state.uniformBuffers || !state.uniformBuffersMemory || !state.uniformBuffersMapped) {
    iio_oom_error(NULL, __LINE__, __FILE__);
    exit(1);
  }
  
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    iio_create_buffer(
      bufferSize,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      &state.uniformBuffers[i],
      &state.uniformBuffersMemory[i]
    );
    vkMapMemory(state.device, state.uniformBuffersMemory[i], 0, bufferSize, 0, &state.uniformBuffersMapped[i]);
  }
}

void iio_create_descriptor_pool() {
  fprintf(stdout, "Creating descriptor pool.\n");
  VkDescriptorPoolSize poolSizes [2] = {0};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = (uint32_t) MAX_FRAMES_IN_FLIGHT;
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = (uint32_t) MAX_FRAMES_IN_FLIGHT;

  VkDescriptorPoolCreateInfo poolCreateInfo = {0};
  poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolCreateInfo.pNext = NULL;
  poolCreateInfo.flags = 0;
  poolCreateInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
  poolCreateInfo.poolSizeCount = 2;
  poolCreateInfo.pPoolSizes = poolSizes;

  VkResult result = vkCreateDescriptorPool(state.device, &poolCreateInfo, NULL, &state.descriptorPool);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }
}

void iio_create_descriptor_sets() {
  fprintf(stdout, "Creating descriptor sets.\n");
  state.descriptorSets = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkDescriptorSet));
  if (!state.descriptorSets) {
    iio_oom_error(NULL, __LINE__, __FILE__);
    exit(1);
  }

  VkDescriptorSetLayout layouts[MAX_FRAMES_IN_FLIGHT];
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    layouts[i] = state.descriptorSetLayout;
  }

  VkDescriptorSetAllocateInfo allocateInfo = {0};
  allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocateInfo.pNext = NULL;
  allocateInfo.descriptorPool = state.descriptorPool;
  allocateInfo.descriptorSetCount = (uint32_t) MAX_FRAMES_IN_FLIGHT;
  allocateInfo.pSetLayouts = layouts;

  VkResult result = vkAllocateDescriptorSets(state.device, &allocateInfo, state.descriptorSets);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo bufferInfo = {0};
    bufferInfo.buffer = state.uniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkDescriptorImageInfo imageInfo = {0};
    imageInfo.sampler = state.textureSampler;
    imageInfo.imageView = state.textureImageView;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet descriptorWrites [2] = {0};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].pNext = NULL;
    descriptorWrites[0].dstSet = state.descriptorSets[i];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].pImageInfo = NULL;
    descriptorWrites[0].pBufferInfo = &bufferInfo;
    descriptorWrites[0].pTexelBufferView = NULL;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].pNext = NULL;
    descriptorWrites[1].dstSet = state.descriptorSets[i];
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].pImageInfo = &imageInfo;
    descriptorWrites[1].pBufferInfo = NULL;
    descriptorWrites[1].pTexelBufferView = NULL;

    vkUpdateDescriptorSets(state.device, 2, descriptorWrites, 0, NULL);
  }
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
  state.renderFinishedSemaphores = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkSemaphore));
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
      result = vkCreateSemaphore(state.device, &semaphoreCreateInfo, NULL, &state.renderFinishedSemaphores[i]);
    } else {
      iio_vk_error(result, __LINE__, __FILE__);
      exit(1);
    }
    if (result == VK_SUCCESS) {
      result = vkCreateFence(state.device, &fenceCreateInfo, NULL, &state.inFlightFences[i]);
    } else {
      iio_vk_error(result, __LINE__, __FILE__);
      exit(1);
    }
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
  VkPhysicalDeviceType * deviceType
) { //  iio_select_physical_device_properties
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
  for (int i = 0; i < presentModeCount; i++) {
    if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
      *preferredPresentMode = presentModes[i];
      break;
    } else if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      *preferredPresentMode = presentModes[i];
    } else if (presentModes[i] == VK_PRESENT_MODE_FIFO_KHR &&
                *preferredPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) {
      *preferredPresentMode = presentModes[i];
    } else if (presentModes[i] == VK_PRESENT_MODE_FIFO_RELAXED_KHR &&
                *preferredPresentMode != VK_PRESENT_MODE_MAILBOX_KHR &&
                *preferredPresentMode != VK_PRESENT_MODE_FIFO_KHR) {
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
  static VkVertexInputAttributeDescription attributeDescriptions[2] = {0};

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
  VkDeviceMemory * bufferMemory
) { //  iio_create_buffer
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

void iio_update_uniform_buffer(uint32_t currentFrame) {
  double elapsedTime = glfwGetTime();
  float fov = 45.0f;
  UniformBufferObject ubo = {0};
  glm_mat4_identity(ubo.model);
  glm_rotate(ubo.model, glm_rad(elapsedTime * 533.0f), (vec3) {0.0f, 1.0f, 0.0f});
  glm_rotate(ubo.model, glm_rad(elapsedTime * 199.0f), (vec3) {1.0f, 0.0f, 0.0f});
  glm_rotate(ubo.model, glm_rad(elapsedTime * 311.0f), (vec3) {0.0f, 0.0f, 1.0f});
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
  memcpy(state.uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
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

void iio_create_texture_image_view(VkImage textureImage, VkImageView * textureImageView) {
  iio_create_image_view(textureImage, textureImageView, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void iio_create_texture_sampler(VkSampler * textureSampler) {
  iio_create_image_sampler(textureSampler);
}

void iio_create_image(
  uint32_t width,
  uint32_t height,
  VkImage * textureImage,
  VkDeviceMemory * textureImageMemory,
  VkFormat format,
  VkImageTiling tiling,
  VkImageUsageFlags usage,
  VkMemoryPropertyFlags properties
) { //  iio_create_image
  VkImageCreateInfo imageCreateInfo = {0};
  imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  imageCreateInfo.format = format; // Assuming RGBA format
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

void iio_sleep(uint32_t ms) {
  //  Sleep for the specified number of milliseconds
  if (ms > 0) {
    struct timespec ts = {0};
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
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

void iio_process_input(GLFWwindow * window) {
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
}

/****************************************************************************************************
 *                                              Runtime                                             *
 ****************************************************************************************************/

void iio_run() {
  double reportInterval = 2.0; // seconds
  double nextReport = reportInterval;
  double lastReport = 0.0;
  double lastTime = 0.0;
  uint32_t frameCount = 0;
  uint32_t targetFPS = 200;
  while (!glfwWindowShouldClose(state.window)) {
    double currentTime = glfwGetTime();
    deltaTime = currentTime - lastTime;
    lastTime = currentTime;
    iio_process_input(state.window);
    if (currentTime >= nextReport) {
      fprintf(stdout, "FPS: %f\n", (double) frameCount / (currentTime - lastReport));
      frameCount = 0;
      lastReport = currentTime;
      nextReport += reportInterval;
    }
    glfwPollEvents();
    draw_frame();
    frameCount++;
    //  sleep for a short duration to limit the frame rate
    if (currentTime - lastReport < frameCount * (1.0 / targetFPS)) {
      iio_sleep((uint32_t) ((frameCount * (1.0 / targetFPS) - (currentTime - lastReport)) * 1000));
    }
  }
  vkDeviceWaitIdle(state.device);
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

  iio_update_uniform_buffer(state.currentFrame);
  
  vkResetFences(state.device, 1, &state.inFlightFences[state.currentFrame]);
  vkResetCommandBuffer(state.commandBuffers[state.currentFrame], 0);
  iio_record_command_buffer(state.commandBuffers[state.currentFrame], imageIndex);
  VkPipelineStageFlags flags [] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSubmitInfo submitInfo = {0};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &state.imageAvailableSemaphores[state.currentFrame];
  submitInfo.pWaitDstStageMask = flags;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &state.commandBuffers[state.currentFrame];
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &state.renderFinishedSemaphores[state.currentFrame];
  result = vkQueueSubmit(state.graphicsQueue, 1, &submitInfo, state.inFlightFences[state.currentFrame]);
  VkPresentInfoKHR presentInfo = {0};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &state.renderFinishedSemaphores[state.currentFrame];
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &state.swapChain;
  presentInfo.pImageIndices = &imageIndex;
  presentInfo.pResults = NULL;
  vkQueuePresentKHR(state.presentQueue, &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || state.framebufferResized) {
    state.framebufferResized = 0;
    iio_recreate_swapchain();
  } else if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }

  state.currentFrame = (state.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void iio_record_command_buffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
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

  VkRenderPassBeginInfo renderPassBeginInfo = {0};
  renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassBeginInfo.pNext = NULL;
  renderPassBeginInfo.renderPass = state.renderPass;
  renderPassBeginInfo.framebuffer = state.framebuffers[imageIndex];
  renderPassBeginInfo.renderArea.offset.x = 0;
  renderPassBeginInfo.renderArea.offset.y = 0;
  renderPassBeginInfo.renderArea.extent.width = state.swapChainImageExtent.width;
  renderPassBeginInfo.renderArea.extent.height = state.swapChainImageExtent.height;
  VkClearValue clearValues [2] = {0};
  clearValues[0].color.float32[0] = 0.0f;
  clearValues[0].color.float32[1] = 0.0f;
  clearValues[0].color.float32[2] = 0.0f;
  clearValues[0].color.float32[3] = 1.0f;
  clearValues[1].depthStencil.depth = 1.0f;
  clearValues[1].depthStencil.stencil = 0;
  renderPassBeginInfo.clearValueCount = sizeof(clearValues) / sizeof(VkClearValue);
  renderPassBeginInfo.pClearValues = clearValues;
  vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state.graphicsPipeline);

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

  VkBuffer vertexBuffers[] = {state.vertexBuffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
  vkCmdBindIndexBuffer(commandBuffer, state.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
  uint32_t vertexCount = sizeof(testCube) / sizeof(Vertex);
  uint32_t indexCount = sizeof(testCubeIndices) / sizeof(uint32_t);

  vkCmdBindDescriptorSets(
    commandBuffer, 
    VK_PIPELINE_BIND_POINT_GRAPHICS, 
    state.pipelineLayout, 
    0, 
    1, 
    &state.descriptorSets[state.currentFrame], 
    0, 
    NULL
  );
  vkCmdDrawIndexed(commandBuffer, indexCount, 2, 0, 0, 0);

  vkCmdEndRenderPass(commandBuffer);

  result = vkEndCommandBuffer(commandBuffer);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    exit(1);
  }
}

void iio_change_physical_device(VkPhysicalDevice physicalDevice) {

}

void iio_recreate_swapchain() {
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
  iio_create_framebuffers();
}

/****************************************************************************************************
 *                                      Cleanup the Vulkan API                                      *
 ****************************************************************************************************/

void iio_cleanup() {
  fprintf(stdout, "Cleaning up Vulkan API.\n");
  iio_cleanup_swapchain();
  if (state.textureSampler) vkDestroySampler(state.device, state.textureSampler, NULL);
  if (state.textureImageView) vkDestroyImageView(state.device, state.textureImageView, NULL);
  if (state.textureImage) vkDestroyImage(state.device, state.textureImage, NULL);
  if (state.textureImageMemory) vkFreeMemory(state.device, state.textureImageMemory, NULL);
  if (state.graphicsPipeline) vkDestroyPipeline(state.device, state.graphicsPipeline, NULL);
  if (state.pipelineLayout) vkDestroyPipelineLayout(state.device, state.pipelineLayout, NULL);
  if (state.renderPass) vkDestroyRenderPass(state.device, state.renderPass, NULL);
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (state.uniformBuffers) vkDestroyBuffer(state.device, state.uniformBuffers[i], NULL);
    if (state.uniformBuffersMemory) vkFreeMemory(state.device, state.uniformBuffersMemory[i], NULL);
  }
  if (state.uniformBuffers) free(state.uniformBuffers);
  if (state.uniformBuffersMemory) free(state.uniformBuffersMemory);
  if (state.descriptorPool) vkDestroyDescriptorPool(state.device, state.descriptorPool, NULL);
  if (state.descriptorSetLayout) vkDestroyDescriptorSetLayout(state.device, state.descriptorSetLayout, NULL);
  if (state.indexBuffer) vkDestroyBuffer(state.device, state.indexBuffer, NULL);
  if (state.indexBufferMemory) vkFreeMemory(state.device, state.indexBufferMemory, NULL);
  if (state.vertexBuffer) vkDestroyBuffer(state.device, state.vertexBuffer, NULL);
  if (state.vertexBufferMemory) vkFreeMemory(state.device, state.vertexBufferMemory, NULL);
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (state.imageAvailableSemaphores) vkDestroySemaphore(state.device, state.imageAvailableSemaphores[i], NULL);
    if (state.renderFinishedSemaphores) vkDestroySemaphore(state.device, state.renderFinishedSemaphores[i], NULL);
    if (state.inFlightFences) vkDestroyFence(state.device, state.inFlightFences[i], NULL);
  }
  if (state.imageAvailableSemaphores) free(state.imageAvailableSemaphores);
  if (state.renderFinishedSemaphores) free(state.renderFinishedSemaphores);
  if (state.inFlightFences) free(state.inFlightFences);
  if (state.commandPool) vkDestroyCommandPool(state.device, state.commandPool, NULL);
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
  if (state.depthImage) vkDestroyImage(state.device, state.depthImage, NULL);
  if (state.depthImageMemory) vkFreeMemory(state.device, state.depthImageMemory, NULL);
  if (state.depthImageView) vkDestroyImageView(state.device, state.depthImageView, NULL);
  if (state.framebuffers) {
    for (uint32_t i = 0; i < state.swapChainImageCount; i++) {
      vkDestroyFramebuffer(state.device, state.framebuffers[i], NULL);
    }
    free(state.framebuffers);
  }
  if (state.swapChainImageViews) {
    for (uint32_t i = 0; i < state.swapChainImageCount; i++) {
      vkDestroyImageView(state.device, state.swapChainImageViews[i], NULL);
    }
    free(state.swapChainImageViews);
  }
  if (state.swapChainImages) free(state.swapChainImages);
  if (state.swapChain) vkDestroySwapchainKHR(state.device, state.swapChain, NULL);
}