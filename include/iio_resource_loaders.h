#ifndef IIO_MODEL_H
#define IIO_MODEL_H

#include <vulkan/vulkan.h>
#include "cglm/cglm.h"
#include "cgltf.h"

#include "iio_string_wrapper.h"

#define IIOVERTEX_ATTRIBUTE_COUNT 8

#ifndef IIO_PATH_TO_TEXTURES
#define IIO_PATH_TO_TEXTURES "resources/textures/"
#endif

#ifndef IIO_PATH_TO_MODELS
#define IIO_PATH_TO_MODELS "resources/models/"
#endif

typedef enum {
  GLTF_AM_UNDEFINED,
  GLTF_AM_OPAQUE,
  GLTF_AM_MASK,
  GLTF_AM_BLEND
} gltfAlphaMode;

typedef struct IIOTextureInfo_S {
  VkImage                                   image;
  VkImageView                               imageView;
  VkDeviceMemory                            imageMemory;
  VkSampler                                 sampler;
  uint32_t                                  texCoord;
} IIOTextureInfo;

typedef struct IIOPBRMetallicRoughness_S {
  IIOTextureInfo                            baseColorTextureInfo;
  IIOTextureInfo                            metallicRoughnessTextureInfo;
  vec4                                      baseColorFactor;
  float                                     metallicFactor;
  float                                     roughnessFactor;
} IIOPBRMetallicRoughness;

typedef struct IIOTextureInfoNormal_S {
  IIOTextureInfo                            textureInfo;
  float                                     scale;
} IIOTextureInfoNormal;

typedef struct IIOTextureInfoOcclusion_S {
  IIOTextureInfo                            textureInfo;
  float                                     strength;
} IIOTextureInfoOcclusion;

typedef struct IIOMaterial_S {
  IIOPBRMetallicRoughness                   pbrMetallicRoughness;
  IIOTextureInfoNormal                      normalTexture;
  IIOTextureInfoOcclusion                   occlusionTexture;
  IIOTextureInfo                            emissiveTexture;
  vec3                                      emissiveFactor;
  gltfAlphaMode                             alphaMode;
  float                                     alphaCutoff;
  bool                                      doubleSided;
} IIOMaterial;

typedef struct IIOVertex_S {
  vec3                                      position;
  vec3                                      normal;
  vec4                                      tangent;
  vec2                                      texCoord [2];
  vec4                                      color;
  vec4                                      joints;
  vec4                                      weights;
} IIOVertex;

typedef struct IIOPrimitive_S {
  IIOVertex *                               vertices;
  uint32_t                                  vertexCount;
  uint32_t *                                indices;
  uint32_t                                  indexCount;
  VkBuffer                                  vertexBuffer;
  VkBuffer                                  indexBuffer;
  IIOMaterial                               material;
  uint8_t                                   mode; // default is 4 (GL_TRIANGLES)
  // TODO: targets
} IIOPrimitive;

typedef struct IIOMesh_S {
  IIOPrimitive *                            primitives;
  uint32_t                                  primitiveCount;
  // TODO: weights
} IIOMesh;

typedef struct IIOModel_S {
  IIOMesh *                                 meshes;
  uint32_t                                  meshCount;
  mat4                                      modelMatrix;
} IIOModel;

typedef struct IIOPrimitive2_S {
  VkBuffer                                  vertexBuffer;
  VkBuffer                                  indexBuffer;
} IIOPrimitive2;

typedef struct IIOPrimitiveGroup_S {
  VkDescriptorSet                           baseColorDescriptor;
  VkDescriptorSet                           metallicRoughnessDescriptor;
  VkDescriptorSet                           normalDescriptor;
  VkDescriptorSet                           occlusionDescriptor;
  VkDescriptorSet                           emmissiveDescriptor;
} IIOPrimitiveGroup;

typedef struct IIOModel2_S {
  IIOPrimitiveGroup *                       primitiveGroups;
  VkBuffer                                  primitiveMeshIDBuffer;
  VkBuffer                                  groupMaterialUBOBuffer;
} IIOModel2;

