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

#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include "cglm/cglm.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "fast_obj.h"

#define DEBUG

#define ATTRIBUTE_COUNT 3

typedef struct AppStruct_S {
  //Needs to be cleaned up
  GLFWwindow* window;
  VkInstance instance;
  VkDevice device;
  VkSurfaceKHR surface;
  VkSwapchainKHR swapChain;
  VkImage * swapChainImages;
  uint32_t swapChainImageCount;
  VkImageView * swapChainImageViews;
  VkRenderPass renderPass;
  VkDescriptorSetLayout descriptorSetLayout;
  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;
  VkFramebuffer * swapChainFramebuffers;
  VkCommandPool commandPool;
  VkCommandBuffer *  commandBuffers;
  VkSemaphore * imageAvailableSemaphores;
  VkSemaphore * renderFinishedSemaphores;
  VkFence * inFlightFences;
  uint8_t framebufferResized;
  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  VkBuffer indexBuffer;
  VkDeviceMemory indexBufferMemory;
  VkDescriptorPool descriptorPool;
  VkDescriptorSet * descriptorSets;
  VkImage textureImage;
  VkDeviceMemory textureImageMemory;
  VkImageView textureImageView;
  VkSampler textureSampler;

  Vertex * vertices;
  uint32_t vertexCount;
  uint32_t * indices;
  uint32_t indexCount;

  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  
  VkBuffer * uniformBuffers;
  VkDeviceMemory * uniformBuffersMemory;
  void ** uniformBuffersMapped;

  VkImage depthImage;
  VkDeviceMemory depthImageMemory;
  VkImageView depthImageView;

  //Does not need to be cleaned up
  VkPhysicalDevice physicalDevice;
  VkQueue graphicsQueue;
  VkQueue presentQueue;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;
  uint32_t currentFrame;
} AppStruct;

typedef struct QueueFamilyIndices_S {
  uint32_t graphicsFamily;
  uint32_t hasGraphicsFamily;
  uint32_t presentFamily;
  uint32_t hasPresentFamily;
} QueueFamilyIndices;

typedef struct SwapChainSupportDetails_S {
  VkSurfaceCapabilitiesKHR capabilities;
  VkSurfaceFormatKHR * formats;
  uint32_t formatCount;
  VkPresentModeKHR * presentModes;
  uint32_t presentModeCount;
} SwapChainSupportDetails;

typedef struct ShaderSource_S {
  uint32_t * data;
  uint32_t codeSize;
} ShaderSource;

typedef struct Vertex_S {
  vec3 pos;
  vec3 color;
  vec2 texCoord;
} Vertex;

typedef struct UniformBufferObject_S {
  mat4 model;
  mat4 view;
  mat4 proj;
} UniformBufferObject;

typedef AppStruct * pAsobj;

int recreateSwapChain(pAsobj obj);

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

static double startTime = 0.0;

const char * deviceExtensions[] = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

/* const Vertex vertices[] = {
  {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
  {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
  {{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
  {{-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

  {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
  {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
  {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
  {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
};

const uint16_t indices[] = {
  0, 1, 2,
  2, 3, 0,

  4, 5, 6,
  6, 7, 4,
}; */

const char MODEL_PATH [] = "models/viking_room.obj";
const char TEXTURE_PATH [] = "textures/viking_room.png";

#ifdef DEBUG
const char * validationLayers[] = {
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

  int layerFound = 0;
  for (int i = 0; i < validationLayerCount; i++) {
    for (int j = 0; j < layerCount; j++) {
      if (strcmp(validationLayers[i], availableLayers[j].layerName) == 0) {
        layerFound = 1;
        break;
      }
    }
  }

  return layerFound;
}
#endif

/**
 * Vertex Functions
 */

VkVertexInputBindingDescription getBindingDescription() {
  VkVertexInputBindingDescription bindingDescription = {0};
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(Vertex);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  return bindingDescription;
}

VkVertexInputAttributeDescription * getAttributeDescriptions(uint32_t * count) {
  *count = ATTRIBUTE_COUNT;
  static VkVertexInputAttributeDescription attributeDescriptions[ATTRIBUTE_COUNT] = {0};
  attributeDescriptions[0].binding = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[0].offset = offsetof(Vertex, pos);

  attributeDescriptions[1].binding = 0;
  attributeDescriptions[1].location = 1;
  attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[1].offset = offsetof(Vertex, color);

  attributeDescriptions[2].binding = 0;
  attributeDescriptions[2].location = 2;
  attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
  attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
  
  return attributeDescriptions;
}

/**
 * Vulkan Functions
 */

VkFormat findSupportedFormat(
  pAsobj obj,
  const VkFormat * candidates,
  uint32_t candidateCount,
  VkImageTiling tiling,
  VkFormatFeatureFlags features
) {
  for (uint32_t i = 0; i < candidateCount; i++) {
    VkFormatProperties props;
#ifdef GLAD_VULKAN
    glad_vkGetPhysicalDeviceFormatProperties(obj->physicalDevice, candidates[i], &props);
#else
    vkGetPhysicalDeviceFormatProperties(obj->physicalDevice, candidates[i], &props);
#endif
    if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
      return candidates[i];
    } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
      return candidates[i];
    }
  }

  fprintf(stderr, "failed to find supported format\n");
  return candidates[0];
}

VkFormat findDepthFormat(pAsobj obj) {
  return findSupportedFormat(
    obj,
    (VkFormat []) {
      VK_FORMAT_D32_SFLOAT,
      VK_FORMAT_D24_UNORM_S8_UINT,
      VK_FORMAT_D16_UNORM
    },
    3,
    VK_IMAGE_TILING_OPTIMAL,
    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
  );
}

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, pAsobj obj) {
  QueueFamilyIndices indices = {0};
  uint32_t queueFamilyCount = 0;
#ifdef GLAD_VULKAN
  glad_vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);
  VkQueueFamilyProperties queueFamilies [queueFamilyCount];
  glad_vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);
#else
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);
  VkQueueFamilyProperties queueFamilies [queueFamilyCount];
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);
#endif
  for (int i = 0; i < queueFamilyCount; i++) {
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphicsFamily = i;
      indices.hasGraphicsFamily = 1;
    }
    VkBool32 presentSupport = 0;
#ifdef GLAD_VULKAN
    glad_vkGetPhysicalDeviceSurfaceSupportKHR(device, i, obj->surface, &presentSupport);
#else
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, obj->surface, &presentSupport);
#endif
    if (presentSupport) {
      indices.presentFamily = i;
      indices.hasPresentFamily = 1;
    }
    if (indices.hasGraphicsFamily && indices.hasPresentFamily) {
      break;
    } 
  }
  return indices;
}

