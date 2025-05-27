#ifndef IIO_VULKAN_API
#define IIO_VULKAN_API

#define DEFAULT_WINDOW_WIDTH 640
#define DEFAULT_WINDOW_HEIGHT 480

/* typedef struct QueueFamilyIndices_S {
  uint32_t graphicsFamily;
  uint32_t hasGraphicsFamily;
  uint32_t presentFamily;
  uint32_t hasPresentFamily;
} QueueFamilyIndices; */

typedef struct IIOVulkanState_S {
  VkInstance instance;
  GLFWwindow * window;
  VkSurfaceKHR surface;
  VkPhysicalDevice * physicalDevices;
  VkPhysicalDevice selectedDevice;
  VkDevice device;
  VkCommandPool commandPool;
  VkCommandBuffer commandBuffer;
  VkRenderPass renderPass;
  VkSwapchainKHR swapChain;

  uint32_t graphicsQueueFamilyIndex;
  uint32_t presentQueueFamilyIndex;
  VkQueue graphicsQueue;
  VkQueue presentQueue;
} IIOVulkanState;

IIOVulkanState * iio_init_vulkan_api();

void iio_init_vulkan();

void iio_create_instance();

void iio_create_window();

void iio_create_surface();

void iio_create_swapchain();

void iio_select_physical_device();

void iio_create_device();

void iio_create_command_pool();

void iio_create_command_buffer();

void iio_create_render_pass();

// QueueFamilyIndices iio_find_queue_families(VkPhysicalDevice device);

void iio_run();

void iio_record_command_buffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags flags);

void iio_cleanup();

#endif