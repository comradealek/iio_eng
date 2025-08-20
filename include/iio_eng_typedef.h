#ifndef IIOTYPEDEF
#define IIOTYPEDEF

#include <stdint.h>
#include <vulkan/vulkan.h>
#include "cglm/cglm.h"
#include "GLFW/glfw3.h"
#include "iio_descriptors.h"
#include "iio_pipeline.h"

typedef struct CameraUniformBufferData_S {
  mat4 view;
  mat4 projection;
} CameraUniformBufferData;

typedef struct ModelUniformBufferData_S {
  mat4 position;
} ModelUniformBufferData;

typedef struct MaterialUniformBufferData_S {
  vec4 baseColorFactor;
  vec4 emissiveFactor;
  alignas(16) float metallicFactor;
  alignas(16) float roughnessFactor;
  alignas(16) float normalScale;
  alignas(16) float occlusionStrength;
  alignas(16) float alphaCutoff;
} MaterialUniformBufferData;

typedef struct Vertex_S {
  vec3 position;
  vec3 color;
  vec2 texCoord;
} Vertex;

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

#endif