int recordCommandBuffer(pAsobj obj, uint32_t imageIndex) {
  int code = 1;
  /**
   * Begin recording command buffer
   */
  VkCommandBufferBeginInfo beginInfo = {0};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0;
  beginInfo.pInheritanceInfo = NULL;

#ifdef GLAD_VULKAN
  if (glad_vkBeginCommandBuffer(obj->commandBuffers[obj->currentFrame], &beginInfo) != VK_SUCCESS) {
    fprintf(stderr, "failed to begin recording command buffer\n");
    code = 0;
  }
#else
  if (vkBeginCommandBuffer(obj->commandBuffers[obj->currentFrame], &beginInfo) != VK_SUCCESS) {
    fprintf(stderr, "failed to begin recording command buffer\n");
    code = 0;
  }
#endif
  /**
   * Begin render pass
   */
  VkRenderPassBeginInfo renderPassInfo = {0};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = obj->renderPass;
  renderPassInfo.framebuffer = obj->swapChainFramebuffers[imageIndex];

  renderPassInfo.renderArea.offset.x = 0;
  renderPassInfo.renderArea.offset.y = 0;
  renderPassInfo.renderArea.extent = obj->swapChainExtent;

  //Clear value will be set to magenta
  VkClearValue clearValues [] = {
    (VkClearValue) /* clearColor */ {0.5f, 0.0f, 0.5f, 1.0f},
    (VkClearValue) /* depthClearValue */ {1.0f, 0.0f},
  };
  renderPassInfo.clearValueCount = (uint32_t) sizeof(clearValues) / sizeof(VkClearValue);
  renderPassInfo.pClearValues = clearValues;

#ifdef GLAD_VULKAN
  glad_vkCmdBeginRenderPass(obj->commandBuffers[obj->currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
#else
  vkCmdBeginRenderPass(obj->commandBuffers[obj->currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
#endif
  /**
   * Bind graphics pipeline
   */
#ifdef GLAD_VULKAN
  glad_vkCmdBindPipeline(obj->commandBuffers[obj->currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, obj->graphicsPipeline);
#else
  vkCmdBindPipeline(obj->commandBuffers[obj->currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, obj->graphicsPipeline);
#endif
  
  /**
   * Bind vertex buffer
   */

  VkBuffer vertexBuffers [] = {obj->vertexBuffer};
  VkDeviceSize offsets [] = {0};
#ifdef GLAD_VULKAN
  glad_vkCmdBindVertexBuffers(obj->commandBuffers[obj->currentFrame], 0, 1, vertexBuffers, offsets);
  glad_vkCmdBindIndexBuffer(obj->commandBuffers[obj->currentFrame], obj->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
#else
  vkCmdBindVertexBuffers(obj->commandBuffers[obj->currentFrame], 0, 1, vertexBuffers, offsets);
  vkCmdBindIndexBuffer(obj->commandBuffers[obj->currentFrame], obj->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
#endif
  
  /**
   * Viewport and scissor
   */
  VkViewport viewport = {0};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float) obj->swapChainExtent.width;
  viewport.height = (float) obj->swapChainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
#ifdef GLAD_VULKAN
  glad_vkCmdSetViewport(obj->commandBuffers[obj->currentFrame], 0, 1, &viewport);
#else
  vkCmdSetViewport(obj->commandBuffers[obj->currentFrame], 0, 1, &viewport);
#endif

  VkRect2D scissor = {0};
  scissor.offset.x = 0;
  scissor.offset.y = 0;
  scissor.extent = obj->swapChainExtent;
#ifdef GLAD_VULKAN
  glad_vkCmdSetScissor(obj->commandBuffers[obj->currentFrame], 0, 1, &scissor);
#else
  vkCmdSetScissor(obj->commandBuffers[obj->currentFrame], 0, 1, &scissor);
#endif
  /**
   * Draw
   */
#ifdef GLAD_VULKAN
  glad_vkCmdBindDescriptorSets(
          obj->commandBuffers[obj->currentFrame], 
          VK_PIPELINE_BIND_POINT_GRAPHICS, 
          obj->pipelineLayout, 
          0, 
          1, 
          &obj->descriptorSets[obj->currentFrame], 
          0, 
          NULL);
  glad_vkCmdDrawIndexed(obj->commandBuffers[obj->currentFrame], obj->indexCount, 1, 0, 0, 0);
#else
  // vkCmdDraw(obj->commandBuffers[obj->currentFrame], sizeof(vertices) / sizeof(Vertex), 1, 0, 0);
  vkCmdBindDescriptorSets(
          obj->commandBuffers[obj->currentFrame], 
          VK_PIPELINE_BIND_POINT_GRAPHICS, 
          obj->pipelineLayout, 
          0, 
          1, 
          &obj->descriptorSets[obj->currentFrame], 
          0, 
          NULL);
  vkCmdDrawIndexed(obj->commandBuffers[obj->currentFrame], sizeof(indices) / sizeof(uint16_t), 1, 0, 0, 0);
#endif
  /**
   * End render pass
   */
#ifdef GLAD_VULKAN
  glad_vkCmdEndRenderPass(obj->commandBuffers[obj->currentFrame]);
#else
  vkCmdEndRenderPass(obj->commandBuffers[obj->currentFrame]);
#endif
  /**
   * End recording command buffer
   */
#ifdef GLAD_VULKAN
  if (glad_vkEndCommandBuffer(obj->commandBuffers[obj->currentFrame]) != VK_SUCCESS) {
    fprintf(stderr, "failed to record command buffer\n");
    code = 0;
  }
#else
  if (vkEndCommandBuffer(obj->commandBuffers[obj->currentFrame]) != VK_SUCCESS) {
    fprintf(stderr, "failed to record command buffer\n");
    code = 0;
  }
#endif
  return code;
}

void updateUniformBuffer(pAsobj obj, uint32_t currentImage) {
  double time = glfwGetTime() - startTime;

  UniformBufferObject ubo = {0};
  glm_mat4_identity(ubo.model);
  glm_rotate(ubo.model, (float) time * glm_rad(90.0f), (vec3) {0.0f, 0.0f, 1.0f});
  glm_mat4_identity(ubo.view);
  glm_lookat((vec3) {2.0f, 2.0f, 2.0f}, (vec3) {0.0f, 0.0f, 0.0f}, (vec3) {0.0f, 0.0f, 1.0f}, ubo.view);
  glm_mat4_identity(ubo.proj);
  glm_perspective(glm_rad(45.0f), (float) obj->swapChainExtent.width / (float) obj->swapChainExtent.height, 0.1f, 10.0f, ubo.proj);
  ubo.proj[1][1] *= -1.0f;
  memcpy(obj->uniformBuffersMapped[obj->currentFrame], &ubo, sizeof(ubo));
}

int drawFrame(pAsobj obj) {
  int code = 1;
  uint32_t imageIndex;
#ifdef GLAD_VULKAN
  glad_vkWaitForFences(obj->device, 1, &obj->inFlightFences[obj->currentFrame], VK_TRUE, UINT64_MAX);
  VkResult result = glad_vkAcquireNextImageKHR(obj->device, 
                                               obj->swapChain, 
                                               UINT64_MAX, 
                                               obj->imageAvailableSemaphores[obj->currentFrame], 
                                               VK_NULL_HANDLE, 
                                               &imageIndex);
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain(obj);
    return 1;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    fprintf(stderr, "failed to acquire swap chain image\n");
    return 0;
  }
  glad_vkResetFences(obj->device, 1, &obj->inFlightFences[obj->currentFrame]);
  glad_vkResetCommandBuffer(obj->commandBuffers[obj->currentFrame], 0);
#else
  vkWaitForFences(obj->device, 1, &obj->inFlightFences[obj->currentFrame], VK_TRUE, UINT64_MAX);
  VkResult result = vkAcquireNextImageKHR(obj->device, 
                                          obj->swapChain, 
                                          UINT64_MAX, 
                                          obj->imageAvailableSemaphores[obj->currentFrame], 
                                          VK_NULL_HANDLE, 
                                          &imageIndex);
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain(obj);
    return 1;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    fprintf(stderr, "failed to acquire swap chain image\n");
    return 0;
  }
  vkResetFences(obj->device, 1, &obj->inFlightFences[obj->currentFrame]);
  vkResetCommandBuffer(obj->commandBuffers[obj->currentFrame], 0);
#endif
  recordCommandBuffer(obj, imageIndex);

  updateUniformBuffer(obj, imageIndex);

  VkSubmitInfo submitInfo = {0};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores [] = {obj->imageAvailableSemaphores[obj->currentFrame]};
  VkPipelineStageFlags waitStages [] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &obj->commandBuffers[obj->currentFrame];

  VkSemaphore signalSemaphores [] = {obj->renderFinishedSemaphores[obj->currentFrame]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

#ifdef GLAD_VULKAN
  if (glad_vkQueueSubmit(obj->graphicsQueue, 1, &submitInfo, obj->inFlightFences[obj->currentFrame]) != VK_SUCCESS) {
    fprintf(stderr, "failed to submit draw command buffer\n");
    code = 0;
  }
#else
  if (vkQueueSubmit(obj->graphicsQueue, 1, &submitInfo, obj->inFlightFences[obj->currentFrame]) != VK_SUCCESS) {
    fprintf(stderr, "failed to submit draw command buffer\n");
    code = 0;
  }
#endif

  VkSubpassDependency dependency = {0};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  
  VkPresentInfoKHR presentInfo = {0};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains [] = {obj->swapChain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;

  presentInfo.pResults = NULL;
  
#ifdef GLAD_VULKAN
  result = glad_vkQueuePresentKHR(obj->presentQueue, &presentInfo);
#else
  result = vkQueuePresentKHR(obj->presentQueue, &presentInfo);
#endif
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || obj->framebufferResized) {
    obj->framebufferResized = 0;
    recreateSwapChain(obj);
  } else if (result != VK_SUCCESS) {
    fprintf(stderr, "failed to present swap chain image\n");
    code = 0;
  }
  obj->currentFrame = (obj->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
  return code;
}

int createSyncObjects(pAsobj obj) {
  fprintf(stdout, "Creating sync objects\n");
  int code = 1;
  obj->imageAvailableSemaphores = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkSemaphore));
  obj->renderFinishedSemaphores = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkSemaphore));
  obj->inFlightFences = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkFence));
  VkSemaphoreCreateInfo semaphoreInfo = {0};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  VkFenceCreateInfo fenceInfo = {0};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT && code; i++) {  
#ifdef GLAD_VULKAN
    if (glad_vkCreateSemaphore(obj->device, &semaphoreInfo, NULL, &obj->imageAvailableSemaphores[i]) != VK_SUCCESS ||
        glad_vkCreateSemaphore(obj->device, &semaphoreInfo, NULL, &obj->renderFinishedSemaphores[i]) != VK_SUCCESS ||
        glad_vkCreateFence(obj->device, &fenceInfo, NULL, &obj->inFlightFences[i]) != VK_SUCCESS) {
      fprintf(stderr, "failed to create sync objects for a frame\n");
      code = 0;
    }
#else
    if (vkCreateSemaphore(obj->device, &semaphoreInfo, NULL, &obj->imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(obj->device, &semaphoreInfo, NULL, &obj->renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(obj->device, &fenceInfo, NULL, &obj->inFlightFences[i]) != VK_SUCCESS) {
      fprintf(stderr, "failed to create sync objects for a frame\n");
      code = 0;
    }
#endif
  }
  return code;
}

int createCommandBuffers(pAsobj obj) {
  fprintf(stdout, "Creating command buffer\n");
  int code = 1;
  obj->commandBuffers = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkCommandBuffer));
  VkCommandBufferAllocateInfo allocInfo = {0};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = obj->commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
#ifdef GLAD_VULKAN
  if (glad_vkAllocateCommandBuffers(obj->device, &allocInfo, obj->commandBuffers) != VK_SUCCESS) {
    fprintf(stderr, "failed to allocate command buffer\n");
    code = 0;
  }
#else
  if (vkAllocateCommandBuffers(obj->device, &allocInfo, obj->commandBuffers) != VK_SUCCESS) {
    fprintf(stderr, "failed to allocate command buffer\n");
    code = 0;
  }
#endif
  return code;
}

