#ifndef IIOTYPEDEF
#define IIOTYPEDEF

#include <stdint.h>
#include <vulkan/vulkan.h>
#include "cglm/cglm.h"
#include "GLFW/glfw3.h"

typedef struct UniformBufferObject_S {
  mat4 model;
  mat4 view;
  mat4 projection;
} UniformBufferObject;

typedef struct DataBuffer_S {
  uint32_t size;
  uint32_t data [];
} DataBuffer;

typedef struct Vertex_S {
  vec3 position;
  vec3 color;
  vec3 texCoord;
} Vertex;

typedef struct IIOTextureInfo_S {
  VkSampler sampler;
  VkImage image;
  VkImageView imageView;
  uint32_t texCoord;
} IIOTextureInfo;

typedef struct IIOPBRMetallicRoughness_S {
  vec4 baseColorFactor;
  IIOTextureInfo baseColorTexture;
  float metallicFactor;
  float roughnessFactor;
  IIOTextureInfo metallicRoughnessTexture;
} IIOPBRMetallicRoughness;

typedef struct IIOTextureInfoNormal_S {
  VkImage image;
  VkImageView imageView;
  VkSampler sampler;
  uint32_t texCoord;
  float scale;
} IIOTextureInfoNormal;

typedef struct IIOTextureInfoOcclusion_S {
  VkImage image;
  VkImageView imageView;
  VkSampler sampler;
  uint32_t texCoord;
  float strength;
} IIOTextureInfoOcclusion;

typedef struct IIOMaterial_S {
  IIOPBRMetallicRoughness pbrMetallicRoughness;
  IIOTextureInfoNormal normalTexture;
  IIOTextureInfoOcclusion occlusionTexture;
  IIOTextureInfo emissiveTexture;
  vec3 emissiveFactor;
  float alphaMode;
  float alphaCutoff;
  bool doubleSided;
} IIOMaterial;

typedef struct IIOVertex_S {
  vec3 position;
  vec3 normal;
  vec4 tangent;
  vec2 texCoord [2];
  vec4 color;
  vec4 joints;
  vec4 weights;
} IIOVertex;

typedef struct IIOPrimitive_S {
  IIOVertex * vertices;
  uint32_t vertexCount;
  uint32_t * indices;
  uint32_t indexCount;
  IIOMaterial material;
  uint8_t mode; // default is 4 (GL_TRIANGLES)
  // TODO: targets
} IIOPrimitive;

typedef struct IIOMesh_S {
  IIOPrimitive * primitives;
  uint32_t primitiveCount;
  // TODO: weights
} IIOMesh;

typedef struct IIOModel_S {
  IIOMesh * meshes;
  uint32_t meshCount;
} IIOModel;

typedef struct IIOVulkanCamera_S {
  vec3 position;
  vec3 front;
  vec3 up;
  vec3 right;
  vec3 forward;
  float yaw;
  float pitch;
  float fov;
} IIOVulkanCamera;

typedef struct IIOVulkanState_S {
  VkInstance instance;
  GLFWwindow * window;
  VkSurfaceKHR surface;
  VkPhysicalDevice * physicalDevices;
  VkPhysicalDevice selectedDevice;
  VkDevice device;
  VkSwapchainKHR swapChain;
  uint32_t swapChainImageCount;
  VkImage * swapChainImages;
  VkImageView * swapChainImageViews;
  VkFramebuffer * framebuffers;
  VkRenderPass renderPass;
  VkDescriptorSetLayout descriptorSetLayout;
  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;

  VkCommandPool commandPool;
  VkCommandBuffer * commandBuffers;

  //  Buffers for test cube and texture
  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  VkBuffer indexBuffer;
  VkDeviceMemory indexBufferMemory;
  VkImage textureImage;
  VkDeviceMemory textureImageMemory;
  VkImageView textureImageView;
  VkSampler textureSampler;
  //  end of test cube data

  VkBuffer * uniformBuffers;
  VkDeviceMemory * uniformBuffersMemory;
  void ** uniformBuffersMapped;

  VkDescriptorPool descriptorPool;
  VkDescriptorSet * descriptorSets;

  VkSemaphore * imageAvailableSemaphores;
  VkSemaphore * renderFinishedSemaphores;
  VkFence * inFlightFences;

  VkImage depthImage;
  VkDeviceMemory depthImageMemory;
  VkImageView depthImageView;

  VkSurfaceFormatKHR surfaceFormat;
  VkPresentModeKHR presentMode;
  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  VkExtent2D swapChainImageExtent;
  uint32_t graphicsQueueFamilyIndex;
  uint32_t presentQueueFamilyIndex;
  VkQueue graphicsQueue;
  VkQueue presentQueue;

  uint32_t currentFrame;
  uint8_t framebufferResized;
} IIOVulkanState;

#endif