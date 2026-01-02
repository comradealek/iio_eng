#ifndef IIO_VULKAN_API
#define IIO_VULKAN_API

#include <stdint.h>
#include "cglm/cglm.h"
#include "iio_eng_typedef.h"
#include "iio_resource_loaders.h"
#include "iio_descriptors.h"

#define DEFAULT_WINDOW_WIDTH 640
#define DEFAULT_WINDOW_HEIGHT 480
#define MAX_FRAMES_IN_FLIGHT 2

#define clamp(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
#define min(x,y) ((x) < (y) ? (x) : (y))
#define max(x,y) ((x) > (y) ? (x) : (y))

typedef struct TestCubeData_S {
  bool                                      isInitialized;

  Vertex                                    vertices [8];
  VkBuffer                                  vertexBuffer;
  VkDeviceMemory                            vertexBufferMemory;
  
  uint32_t                                  indices [36];
  VkBuffer                                  indexBuffer;
  VkDeviceMemory                            indexBufferMemory;

  IIOImageHandle                            textureImage;
  VkDescriptorSet                           texSamplerDescriptorSets [MAX_FRAMES_IN_FLIGHT];

  
  ModelUniformBufferData                    modelUniformBufferData [MAX_FRAMES_IN_FLIGHT];
  VkBuffer                                  modelUniformBuffer [MAX_FRAMES_IN_FLIGHT];
  VkDeviceMemory                            modelUniformBufferMemory [MAX_FRAMES_IN_FLIGHT];
  void *                                    modelUniformBufferMapped [MAX_FRAMES_IN_FLIGHT];
  VkDescriptorSet                           modelUniformBufferDescriptorSets [MAX_FRAMES_IN_FLIGHT];
} TestCubeData;

typedef struct DataBuffer_S {
  uint32_t size;
  uint32_t data [];
} DataBuffer;

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

  VkCommandPool commandPool;
  VkCommandBuffer * commandBuffers;

  VkBuffer globalUniformBuffers [2];
  VkDeviceMemory globalUniformBuffersMemory [2];
  void * globalUniformBuffersMapped [2];
  VkDescriptorSet cameraDescriptorSets [2];

  IIOGraphicsPipelineManager graphicsPipelineManger;

  uint32_t descriptorPoolManagerCount;
  IIODescriptorPoolManager * descriptorPoolMangers;

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

  IIODescriptorSetWriter descriptorSetWriter;

  IIOResourceManager resourceManager;

  IIOModel testModel;

} IIOVulkanState;

IIOVulkanState * iio_init_vulkan_api();

void iio_init_vulkan();

void iio_create_instance();

void iio_create_window();

void iio_create_surface();

void iio_select_physical_device();

void iio_create_device();

void iio_create_swapchain();

void iio_create_swapchain_image_views();

void iio_create_application_descriptor_pool_managers();

void iio_create_descriptor_pool_managers_testcube();

void iio_initialize_testcube();

void iio_initialize_camera();

void iio_create_application_graphics_pipeline();

void iio_create_graphics_pipeline_testtriangle();

void iio_create_graphics_pipeline_testcube();

void iio_create_command_pool();

void iio_create_depth_resources();

static VkFormat iio_find_depth_format();

void iio_create_vertex_buffer_testcube();

void iio_create_index_buffer_testcube();

void iio_create_uniform_buffers();

void iio_create_uniform_buffer(VkDevice device, VkDeviceSize bufferSize, VkBuffer * buffer, VkDeviceMemory * bufferMemory, void ** bufferMapped);

void iio_create_descriptor_pool_api();

void iio_create_descriptor_sets();

void iio_create_command_buffers();

void iio_create_synchronization_objects();

void iio_initialize_resource_loader();

void iio_create_texture_image_func(const char * path, VkImage * image, VkDeviceMemory * imageMemory, VkImageView * imageView);

void iio_create_texture_image_from_memory_func(const uint8_t * data, size_t dataSize, VkImage * image, VkDeviceMemory * imageMemory, VkImageView * imageView);

void iio_create_texture_image_from_pixels_func(const uint8_t * pixels, size_t width, size_t height, VkImage * image, VkDeviceMemory * imageMemory, VkImageView * imageView);

void iio_create_image_sampler_func(const VkSamplerCreateInfo * createInfo, VkSampler * sampler);

DataBuffer * iio_read_shader_file_to_buffer(const char * path);

VkShaderModule iio_create_shader_module(const DataBuffer * shaderCode);

void iio_select_physical_device_properties (
  VkPhysicalDevice physicalDevice,
  VkSurfaceFormatKHR * preferredSurfaceFormat,
  VkPresentModeKHR * preferredPresentMode,
  VkSurfaceCapabilitiesKHR * surfaceCapabilities,
  uint32_t * graphicsQueueIndex,
  uint32_t * presentQueueIndex,
  VkPhysicalDeviceType * deviceType
);

static VkVertexInputBindingDescription iio_get_binding_description();

static VkVertexInputAttributeDescription * iio_get_attribute_descriptions(int * count);

uint32_t iio_find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties);

void iio_create_buffer(
  VkDeviceSize size,
  VkBufferUsageFlags usage,
  VkMemoryPropertyFlags properties,
  VkBuffer * buffer,
  VkDeviceMemory * bufferMemory
);

void iio_copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

VkCommandBuffer iio_begin_single_time_commands();

void iio_end_single_time_commands(VkCommandBuffer commandBuffer);

void iio_update_camera_uniform_buffer(uint32_t currentFrame);

void iio_create_texture_image(const char * path, VkImage * textureImage, VkDeviceMemory * textureImageMemory);

void iio_create_texture_image_from_memory(const uint8_t * data, int size, VkImage * textureImage, VkDeviceMemory * textureImageMemory);

void iio_create_texture_image_from_pixels(const uint8_t * pixels, int width, int height, VkImage * textureImage, VkDeviceMemory * textureImageMemory);

void iio_create_texture_image_view(VkImage textureImage, VkImageView * textureImageView);

void iio_create_texture_sampler(VkSampler * textureSampler);

void iio_create_image(
  uint32_t width,
  uint32_t height,
  VkImage * textureImage,
  VkDeviceMemory * textureImageMemory,
  VkFormat format,
  VkImageTiling tiling,
  VkImageUsageFlags usage,
  VkMemoryPropertyFlags properties
);

void iio_create_image_view(VkImage image, VkImageView * imageView, VkFormat format, VkImageAspectFlags aspectMask);

void iio_create_image_sampler(VkSampler * textureSampler);

void iio_transition_image_layout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

VkFormat iio_find_supported_format(const VkFormat * candidates, uint32_t count, VkImageTiling tiling, VkFormatFeatureFlags features);

void iio_copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

void iio_framebuffer_size_callback(GLFWwindow * window, int width, int height);

void iio_mouse_button_callback(GLFWwindow * window, int button, int action, int mods);

void iio_scroll_callback(GLFWwindow * window, double xoffset, double yoffset);

void iio_key_callback(GLFWwindow * window, int key, int scancode, int action, int mods);

void iio_cursor_position_callback(GLFWwindow * window, double xpos, double ypos);

void iio_run();

void draw_frame();

void iio_record_render_to_command_buffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame);

void iio_record_testtriangle_command_buffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame);

void iio_record_testcube_command_buffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame);

void iio_record_primitive_command_buffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame, IIOPrimitive * primitive);

void iio_recreate_swapchain();

void iio_cleanup();

void iio_cleanup_device();

void iio_cleanup_swapchain();

#endif