int createCommandPool(pAsobj obj) {
  fprintf(stdout, "Creating command pool\n");
  int code = 1;
  QueueFamilyIndices queueFamilyIndices = findQueueFamilies(obj->physicalDevice, obj);
  VkCommandPoolCreateInfo poolInfo = {0};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
#ifdef GLAD_VULKAN
  if (glad_vkCreateCommandPool(obj->device, &poolInfo, NULL, &obj->commandPool) != VK_SUCCESS) {
    fprintf(stderr, "failed to create command pool\n");
    code = 0;
  }
#else
  if (vkCreateCommandPool(obj->device, &poolInfo, NULL, &obj->commandPool) != VK_SUCCESS) {
    fprintf(stderr, "failed to create command pool\n");
    code = 0;
  }
#endif
  return code;
}

uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, pAsobj obj) {
  VkPhysicalDeviceMemoryProperties memProperties;
#ifdef GLAD_VULKAN
  glad_vkGetPhysicalDeviceMemoryProperties(obj->physicalDevice, &memProperties);
#else
  vkGetPhysicalDeviceMemoryProperties(obj->physicalDevice, &memProperties);
#endif
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }
  fprintf(stderr, "failed to find suitable memory type\n");
  return -1;
}

int createImage(pAsobj obj, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, 
                VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage * image, 
                VkDeviceMemory * imageMemory) {
  int code = 1;
  VkImageCreateInfo imageInfo = {0};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = usage;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.flags = 0;
#ifdef GLAD_VULKAN
  if (glad_vkCreateImage(obj->device, &imageInfo, NULL, image) != VK_SUCCESS) {
    fprintf(stderr, "failed to create texture image\n");
    return 0;
  }
#else
  if (vkCreateImage(obj->device, &imageInfo, NULL, image) != VK_SUCCESS) {
    fprintf(stderr, "failed to create texture image\n");
    return 0;
  }
#endif
  VkMemoryRequirements memRequirements;
#ifdef GLAD_VULKAN
  glad_vkGetImageMemoryRequirements(obj->device, *image, &memRequirements);
#else
  vkGetImageMemoryRequirements(obj->device, *image, &memRequirements);
#endif
  VkMemoryAllocateInfo allocInfo = {0};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, 
                                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                                              obj);
#ifdef GLAD_VULKAN
  if (glad_vkAllocateMemory(obj->device, &allocInfo, NULL, imageMemory) != VK_SUCCESS) {
    fprintf(stderr, "failed to allocate texture image memory\n");
    code = 0;
  }
  glad_vkBindImageMemory(obj->device, *image, *imageMemory, 0);
#else
  if (vkAllocateMemory(obj->device, &allocInfo, NULL, imageMemory) != VK_SUCCESS) {
    fprintf(stderr, "failed to allocate texture image memory\n");
    code = 0;
  }
  vkBindImageMemory(obj->device, *image, *imageMemory, 0);
#endif
  return code;
}

int createBuffer(VkDeviceSize size,
                 VkBufferUsageFlags usage,
                 VkMemoryPropertyFlags properties,
                 VkBuffer * buffer,
                 VkDeviceMemory * bufferMemory,
                 pAsobj obj) {
  
  int code = 1;
  VkBufferCreateInfo bufferInfo = {0};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

#ifdef GLAD_VULKAN
  if (glad_vkCreateBuffer(obj->device, &bufferInfo, NULL, buffer) != VK_SUCCESS) {
    fprintf(stderr, "failed to create buffer\n");
    code = 0;
  }
#else
  if (vkCreateBuffer(obj->device, &bufferInfo, NULL, buffer) != VK_SUCCESS) {
    fprintf(stderr, "failed to create buffer\n");
    code = 0;
  }
#endif
  VkMemoryRequirements memRequirements;
#ifdef GLAD_VULKAN
  glad_vkGetBufferMemoryRequirements(obj->device, *buffer, &memRequirements);
#else
  vkGetBufferMemoryRequirements(obj->device, *buffer, &memRequirements);
#endif
  
  VkMemoryAllocateInfo allocInfo = {0};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, obj);

#ifdef GLAD_VULKAN
  if (glad_vkAllocateMemory(obj->device, &allocInfo, NULL, bufferMemory) != VK_SUCCESS) {
    fprintf(stderr, "failed to allocate buffer memory\n");
    code = 0;
  }
  glad_vkBindBufferMemory(obj->device, *buffer, *bufferMemory, 0);
#else
  if (vkAllocateMemory(obj->device, &allocInfo, NULL, bufferMemory) != VK_SUCCESS) {
    fprintf(stderr, "failed to allocate buffer memory\n");
    code = 0;
  }
  vkBindBufferMemory(obj->device, *buffer, *bufferMemory, 0);
#endif
  
  return code;
}

int createUniformBuffers(pAsobj obj) {
  int code = 1;
  VkDeviceSize bufferSize = sizeof(UniformBufferObject);

  obj->uniformBuffers = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkBuffer));
  obj->uniformBuffersMemory = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkDeviceMemory));
  obj->uniformBuffersMapped = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(void *));

#ifdef GLAD_VULKAN
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT && code; i++) {
    code = createBuffer(bufferSize,
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        &obj->uniformBuffers[i],
                        &obj->uniformBuffersMemory[i],
                        obj);
    if (code) {
      VkResult result = glad_vkMapMemory(obj->device, obj->uniformBuffersMemory[i], 0, bufferSize, 0, &obj->uniformBuffersMapped[i]);
      if (result != VK_SUCCESS) {
        fprintf(stderr, "failed to map memory\n");
        code = 0;
      }
    }
  }
#else
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT && code; i++) {
    code = createBuffer(bufferSize,
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        &obj->uniformBuffers[i],
                        &obj->uniformBuffersMemory[i],
                        obj);
    if (code) {
      vkMapMemory(obj->device, obj->uniformBuffersMemory[i], 0, bufferSize, 0, &obj->uniformBuffersMapped[i]);
    }
  }
#endif

  return code;
}

int createDescriptorPool(pAsobj obj) {
  int code = 1;
  
  VkDescriptorPoolSize poolSize = {0};
  poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSize.descriptorCount = (uint32_t) MAX_FRAMES_IN_FLIGHT;

  VkDescriptorPoolSize poolSize2 = {0};
  poolSize2.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSize2.descriptorCount = (uint32_t) MAX_FRAMES_IN_FLIGHT;
  VkDescriptorPoolSize poolSizes [] = {poolSize, poolSize2};

  VkDescriptorPoolCreateInfo poolInfo = {0};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = (uint32_t) sizeof(poolSizes) / sizeof(VkDescriptorPoolSize);
  poolInfo.pPoolSizes = poolSizes;

  poolInfo.maxSets = (uint32_t) MAX_FRAMES_IN_FLIGHT;

#ifdef GLAD_VULKAN
  if (glad_vkCreateDescriptorPool(obj->device, &poolInfo, NULL, &obj->descriptorPool) != VK_SUCCESS) {
    fprintf(stderr, "failed to create descriptor pool\n");
    code = 0;
  }
#else
  if (vkCreateDescriptorPool(obj->device, &poolInfo, NULL, &obj->descriptorPool) != VK_SUCCESS) {
    fprintf(stderr, "failed to create descriptor pool\n");
    code = 0;
  }
#endif

  return code;
}

int createDescriptorSets(pAsobj obj) {
  int code = 1;

  VkDescriptorSetLayout * layouts = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkDescriptorSetLayout));
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT && code; i++) {
    layouts[i] = obj->descriptorSetLayout;
  }
  VkDescriptorSetAllocateInfo allocInfo = {0};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = obj->descriptorPool;
  allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
  allocInfo.pSetLayouts = layouts;

  obj->descriptorSets = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkDescriptorSet));
#ifdef GLAD_VULKAN
  if (glad_vkAllocateDescriptorSets(obj->device, &allocInfo, obj->descriptorSets) != VK_SUCCESS) {
    fprintf(stderr, "failed to allocate descriptor sets\n");
    code = 0;
  }
#else
  if (vkAllocateDescriptorSets(obj->device, &allocInfo, obj->descriptorSets) != VK_SUCCESS) {
    fprintf(stderr, "failed to allocate descriptor sets\n");
    code = 0;
  }
#endif

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT && code; i++) {
    VkDescriptorBufferInfo bufferInfo = {0};
    bufferInfo.buffer = obj->uniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkDescriptorImageInfo imageInfo = {0};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = obj->textureImageView;
    imageInfo.sampler = obj->textureSampler;

    VkWriteDescriptorSet descriptorWrites [2] = {0};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = obj->descriptorSets[i];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = obj->descriptorSets[i];
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pBufferInfo = NULL;
    descriptorWrites[1].pImageInfo = &imageInfo;

#ifdef GLAD_VULKAN
    glad_vkUpdateDescriptorSets(obj->device, 2, descriptorWrites, 0, NULL);
#else
    vkUpdateDescriptorSets(obj->device, 2, descriptorWrites, 0, NULL);
#endif
  }
  free(layouts);

  return code;
}