typedef struct IIOImageHandle_S {
  VkImage                                   data;
  VkImageView                               view;
  VkSampler                                 sampler;
  VkDeviceMemory                            memory;
  VkDescriptorSet                           descriptor;
} IIOImageHandle;

typedef enum IIOImageType_E {
  iio_image_type_path,
  iio_image_type_data,
  iio_image_type_pixels,

  iio_image_type_maxenum
} IIOImageType;

typedef IIOModel * IIOModelPtr;

#define T hmap_strModel, IIOStringWrapper, IIOModelPtr, (c_keypro)
#include "stc/hmap.h"

#define T hmap_strImg, IIOStringWrapper, IIOImageHandle, (c_keypro)
#include "stc/hmap.h"

typedef struct IIOResourceManager_S {
  hmap_strModel modelMap;
  hmap_strImg imageMap;
} IIOResourceManager;

typedef void (* IIOCreateTextureImageFunc) (const char * path, VkImage * image, VkDeviceMemory * imageMemory, VkImageView * imageView);
typedef void (* IIOCreateTextureImageFromMemoryFunc) (const uint8_t * data, size_t size, VkImage * image, VkDeviceMemory * imageMemory, VkImageView * imageView);
typedef void (* IIOCreateTextureImageFromPixelsFunc) (const uint8_t * pixels, size_t width, size_t height, VkImage * image, VkDeviceMemory * imageMemory, VkImageView * imageView);
typedef void (* IIOCreateImageSamplerFunc) (const VkSamplerCreateInfo * samplerInfo, VkSampler * sampler);

extern const char * testModelPath;

extern const VkSamplerCreateInfo defaultSamplerCreateInfo;

void iio_set_create_texture_image_func(IIOCreateTextureImageFunc func);

void iio_set_create_texture_image_from_memory_func(IIOCreateTextureImageFromMemoryFunc func);

void iio_set_create_texture_image_from_pixels_func(IIOCreateTextureImageFromPixelsFunc func);

void iio_set_create_image_sampler_func(IIOCreateImageSamplerFunc func);

VkVertexInputBindingDescription iio_get_iiovertex_binding_description();

VkVertexInputAttributeDescription * iio_get_iiovertex_attribute_descriptions(int * count);

void iio_initialize_default_texture_resources(IIOResourceManager * manager);

void iio_initialize_resource_manager(IIOResourceManager * manager);

void iio_load_model(
  IIOResourceManager *                      manager,
  const char *                              path, 
  IIOModel *                                model
);

void iio_load_image(
  IIOResourceManager *                      manager, 
  const char * path, IIOImageHandle *       image, 
  const VkSamplerCreateInfo *               samplerInfo
);

void iio_extract_cgltf_mesh(
  cgltf_mesh *                              cgltfMesh, 
  IIOMesh *                                 iioMesh
);

void iio_extract_cgltf_vertices(
  cgltf_attribute *                         cgltfAttributes, 
  cgltf_size                                cgltfAttributeCount, 
  IIOVertex **                              pVertices, 
  uint32_t *                                pVertexCount
);

void iio_extract_cgltf_primitive(
  cgltf_primitive *                         cgltfPrimitive, 
  IIOPrimitive *                            iioPrimitive
);

void iio_extract_cgltf_material(
  cgltf_material *                          cgltfMaterial, 
  IIOMaterial *                             iioMaterial
);

void iio_extract_cgltf_texture(
  cgltf_texture *                           cgltfTexture, 
  VkImage *                                 image, 
  VkDeviceMemory *                          imageMemory, 
  VkImageView *                             imageView
);

void iio_destroy_resources(VkDevice device);

void iio_destroy_model(IIOModel * model);

void iio_destroy_image(
  VkDevice                                  device, 
  const char *                              name, 
  IIOResourceManager *                      manager
);

void iio_destroy_material(IIOMaterial * material);

void iio_load_test_model();

#endif