VkCommandBuffer beginSingleTimeCommands(pAsobj obj) {
  VkCommandBufferAllocateInfo allocInfo = {0};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = obj->commandPool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer = {0};
#ifdef GLAD_VULKAN
  glad_vkAllocateCommandBuffers(obj->device, &allocInfo, &commandBuffer);
#else
  vkAllocateCommandBuffers(obj->device, &allocInfo, &commandBuffer);
#endif

  VkCommandBufferBeginInfo beginInfo = {0};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
#ifdef GLAD_VULKAN
  glad_vkBeginCommandBuffer(commandBuffer, &beginInfo);
#else
  vkBeginCommandBuffer(commandBuffer, &beginInfo);
#endif
  return commandBuffer;
}

void  endSingleTimeCommands(pAsobj obj, VkCommandBuffer commandBuffer) {
#ifdef GLAD_VULKAN
  glad_vkEndCommandBuffer(commandBuffer);
#else
  vkEndCommandBuffer(commandBuffer);
#endif

  VkSubmitInfo submitInfo = {0};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

#ifdef GLAD_VULKAN
  glad_vkQueueSubmit(obj->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  glad_vkQueueWaitIdle(obj->graphicsQueue);
  glad_vkFreeCommandBuffers(obj->device, obj->commandPool, 1, &commandBuffer);
#else
  vkQueueSubmit(obj->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(obj->graphicsQueue);
  vkFreeCommandBuffers(obj->device, obj->commandPool, 1, &commandBuffer);
#endif
}

int copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, pAsobj obj) {
  int code = 1;

  VkCommandBuffer commandBuffer = beginSingleTimeCommands(obj);

  VkBufferCopy copyRegion = {0};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = 0;
  copyRegion.size = size;

#ifdef GLAD_VULKAN
  glad_vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
#else
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
#endif

  endSingleTimeCommands(obj, commandBuffer);

  return code;
}

int hasStencilComponent(VkFormat format) {
  return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

int transitionImageLayout(pAsobj obj, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
  int code = 1;
  VkCommandBuffer commandBuffer = beginSingleTimeCommands(obj);

  VkImageMemoryBarrier barrier = {0};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = 0;

  if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (hasStencilComponent(format)) {
      barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
  } else {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  }

  VkPipelineStageFlags sourceStage = 0;
  VkPipelineStageFlags destinationStage = 0;
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
  } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  } else {
    fprintf(stderr, "unsupported layout transition!\n");
    return 0;
  }

#ifdef GLAD_VULKAN
  glad_vkCmdPipelineBarrier(
      commandBuffer,
      sourceStage, destinationStage,
      0,
      0, NULL,
      0, NULL,
      1, &barrier);
#else
  vkCmdPipelineBarrier(
      commandBuffer,
      sourceStage, destinationStage,
      0,
      0, NULL,
      0, NULL,
      1, &barrier);
#endif
  
  endSingleTimeCommands(obj, commandBuffer);
  return code;
}

void copyBufferToImage(pAsobj obj, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands(obj);

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

#ifdef GLAD_VULKAN
  glad_vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
#else
  vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
#endif

  endSingleTimeCommands(obj, commandBuffer);
}

int createTextureImage(pAsobj obj) {
  int code = 1;
  int texWidth, texHeight, texChannels;
  stbi_uc * pixels = stbi_load(TEXTURE_PATH, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
  VkDeviceSize imagesize = texWidth * texWidth * 4;
  if (!pixels) {
    fprintf(stderr, "failed to load texture image\n");
    return 0;
  }
  VkBuffer stagingBuffer = {0};
  VkDeviceMemory stagingBufferMemory = {0};
  code = createBuffer(imagesize,
              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
              &stagingBuffer,
              &stagingBufferMemory,
              obj);
  if (code) {
    void * data;
#ifdef GLAD_VULKAN
    glad_vkMapMemory(obj->device, stagingBufferMemory, 0, imagesize, 0, &data);
    memcpy(data, pixels, (size_t) imagesize);
    glad_vkUnmapMemory(obj->device, stagingBufferMemory);
#else
    vkMapMemory(obj->device, stagingBufferMemory, 0, imagesize, 0, &data);
    memcpy(data, pixels, (size_t) imagesize);
    vkUnmapMemory(obj->device, stagingBufferMemory);
#endif
    stbi_image_free(pixels);

    code = createImage(obj,
                texWidth,
                texHeight,
                VK_FORMAT_R8G8B8A8_SRGB,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                &obj->textureImage,
                &obj->textureImageMemory);
  }
  if (code) {
    code = transitionImageLayout(
        obj,
        obj->textureImage,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );
  }
  if (code) {
    copyBufferToImage(
        obj,
        stagingBuffer,
        obj->textureImage,
        (uint32_t) texWidth,
        (uint32_t) texHeight
    );
    code = transitionImageLayout(
        obj,
        obj->textureImage,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
  }
#ifdef GLAD_VULKAN
  vkDestroyBuffer(obj->device, stagingBuffer, NULL);
  vkFreeMemory(obj->device, stagingBufferMemory, NULL);
#else
  vkDestroyBuffer(obj->device, stagingBuffer, NULL);
  vkFreeMemory(obj->device, stagingBufferMemory, NULL);
#endif

  return code;
}

int createIndexBuffer(pAsobj obj) {
  int code = 1;
  VkDeviceSize bufferSize = obj->indexCount * sizeof(uint32_t);
  VkBuffer stagingBuffer = {0};
  VkDeviceMemory stagingBufferMemory = {0};
  code = createBuffer(bufferSize,
                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      &stagingBuffer,
                      &stagingBufferMemory,
                      obj);
  if (code) {
    void * data;
#ifdef GLAD_VULKAN
    glad_vkMapMemory(obj->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, obj->indices, (size_t) bufferSize);
    glad_vkUnmapMemory(obj->device, stagingBufferMemory);
#else
    vkMapMemory(obj->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices, (size_t) bufferSize);
    vkUnmapMemory(obj->device, stagingBufferMemory);
#endif
    code = createBuffer(bufferSize,
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        &obj->indexBuffer,
                        &obj->indexBufferMemory,
                        obj);
  }
  if (code) {
    code = copyBuffer(stagingBuffer, obj->indexBuffer, bufferSize, obj);
#ifdef GLAD_VULKAN
    glad_vkDestroyBuffer(obj->device, stagingBuffer, NULL);
    glad_vkFreeMemory(obj->device, stagingBufferMemory, NULL);
#else
    vkDestroyBuffer(obj->device, stagingBuffer, NULL);
    vkFreeMemory(obj->device, stagingBufferMemory, NULL);
#endif
  }
  return code;
}

int createVertexBuffer(pAsobj obj) {
  int code = 1;
  VkDeviceSize bufferSize = obj->vertexCount * sizeof(Vertex);

  VkBuffer stagingBuffer = {0};
  VkDeviceMemory stagingBufferMemory = {0};

  code = createBuffer(bufferSize,
                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      &stagingBuffer,
                      &stagingBufferMemory,
                      obj);
                      
  if (code) {
    void * data;
#ifdef GLAD_VULKAN
    glad_vkMapMemory(obj->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, obj->vertices, (size_t) bufferSize);
    glad_vkUnmapMemory(obj->device, stagingBufferMemory);
#else
    vkMapMemory(obj->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices, (size_t) bufferSize);
    vkUnmapMemory(obj->device, stagingBufferMemory);
#endif
    code = createBuffer(bufferSize,
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        &obj->vertexBuffer,
                        &obj->vertexBufferMemory,
                        obj);
  }
  if (code) {
    code = copyBuffer(stagingBuffer, obj->vertexBuffer, bufferSize, obj);
#ifdef GLAD_VULKAN
    glad_vkDestroyBuffer(obj->device, stagingBuffer, NULL);
    glad_vkFreeMemory(obj->device, stagingBufferMemory, NULL);
#else
    vkDestroyBuffer(obj->device, stagingBuffer, NULL);
    vkFreeMemory(obj->device, stagingBufferMemory, NULL);
#endif
  }
  return code;
}

int createFramebuffers(pAsobj obj) {
  // fprintf(stdout, "Creating framebuffers\n");
  int code = 1;
  obj->swapChainFramebuffers = malloc(obj->swapChainImageCount * sizeof(VkFramebuffer));
  for (int i = 0; i < obj->swapChainImageCount; i++) {
    VkImageView attachments [] = {obj->swapChainImageViews[i], obj->depthImageView};
    VkFramebufferCreateInfo framebufferInfo = {0};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = obj->renderPass;
    framebufferInfo.attachmentCount = (uint32_t) sizeof(attachments) / sizeof(VkImageView);
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = obj->swapChainExtent.width;
    framebufferInfo.height = obj->swapChainExtent.height;
    framebufferInfo.layers = 1;
#ifdef GLAD_VULKAN
    if (glad_vkCreateFramebuffer(obj->device, &framebufferInfo, NULL, &obj->swapChainFramebuffers[i]) != VK_SUCCESS) {
      fprintf(stderr, "failed to create framebuffer\n");
      code = 0;
      break;
    }
#else
    if (vkCreateFramebuffer(obj->device, &framebufferInfo, NULL, &obj->swapChainFramebuffers[i]) != VK_SUCCESS) {
      fprintf(stderr, "failed to create framebuffer\n");
      code = 0;
      break;
    }
#endif
  }
  return code;
}

int createRenderPass(pAsobj obj) {
  fprintf(stdout, "Creating render pass\n");
  int code = 1;
  VkAttachmentDescription colorAttachment = {0};
  colorAttachment.format = obj->swapChainImageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef = {0};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription depthAttachment = {0};
  depthAttachment.format = findDepthFormat(obj);
  depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthAttachmentRef = {0};
  depthAttachmentRef.attachment = 1;
  depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {0};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;
  subpass.pDepthStencilAttachment = &depthAttachmentRef;

  VkSubpassDependency dependency = {0};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  VkAttachmentDescription attachments [] = {colorAttachment, depthAttachment};
  VkRenderPassCreateInfo renderPassInfo = {0};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 2;
  renderPassInfo.pAttachments = attachments;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;
  
#ifdef GLAD_VULKAN
  if (glad_vkCreateRenderPass(obj->device, &renderPassInfo, NULL, &obj->renderPass) != VK_SUCCESS) {
    fprintf(stderr, "failed to create render pass\n");
    code = 0;
  }
#else
  if (vkCreateRenderPass(obj->device, &renderPassInfo, NULL, &obj->renderPass) != VK_SUCCESS) {
    fprintf(stderr, "failed to create render pass\n");
    code = 0;
  }
#endif
  return code;
}

int readFile(const char * filename, ShaderSource * source) {
  int code = 1;
  FILE * file = fopen(filename, "rb");
  if (!file) {
    fprintf(stderr, "failed to open file\n");
    code = 0;
  }
  if (code) {
    fseek(file, 0, SEEK_END);
    source->codeSize = (ftell(file) + 3) / 4;
    rewind(file);
    if (!(source->data = malloc(source->codeSize * sizeof(uint32_t)))) {
      fprintf(stderr, "failed to allocate memory\n");
      code = 0;
    }
  }
  if (code) {
    fread(source->data, sizeof(uint32_t), source->codeSize, file);
#ifdef DEBUG
    fprintf(stdout, "read %d bytes from %s\n", source->codeSize * sizeof(uint32_t), filename);
#endif
    fclose(file);
  }
  return code;
}

int createShaderModule(pAsobj obj, ShaderSource * source, VkShaderModule * shaderModule) {
  int code = 1;
  VkShaderModuleCreateInfo createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = source->codeSize * sizeof(uint32_t);
  createInfo.pCode = source->data;
#ifdef GLAD_VULKAN
  if (glad_vkCreateShaderModule(obj->device, &createInfo, NULL, shaderModule) != VK_SUCCESS) {
    fprintf(stderr, "failed to create shader module\n");
    code = 0;
  }
#else
  if (vkCreateShaderModule(obj->device, &createInfo, NULL, shaderModule) != VK_SUCCESS) {
    fprintf(stderr, "failed to create shader module\n");
    code = 0;
  }
#endif
  return code;
}

int createDescriptorSetLayout(pAsobj obj) {
  int code = 1;

  VkDescriptorSetLayoutBinding uboLayoutBinding = {0};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  uboLayoutBinding.pImmutableSamplers = NULL;

  VkDescriptorSetLayoutBinding samplerLayoutBinding = {0};
  samplerLayoutBinding.binding = 1;
  samplerLayoutBinding.descriptorCount = 1;
  samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.pImmutableSamplers = NULL;
  samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutBinding bindings [] = {uboLayoutBinding, samplerLayoutBinding};

  VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = (uint32_t) sizeof(bindings) / sizeof(VkDescriptorSetLayoutBinding);
  layoutInfo.pBindings = bindings;
  
#ifdef GLAD_VULKAN
  if (glad_vkCreateDescriptorSetLayout(obj->device, &layoutInfo, NULL, &obj->descriptorSetLayout) != VK_SUCCESS) {
    fprintf(stderr, "failed to create descriptor set layout\n");
    code = 0;
  }
#else
  if (vkCreateDescriptorSetLayout(obj->device, &layoutInfo, NULL, &obj->descriptorSetLayout) != VK_SUCCESS) {
    fprintf(stderr, "failed to create descriptor set layout\n");
    code = 0;
  }
#endif

  return code;
}

int createGraphicsPipeline(pAsobj obj) {
  fprintf(stdout, "Creating graphics pipeline\n");
  int code = 1;
  ShaderSource vertSource, fragSource = {0};
  VkShaderModule vertShaderModule, fragShaderModule = {0};
  code = readFile("srctut/shaders/vertex.spv", &vertSource);
  code = readFile("srctut/shaders/fragment.spv", &fragSource);
  if (code) {
    code = createShaderModule(obj, &vertSource, &vertShaderModule);
    code = createShaderModule(obj, &fragSource, &fragShaderModule);
    /**
     * Shaders
     */
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {0};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {0};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages [] = {vertShaderStageInfo, fragShaderStageInfo};
    /**
     * Vertex input and assembly
     */
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};

    uint32_t attributeCount = 0;
    VkVertexInputAttributeDescription * attributeDescriptions = getAttributeDescriptions(&attributeCount);
    VkVertexInputBindingDescription bindingDescription = getBindingDescription();

    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = attributeCount;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {0};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    /**
     * Viewport
     */
    VkDynamicState dynamicStates [] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState = {0};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = sizeof(dynamicStates) / sizeof(VkDynamicState);
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineViewportStateCreateInfo viewportState = {0};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    viewportState.pScissors = NULL;
    viewportState.pViewports = NULL;

    /**
     * Rasterizer
     */
    VkPipelineRasterizationStateCreateInfo rasterizer = {0};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    /**
     * Multisampling
     */
    VkPipelineMultisampleStateCreateInfo multisampling = {0};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0;
    multisampling.pSampleMask = NULL;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;
    multisampling.flags = 0;
    multisampling.pNext = NULL;

    /**
     * Depth and Stencil Testing
     */
    VkPipelineDepthStencilStateCreateInfo depthStencil = {0};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = (VkStencilOpState) {0};
    depthStencil.back = (VkStencilOpState) {0};

    /**
     * Color Blending
     */
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | 
                                          VK_COLOR_COMPONENT_G_BIT | 
                                          VK_COLOR_COMPONENT_B_BIT | 
                                          VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending = {0};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;
    colorBlending.flags = 0;
    colorBlending.pNext = NULL;

    /**
     * Pipeline Layout
     */
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &obj->descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = NULL;
    pipelineLayoutInfo.flags = 0;
    pipelineLayoutInfo.pNext = NULL;

#ifdef GLAD_VULKAN
    if (glad_vkCreatePipelineLayout(obj->device, &pipelineLayoutInfo, NULL, &obj->pipelineLayout) != VK_SUCCESS) {
      fprintf(stderr, "failed to create pipeline layout\n");
      code = 0;
    }
#else
    if (vkCreatePipelineLayout(obj->device, &pipelineLayoutInfo, NULL, &obj->pipelineLayout) != VK_SUCCESS) {
      fprintf(stderr, "failed to create pipeline layout\n");
      code = 0;
    }
#endif
    else {
      VkGraphicsPipelineCreateInfo pipelineInfo = {0};
      pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
      pipelineInfo.stageCount = sizeof(shaderStages) / sizeof(VkPipelineShaderStageCreateInfo);
      pipelineInfo.pStages = shaderStages;
      pipelineInfo.pVertexInputState = &vertexInputInfo;
      pipelineInfo.pInputAssemblyState = &inputAssembly;
      pipelineInfo.pViewportState = &viewportState;
      pipelineInfo.pRasterizationState = &rasterizer;
      pipelineInfo.pMultisampleState = &multisampling;
      pipelineInfo.pDepthStencilState = &depthStencil;
      pipelineInfo.pColorBlendState = &colorBlending;
      pipelineInfo.pDynamicState = &dynamicState;
      pipelineInfo.layout = obj->pipelineLayout;
      pipelineInfo.renderPass = obj->renderPass;
      pipelineInfo.subpass = 0;
      pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
      pipelineInfo.basePipelineIndex = -1;
#ifdef GLAD_VULKAN
      if (glad_vkCreateGraphicsPipelines(obj->device, 
                                         VK_NULL_HANDLE, 
                                         1, 
                                         &pipelineInfo, 
                                         NULL, 
                                         &obj->graphicsPipeline) != VK_SUCCESS) {
        fprintf(stderr, "failed to create graphics pipeline\n");
        code = 0;
      }
#else
      if (vkCreateGraphicsPipelines(obj->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &obj->graphicsPipeline) != VK_SUCCESS) {
        fprintf(stderr, "failed to create graphics pipeline\n");
        code = 0;
      }
#endif
    }
  }
  if (vertShaderModule) {
#ifdef GLAD_VULKAN
    glad_vkDestroyShaderModule(obj->device, vertShaderModule, NULL);
#else
    vkDestroyShaderModule(obj->device, vertShaderModule, NULL);
#endif
  }
  if (fragShaderModule) {
#ifdef GLAD_VULKAN
    glad_vkDestroyShaderModule(obj->device, fragShaderModule, NULL);
#else
    vkDestroyShaderModule(obj->device, fragShaderModule, NULL);
#endif
  }
  return code;
}

VkImageView createImageView(pAsobj obj, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
  VkImageViewCreateInfo viewInfo = {0};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = format;
  viewInfo.subresourceRange.aspectMask = aspectFlags;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  VkImageView imageView = {0};

#ifdef GLAD_VULKAN
  if (glad_vkCreateImageView(obj->device, &viewInfo, NULL, &imageView) != VK_SUCCESS) {
    fprintf(stderr, "failed to create texture image view\n");
    return 0;
  }
#else
  if (vkCreateImageView(obj->device, &viewInfo, NULL, &imageView) != VK_SUCCESS) {
    fprintf(stderr, "failed to create image view\n");
    return 0;
  }
#endif
  return imageView;
}

int createImageViews(pAsobj obj) {
  // fprintf(stdout, "Creating image views\n");
  int code = 1;
  obj->swapChainImageViews = malloc(obj->swapChainImageCount * sizeof(VkImageView));
  for (int i = 0; i < obj->swapChainImageCount; i++) {
    obj->swapChainImageViews[i] = createImageView(
        obj,
        obj->swapChainImages[i],
        obj->swapChainImageFormat,
        VK_IMAGE_ASPECT_COLOR_BIT
    );
    if (!obj->swapChainImageViews[i]) {
      fprintf(stderr, "failed to create image views\n");
      return 0;
    }
  }
  return code;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(VkSurfaceFormatKHR * formats, uint32_t formatCount) {
  VkSurfaceFormatKHR chosenFormat = {0};
  for (int i = 0; i < formatCount; i++) {
    if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      chosenFormat = formats[i];
      break;
    }
  }
  if (!chosenFormat.format) {
    chosenFormat = formats[0];
  }
  return chosenFormat;
}

VkPresentModeKHR chooseSwapPresentMode(VkPresentModeKHR * presentModes, uint32_t presentModeCount) {
  VkPresentModeKHR chosenMode = VK_PRESENT_MODE_FIFO_KHR;
  for (int i = 0; i < presentModeCount; i++) {
    if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      chosenMode = presentModes[i];
      break;
    }
  }
  return chosenMode;
}

VkExtent2D chooseSwapExtent(VkSurfaceCapabilitiesKHR capabilities, pAsobj obj) {
  VkExtent2D extent = {0};
  if (capabilities.currentExtent.width != UINT32_MAX) {
    extent = capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(obj->window, &width, &height);
    extent.width = width;
    extent.height = height;
    extent.width = (extent.width < capabilities.minImageExtent.width) ? capabilities.minImageExtent.width : extent.width;
    extent.width = (extent.width > capabilities.maxImageExtent.width) ? capabilities.maxImageExtent.width : extent.width;
    extent.height = (extent.height < capabilities.minImageExtent.height) ? capabilities.minImageExtent.height : extent.height;
    extent.height = (extent.height > capabilities.maxImageExtent.height) ? capabilities.maxImageExtent.height : extent.height;
  }
  return extent;
}

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, pAsobj obj) {
  // fprintf(stdout, "Querying swap chain support\n");
  SwapChainSupportDetails details = {0};
#ifdef GLAD_VULKAN
  glad_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, obj->surface, &details.capabilities);
  glad_vkGetPhysicalDeviceSurfaceFormatsKHR(device, obj->surface, &details.formatCount, NULL);
  if (details.formatCount) {
    details.formats = malloc(details.formatCount * sizeof(VkSurfaceFormatKHR));
    glad_vkGetPhysicalDeviceSurfaceFormatsKHR(device, obj->surface, &details.formatCount, details.formats);
  }
  glad_vkGetPhysicalDeviceSurfacePresentModesKHR(device, obj->surface, &details.presentModeCount, NULL);
  if (details.presentModeCount) {
    details.presentModes = malloc(details.presentModeCount * sizeof(VkPresentModeKHR));
    glad_vkGetPhysicalDeviceSurfacePresentModesKHR(device, obj->surface, &details.presentModeCount, details.presentModes);
  }
#else
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, obj->surface, &details.capabilities);
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, obj->surface, &details.formatCount, NULL);
  if (details.formatCount) {
    details.formats = malloc(details.formatCount * sizeof(VkSurfaceFormatKHR));
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, obj->surface, &details.formatCount, details.formats);
  }
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, obj->surface, &details.presentModeCount, NULL);
  if (details.presentModeCount) {
    details.presentModes = malloc(details.presentModeCount * sizeof(VkPresentModeKHR));
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, obj->surface, &details.presentModeCount, details.presentModes);
  }
#endif
  return details;
}

int createSurface(pAsobj obj) {
  int code = 1;
  if (glfwCreateWindowSurface(obj->instance, obj->window, NULL, &obj->surface) != VK_SUCCESS) {
    fprintf(stderr, "failed to create window surface\n");
    code = 0;
  }
  return code;
}

int createSwapChain(pAsobj obj) {
  // fprintf(stdout, "Creating swap chain\n");
  int code = 1;
  SwapChainSupportDetails swapChainSupport = querySwapChainSupport(obj->physicalDevice, obj);
  VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats, swapChainSupport.formatCount);
  VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes, swapChainSupport.presentModeCount);
  VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, obj);

  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = obj->surface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilyIndices indices = findQueueFamilies(obj->physicalDevice, obj);
  uint32_t queueFamilyIndices [] = {indices.graphicsFamily, indices.presentFamily};
  if (indices.graphicsFamily != indices.presentFamily) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = sizeof(queueFamilyIndices) / sizeof(uint32_t);
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
#ifdef GLAD_VULKAN
  if (glad_vkCreateSwapchainKHR(obj->device, &createInfo, NULL, &obj->swapChain) != VK_SUCCESS) {
    fprintf(stderr, "failed to create swap chain\n");
    code = 0;
  }
  glad_vkGetSwapchainImagesKHR(obj->device, obj->swapChain, &obj->swapChainImageCount, NULL);
  obj->swapChainImages = malloc(obj->swapChainImageCount * sizeof(VkImage));
  glad_vkGetSwapchainImagesKHR(obj->device, obj->swapChain, &obj->swapChainImageCount, obj->swapChainImages);
#else
  if (vkCreateSwapchainKHR(obj->device, &createInfo, NULL, &obj->swapChain) != VK_SUCCESS) {
    fprintf(stderr, "failed to create swap chain\n");
    code = 0;
  }
  vkGetSwapchainImagesKHR(obj->device, obj->swapChain, &obj->swapChainImageCount, NULL);
  obj->swapChainImages = malloc(obj->swapChainImageCount * sizeof(VkImage));
  vkGetSwapchainImagesKHR(obj->device, obj->swapChain, &obj->swapChainImageCount, obj->swapChainImages);
#endif
  obj->swapChainImageFormat = surfaceFormat.format;
  obj->swapChainExtent = extent;
  return code;
}

int createLogicalDevice(pAsobj obj) {
  fprintf(stdout, "Creating logical device\n");
  int code = 1;
  QueueFamilyIndices indices = findQueueFamilies(obj->physicalDevice, obj);
  
  uint32_t queueFamilies [] = {indices.graphicsFamily, indices.presentFamily};
  uint32_t queueFamilyCount = 0;
  uint32_t uniqueQueueFamilies [sizeof(queueFamilies) / sizeof(uint32_t)];
  for (int i = 0; i < sizeof(queueFamilies) / sizeof(uint32_t); i++) {
    int isUnique = 1;
    for (int j = 0; j < queueFamilyCount; j++) {
      if (queueFamilies[i] == uniqueQueueFamilies[j]) {
        isUnique = 0;
        break;
      }
    }
    if (isUnique) {
      uniqueQueueFamilies[queueFamilyCount++] = queueFamilies[i];
    }
  }

  VkDeviceQueueCreateInfo queueCreateInfos[queueFamilyCount];
  memset(queueCreateInfos, 0, sizeof(queueCreateInfos));
  
  float queuePriority = 1.0f;
  for (int i = 0; i < queueFamilyCount; i++) {
    queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfos[i].queueFamilyIndex = uniqueQueueFamilies[i];
    queueCreateInfos[i].queueCount = 1;
    queueCreateInfos[i].pQueuePriorities = &queuePriority;
  }

  VkPhysicalDeviceFeatures deviceFeatures = {0};

  VkDeviceCreateInfo createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pQueueCreateInfos = queueCreateInfos;
  createInfo.queueCreateInfoCount = queueFamilyCount;
  createInfo.pEnabledFeatures = &deviceFeatures;
  createInfo.enabledLayerCount = sizeof(validationLayers) / sizeof(char *);
  createInfo.ppEnabledLayerNames = validationLayers;
  createInfo.enabledExtensionCount = sizeof(deviceExtensions) / sizeof(char *);
  createInfo.ppEnabledExtensionNames = deviceExtensions;

  if (vkCreateDevice(obj->physicalDevice, &createInfo, NULL, &obj->device) != VK_SUCCESS) {
    fprintf(stderr, "failed to create logical device\n");
    code = 0;
  }
#ifdef GLAD_VULKAN
  glad_vkGetDeviceQueue(obj->device, indices.graphicsFamily, 0, &obj->graphicsQueue);
  glad_vkGetDeviceQueue(obj->device, indices.presentFamily, 0, &obj->presentQueue);
#else
  vkGetDeviceQueue(obj->device, indices.graphicsFamily, 0, &obj->graphicsQueue);
  vkGetDeviceQueue(obj->device, indices.presentFamily, 0, &obj->presentQueue);
#endif

  return code;
}

int extensionsSupported(VkPhysicalDevice device) {
  int isSupported = 1;
  uint32_t extensionCount;
#ifdef GLAD_VULKAN
  glad_vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);
  VkExtensionProperties availableExtensions [extensionCount];
  glad_vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, availableExtensions);
#else
  vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);
  VkExtensionProperties availableExtensions [extensionCount];
  vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, availableExtensions);
#endif
  for (int i = 0; i < sizeof(deviceExtensions) / sizeof(char *); i++) {
    int extensionFound = 0;
    for (int j = 0; j < extensionCount; j++) {
      if (strcmp(deviceExtensions[i], availableExtensions[j].extensionName) == 0) {
        extensionFound = 1;
        break;
      }
    }
    if (!extensionFound) {
      isSupported = 0;
      break;
    }
  }
  return isSupported;
}

int isDeviceSuitable(VkPhysicalDevice device, pAsobj obj) {
  fprintf(stdout, "Checking device suitability\n");
  int isSuitable = 0;
  VkPhysicalDeviceProperties deviceProperties = {0};
  VkPhysicalDeviceFeatures deviceFeatures = {0};
#ifdef GLAD_VULKAN
  glad_vkGetPhysicalDeviceProperties(device, &deviceProperties);
  glad_vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
#else
  vkGetPhysicalDeviceProperties(device, &deviceProperties);
  vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
#endif
  if (deviceFeatures.geometryShader && 
      (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
      deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)) {
    isSuitable = 1;
  }
  QueueFamilyIndices indices = findQueueFamilies(device, obj);
  if (!indices.hasGraphicsFamily || !indices.hasPresentFamily) {
    isSuitable = 0;
  }
  if (isSuitable) {
    isSuitable = extensionsSupported(device);
  }
  if (isSuitable) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, obj);
    isSuitable = swapChainSupport.formatCount && swapChainSupport.presentModeCount;
    free(swapChainSupport.formats);
    free(swapChainSupport.presentModes);
  }
  return isSuitable;
}

int selectPhysicalDevice(pAsobj obj) {
  fprintf(stdout, "Selecting physical device\n");
  int code = 1;
  uint32_t deviceCount = 0;
#ifdef GLAD_VULKAN
  glad_vkEnumeratePhysicalDevices(obj->instance, &deviceCount, NULL);
  VkPhysicalDevice devices [deviceCount];
  glad_vkEnumeratePhysicalDevices(obj->instance, &deviceCount, devices);
#else
  vkEnumeratePhysicalDevices(obj->instance, &deviceCount, NULL);
  VkPhysicalDevice devices [deviceCount];
  vkEnumeratePhysicalDevices(obj->instance, &deviceCount, devices);
#endif

  if (deviceCount == 0) {
    fprintf(stderr, "failed to find GPUs with Vulkan support\n");
    code = 0;
  } else {
    fprintf(stdout, "found %d GPUs\n", deviceCount);
  }

  for (int i = 0; i < deviceCount; i++) {
    if (isDeviceSuitable(devices[i], obj)) {
      obj->physicalDevice = devices[i];
      break;
    }
  }

  if (obj->physicalDevice == VK_NULL_HANDLE) {
    fprintf(stderr, "failed to find a suitable GPU\n");
    code = 0;
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
  fprintf(stdout, "version:         %d\n", version);
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

#ifdef DEBUG
  if (!checkValidationLayerSupport(obj)) {
    fprintf(stderr, "Validation layers requested, but not available\n");
    code = 0;
  }
  createInfo.enabledLayerCount = sizeof(validationLayers) / sizeof(char *);
  createInfo.ppEnabledLayerNames = validationLayers;
#endif
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

int createTextureImageView(pAsobj obj) {
  int code = 1;
  fprintf(stdout, "Creating texture image view\n");
  obj->textureImageView = createImageView(
    obj,
    obj->textureImage,
    VK_FORMAT_R8G8B8A8_SRGB,
    VK_IMAGE_ASPECT_COLOR_BIT
  );
  if (!obj->textureImageView) {
    fprintf(stderr, "failed to create texture image view\n");
    code = 0;
  }
  return code;
}

int createDepthResources(pAsobj obj) {
  int code = 1;
  fprintf(stdout, "Creating depth resources\n");

  VkFormat depthFormat = findDepthFormat(obj);
  createImage(
    obj,
    obj->swapChainExtent.width,
    obj->swapChainExtent.height,
    depthFormat,
    VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    &obj->depthImage,
    &obj->depthImageMemory
  );
  obj->depthImageView = createImageView(
    obj,
    obj->depthImage,
    depthFormat,
    VK_IMAGE_ASPECT_DEPTH_BIT
  );

  return code;
}

void loadObjFile(pAsobj obj, const char * filename) {
  
}

void loadModelFromFile(pAsobj obj, const char * filename) {
  fprintf(stdout, "Loading model from file: %s\n", filename);
  
}

int loadModel(pAsobj obj) {
  int code = 1;

  return code;
}

int cleanupSwapChain(pAsobj obj) {
  // fprintf(stdout, "Cleaning up swap chain\n");
  int code = 1;
#ifdef GLAD_VULKAN
  if (obj->depthImageView) {
    fprintf(stdout, "Destroying depth image view\n");
    glad_vkDestroyImageView(obj->device, obj->depthImageView, NULL);
    obj->depthImageView = VK_NULL_HANDLE;
  }
  if (obj->depthImage) {
    fprintf(stdout, "Destroying depth image\n");
    glad_vkDestroyImage(obj->device, obj->depthImage, NULL);
    obj->depthImage = VK_NULL_HANDLE;
  }
  if (obj->depthImageMemory) {
    fprintf(stdout, "Freeing depth image memory\n");
    glad_vkFreeMemory(obj->device, obj->depthImageMemory, NULL);
    obj->depthImageMemory = VK_NULL_HANDLE;
  }
  for (int i = 0; i < obj->swapChainImageCount; i++) {
    if (obj->swapChainFramebuffers[i]) {
      glad_vkDestroyFramebuffer(obj->device, obj->swapChainFramebuffers[i], NULL);
      obj->swapChainFramebuffers[i] = VK_NULL_HANDLE;
    }
    if (obj->swapChainImageViews[i]) {
      glad_vkDestroyImageView(obj->device, obj->swapChainImageViews[i], NULL);
      obj->swapChainImageViews[i] = VK_NULL_HANDLE;
    }
  }
  if (obj->swapChain) {
    glad_vkDestroySwapchainKHR(obj->device, obj->swapChain, NULL);
    obj->swapChain = VK_NULL_HANDLE;
  }
#else
  if (obj->depthImageView) {
    vkDestroyImageView(obj->device, obj->depthImageView, NULL);
    obj->depthImageView = VK_NULL_HANDLE;
  }
  if (obj->depthImage) {
    vkDestroyImage(obj->device, obj->depthImage, NULL);
    obj->depthImage = VK_NULL_HANDLE;
  }
  if (obj->depthImageMemory) {
    vkFreeMemory(obj->device, obj->depthImageMemory, NULL);
    obj->depthImageMemory = VK_NULL_HANDLE;
  }
  for (int i = 0; i < obj->swapChainImageCount; i++) {
    if (obj->swapChainFramebuffers[i]) {
      vkDestroyFramebuffer(obj->device, obj->swapChainFramebuffers[i], NULL);
      obj->swapChainFramebuffers[i] = VK_NULL_HANDLE;
    }
    if (obj->swapChainImageViews[i]) {
      vkDestroyImageView(obj->device, obj->swapChainImageViews[i], NULL);
      obj->swapChainImageViews[i] = VK_NULL_HANDLE;
    }
  }
  if (obj->swapChain) {
    vkDestroySwapchainKHR(obj->device, obj->swapChain, NULL);
    obj->swapChain = VK_NULL_HANDLE;
  }if(obj->swapChain) {
    vkDestroySwapchainKHR(obj->device, obj->swapChain, NULL);
    obj->swapChain = VK_NULL_HANDLE;
  }
#endif

  if (obj->swapChainImages) {
    free(obj->swapChainImages);
    obj->swapChainImages = NULL;
  }
  if (obj->swapChainImageViews) {
    free(obj->swapChainImageViews);
    obj->swapChainImageViews = NULL;
  }
  if (obj->swapChainFramebuffers) {
    free(obj->swapChainFramebuffers);
    obj->swapChainFramebuffers = NULL;
  }
  return code;
}

int recreateSwapChain(pAsobj obj) {
  int code = 1;
  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(obj->window, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(obj->window, &width, &height);
    glfwWaitEvents();
  }
  vkDeviceWaitIdle(obj->device);
  if (code) code = cleanupSwapChain(obj);
  if (code) code = createSwapChain(obj);
  if (code) code = createImageViews(obj);
  if (code) code = createDepthResources(obj);
  if (code) code = createFramebuffers(obj);
  return code;
}

int createTextureSampler(pAsobj obj) {
  int code = 1;
  fprintf(stdout, "Creating texture sampler\n");

  VkSamplerCreateInfo samplerInfo = {0};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;

  //we are going to use repeating for the address mode

  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.maxAnisotropy = 1.0f;

  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;
#ifdef GLAD_VULKAN
  if (glad_vkCreateSampler(obj->device, &samplerInfo, NULL, &obj->textureSampler) != VK_SUCCESS) {
    fprintf(stderr, "failed to create texture sampler\n");
    code = 0;
  }
#else
  if (vkCreateSampler(obj->device, &samplerInfo, NULL, &obj->textureSampler) != VK_SUCCESS) {
    fprintf(stderr, "failed to create texture sampler\n");
    code = 0;
  }
#endif
  return code;
}

int initVulkan(pAsobj obj) {
  int code = 1;
  if (code) code = createVulkanInstance(obj);
  if (code) code = createSurface(obj);
  if (code) code = selectPhysicalDevice(obj);
#ifdef GLAD_VULKAN
  if (code) initGLADLibraries(obj);
#endif
  if (code) code = createLogicalDevice(obj);
  if (code) code = createSwapChain(obj);
  if (code) code = createImageViews(obj);
  if (code) code = createRenderPass(obj);
  if (code) code = createDescriptorSetLayout(obj);
  if (code) code = createGraphicsPipeline(obj);
  if (code) code = createCommandPool(obj);
  if (code) code = createDepthResources(obj);
  if (code) code = createFramebuffers(obj);
  if (code) code = createTextureImage(obj);
  if (code) code = createTextureImageView(obj);
  if (code) code = createTextureSampler(obj);
  if (code) code = loadModel(obj);
  if (code) code = createVertexBuffer(obj);
  if (code) code = createIndexBuffer(obj);
  if (code) code = createUniformBuffers(obj);
  if (code) code = createDescriptorPool(obj);
  if (code) code = createDescriptorSets(obj);
  if (code) code = createCommandBuffers(obj);
  if (code) code = createSyncObjects(obj);
  return code;
}

int mainLoop(pAsobj obj) {
  int code = 1;
  while(!glfwWindowShouldClose(obj->window) && code) {
    glfwPollEvents();
    code = drawFrame(obj);
  }
  vkDeviceWaitIdle(obj->device);
  return code;
}

void cleanup(pAsobj obj) {
  fprintf(stdout, "Cleaning up\n");
  //Destroy the swap chains
  cleanupSwapChain(obj);
#ifdef GLAD_VULKAN
  //If we're using glad, then we want to use the glad functions
  if (obj->textureSampler) glad_vkDestroySampler(obj->device, obj->textureSampler, NULL);
  if (obj->textureImageView) glad_vkDestroyImageView(obj->device, obj->textureImageView, NULL);
  if (obj->textureImage) glad_vkDestroyImage(obj->device, obj->textureImage, NULL);
  if (obj->textureImageMemory) glad_vkFreeMemory(obj->device, obj->textureImageMemory, NULL);
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (obj->uniformBuffers[i]) glad_vkDestroyBuffer(obj->device, obj->uniformBuffers[i], NULL);
    if (obj->uniformBuffersMemory[i]) glad_vkFreeMemory(obj->device, obj->uniformBuffersMemory[i], NULL);
  }
  if (obj->descriptorPool) glad_vkDestroyDescriptorPool(obj->device, obj->descriptorPool, NULL);
  if (obj->descriptorSetLayout) glad_vkDestroyDescriptorSetLayout(obj->device, obj->descriptorSetLayout, NULL);
  if (obj->indexBuffer) glad_vkDestroyBuffer(obj->device, obj->indexBuffer, NULL);
  if (obj->indexBufferMemory) glad_vkFreeMemory(obj->device, obj->indexBufferMemory, NULL);
  if (obj->vertexBuffer) glad_vkDestroyBuffer(obj->device, obj->vertexBuffer, NULL);
  if (obj->vertexBufferMemory) glad_vkFreeMemory(obj->device, obj->vertexBufferMemory, NULL);
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (obj->imageAvailableSemaphores[i]) glad_vkDestroySemaphore(obj->device, obj->imageAvailableSemaphores[i], NULL);
    if (obj->renderFinishedSemaphores[i]) glad_vkDestroySemaphore(obj->device, obj->renderFinishedSemaphores[i], NULL);
    if (obj->inFlightFences[i]) glad_vkDestroyFence(obj->device, obj->inFlightFences[i], NULL);
  }
  if (obj->commandPool) glad_vkDestroyCommandPool(obj->device, obj->commandPool, NULL);
  if (obj->graphicsPipeline) glad_vkDestroyPipeline(obj->device, obj->graphicsPipeline, NULL);
  if (obj->pipelineLayout) glad_vkDestroyPipelineLayout(obj->device, obj->pipelineLayout, NULL);
  if (obj->renderPass) glad_vkDestroyRenderPass(obj->device, obj->renderPass, NULL);
  if (obj->device) glad_vkDestroyDevice(obj->device, NULL);
  if (obj->surface) glad_vkDestroySurfaceKHR(obj->instance, obj->surface, NULL);
  if (obj->instance) glad_vkDestroyInstance(obj->instance, NULL);
  #else
  //Otherwise, we use the traditional function names
  if (obj->textureSampler) vkDestroySampler(obj->device, obj->textureSampler, NULL);
  if (obj->textureImageView) vkDestroyImageView(obj->device, obj->textureImageView, NULL);
  if (obj->textureImage) vkDestroyImage(obj->device, obj->textureImage, NULL);
  if (obj->textureImageMemory) vkFreeMemory(obj->device, obj->textureImageMemory, NULL);
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (obj->uniformBuffers[i]) vkDestroyBuffer(obj->device, obj->uniformBuffers[i], NULL);
    if (obj->uniformBuffersMemory[i]) vkFreeMemory(obj->device, obj->uniformBuffersMemory[i], NULL);
  }
  if (obj->descriptorPool) vkDestroyDescriptorPool(obj->device, obj->descriptorPool, NULL);
  if (obj->descriptorSetLayout) vkDestroyDescriptorSetLayout(obj->device, obj->descriptorSetLayout, NULL);
  if (obj->indexBuffer) vkDestroyBuffer(obj->device, obj->indexBuffer, NULL);
  if (obj->indexBufferMemory) vkFreeMemory(obj->device, obj->indexBufferMemory, NULL);
  if (obj->vertexBuffer) vkDestroyBuffer(obj->device, obj->vertexBuffer, NULL);
  if (obj->vertexBufferMemory) vkFreeMemory(obj->device, obj->vertexBufferMemory, NULL);
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (obj->imageAvailableSemaphores[i]) vkDestroySemaphore(obj->device, obj->imageAvailableSemaphores[i], NULL);
    if (obj->renderFinishedSemaphores[i]) vkDestroySemaphore(obj->device, obj->renderFinishedSemaphores[i], NULL);
    if (obj->inFlightFences[i]) vkDestroyFence(obj->device, obj->inFlightFences[i], NULL);
  }
  if (obj->commandPool) vkDestroyCommandPool(obj->device, obj->commandPool, NULL);
  for (int i = 0; i < obj->swapChainImageCount; i++) {
    if (obj->swapChainFramebuffers[i]) vkDestroyFramebuffer(obj->device, obj->swapChainFramebuffers[i], NULL);
  }
  if (obj->graphicsPipeline) vkDestroyPipeline(obj->device, obj->graphicsPipeline, NULL);
  if (obj->pipelineLayout) vkDestroyPipelineLayout(obj->device, obj->pipelineLayout, NULL);
  if (obj->renderPass) vkDestroyRenderPass(obj->device, obj->renderPass, NULL);
  for (int i = 0; i < obj->swapChainImageCount; i++) {
    if (obj->swapChainImageViews[i]) vkDestroyImageView(obj->device, obj->swapChainImageViews[i], NULL);
  }
  if (obj->swapChain) vkDestroySwapchainKHR(obj->device, obj->swapChain, NULL);
  if (obj->device) vkDestroyDevice(obj->device, NULL);
  if (obj->surface) vkDestroySurfaceKHR(obj->instance, obj->surface, NULL);
  if (obj->instance) vkDestroyInstance(obj->instance, NULL);
  #endif
  if (obj->imageAvailableSemaphores) free(obj->imageAvailableSemaphores);
  if (obj->renderFinishedSemaphores) free(obj->renderFinishedSemaphores);
  if (obj->inFlightFences) free(obj->inFlightFences);
  if (obj->commandBuffers) free(obj->commandBuffers);
  if (obj->window) glfwDestroyWindow(obj->window);
  glfwTerminate();
}

/**
 * glfw callback functions
 */

void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
  pAsobj obj = glfwGetWindowUserPointer(window);
  obj->framebufferResized = 1;
}

int initWindow(pAsobj obj) {
  int code = 1;
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  if (!(obj->window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", NULL, NULL))) {
    fprintf(stderr, "Failed to create window\n");
  }
  glfwSetWindowUserPointer(obj->window, obj);
  glfwSetFramebufferSizeCallback(obj->window, framebufferResizeCallback);

  int version[3] = {0};
  glfwGetVersion(&version[0], &version[1], &version[2]);
  fprintf(stdout, "GLFW version: %d.%d.%d\n", version[0], version[1], version[2]);
  return code;
}

/**
 * run
 */

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

/**
 * main
 */

int main(void) {
  AppStruct obj = {0};
  int code = run(&obj);
  return !code;
}