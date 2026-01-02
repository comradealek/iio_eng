#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <vulkan/vulkan.h>
#include "cgltf.h"
#include "cglm/cglm.h"
#include "stb_image.h"

#include "iio_resource_loaders.h"
#include "iio_string_wrapper.h"
// #include "iio_eng_typedef.h"

/**
 *   constants
 */

const char * testModelPath = "resources/models/Avocado.gltf";
const uint8_t defaultRGBAdat [4] = {0xff, 0xff, 0xff, 0xff}; // White RGBA color
const uint8_t defaultNormalDat [4] = {0x7f, 0x7f, 0xff, 0xff}; // Neutral normal map color
const VkSamplerCreateInfo defaultSamplerCreateInfo = {
  .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
  .magFilter = VK_FILTER_LINEAR,
  .minFilter = VK_FILTER_LINEAR,
  .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
  .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
  .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
  .anisotropyEnable = VK_FALSE,
  .maxAnisotropy = 1.0f,
  .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK
};


const char * IIO_DEFAULT_NORMAL_NAME = "IIO_DEFAULT_NORMAL";
const char * IIO_DEFAULT_TEXTURE_NAME = "IIO_DEFAULT_TEXTURE";

/**
 *   Default Texture Data
 */

VkImage defaultRGBAImage = VK_NULL_HANDLE;
VkDeviceMemory defaultRGBAImageMemory = VK_NULL_HANDLE;
VkImageView defaultRGBAImageView = VK_NULL_HANDLE;
VkSampler defaultSampler = VK_NULL_HANDLE;
VkImage defaultNormalImage = VK_NULL_HANDLE;
VkDeviceMemory defaultNormalImageMemory = VK_NULL_HANDLE;
VkImageView defaultNormalImageView = VK_NULL_HANDLE;

/**
 *   Function Pointers
 */

IIOCreateTextureImageFunc iioCreateTextureImageFunc = NULL;
IIOCreateTextureImageFromMemoryFunc iioCreateTextureImageFromMemoryFunc = NULL;
IIOCreateTextureImageFromPixelsFunc iioCreateTextureImageFromPixelsFunc = NULL;
IIOCreateImageSamplerFunc iioCreateImageSamplerFunc = NULL;

void iio_set_create_texture_image_func(
  IIOCreateTextureImageFunc                 func) 

{
  iioCreateTextureImageFunc = func;
}

void iio_set_create_texture_image_from_memory_func(
  IIOCreateTextureImageFromMemoryFunc       func) 

{
  iioCreateTextureImageFromMemoryFunc = func;
}

void iio_set_create_texture_image_from_pixels_func(
  IIOCreateTextureImageFromPixelsFunc       func) 

{
  iioCreateTextureImageFromPixelsFunc = func;
}

void iio_set_create_image_sampler_func(
  IIOCreateImageSamplerFunc                 func) 

{
  iioCreateImageSamplerFunc = func;
}

/**
 *   descriptors   *
 */

VkVertexInputBindingDescription iio_get_iiovertex_binding_description() {
  VkVertexInputBindingDescription bindingDescription = {0};

  bindingDescription.binding = 0; // Binding index
  bindingDescription.stride = sizeof(IIOVertex); // Size of each vertex
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Each vertex is a separate instance

  return bindingDescription;
}

VkVertexInputAttributeDescription * iio_get_iiovertex_attribute_descriptions(
  int *                                     count) 

{
  *count = IIOVERTEX_ATTRIBUTE_COUNT;
  static VkVertexInputAttributeDescription attributeDescriptions[IIOVERTEX_ATTRIBUTE_COUNT];

  attributeDescriptions[0].binding = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // Position format
  attributeDescriptions[0].offset = offsetof(IIOVertex, position);

  attributeDescriptions[1].binding = 0;
  attributeDescriptions[1].location = 1;
  attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // Normal format
  attributeDescriptions[1].offset = offsetof(IIOVertex, normal);

  attributeDescriptions[2].binding = 0;
  attributeDescriptions[2].location = 2;
  attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT; // Tangent format
  attributeDescriptions[2].offset = offsetof(IIOVertex, tangent);

  for (int i = 0; i < 2; i++) {
    attributeDescriptions[i + 3].binding = 0;
    attributeDescriptions[i + 3].location = i + 3;
    attributeDescriptions[i + 3].format = VK_FORMAT_R32G32_SFLOAT; // TexCoord format
    attributeDescriptions[i + 3].offset = offsetof(IIOVertex, texCoord[i]);
  }

  attributeDescriptions[5].binding = 0;
  attributeDescriptions[5].location = 5;
  attributeDescriptions[5].format = VK_FORMAT_R32G32B32A32_SFLOAT; // Color format
  attributeDescriptions[5].offset = offsetof(IIOVertex, color);

  attributeDescriptions[6].binding = 0;
  attributeDescriptions[6].location = 6;
  attributeDescriptions[6].format = VK_FORMAT_R32G32B32A32_SFLOAT; // Joints format
  attributeDescriptions[6].offset = offsetof(IIOVertex, joints);

  attributeDescriptions[7].binding = 0;
  attributeDescriptions[7].location = 7;
  attributeDescriptions[7].format = VK_FORMAT_R32G32B32A32_SFLOAT; // Weights format
  attributeDescriptions[7].offset = offsetof(IIOVertex, weights);

  return attributeDescriptions;
}

/**
 *  Initialization Functions
 */

void iio_initialize_resource_manager(
  IIOResourceManager *                      manager) 

{
  manager->modelMap = hmap_strModel_init();
  manager->imageMap = hmap_strImg_init();
}

void iio_initialize_default_texture_resources(
  IIOResourceManager *                      manager) 

{
  fprintf(stdout, "initializing default texture resources\n");
  //  Load the default RGBA texture
  iioCreateTextureImageFromPixelsFunc(defaultRGBAdat, 1, 1, &defaultRGBAImage, &defaultRGBAImageMemory, &defaultRGBAImageView);
  
  //  Load the default normal texture
  iioCreateTextureImageFromPixelsFunc(defaultNormalDat, 1, 1, &defaultNormalImage, &defaultNormalImageMemory, &defaultNormalImageView);
  
  iioCreateImageSamplerFunc(&defaultSamplerCreateInfo, &defaultSampler);

  IIOImageHandle image = {
    .data = defaultRGBAImage,
    .memory = defaultRGBAImageMemory,
    .view = defaultRGBAImageView,
    .sampler = defaultSampler
  };

  hmap_strImg_insert(&manager->imageMap, IIOStringWrapper_from(IIO_DEFAULT_TEXTURE_NAME), image);

  IIOImageHandle normal = {
    .data = defaultNormalImage,
    .memory = defaultNormalImageMemory,
    .view = defaultNormalImageView,
    .sampler = defaultSampler
  };

  hmap_strImg_insert(&manager->imageMap, IIOStringWrapper_from(IIO_DEFAULT_NORMAL_NAME), normal);
}

/*****************
 *    Loaders    *
 *****************/

void iio_load_model2(
  IIOResourceManager *                      manager,
  const char *                              filename,
  IIOModel2 *                               model)

{
  //  Check if the pointer to model in NULL. The algorithm expects there to be an initialized model struct.
  if (!model) {
    fprintf(stderr, "Tried to load GLTF Model into a null IIOModel2]n");
    return;
  }

  //  Check if the model is already loaded. If so, then we will just copy the existing data.
  if (hmap_strModel_contains(&manager->modelMap, filename)) {
    IIOModel2 * mappedModel = hmap_strModel_at_mut(&manager->modelMap, filename);
    model->primitiveGroups = mappedModel->primitiveGroups;
    model->groupMaterialUBOBuffer = mappedModel->groupMaterialUBOBuffer;
    model->primitiveMeshIDBuffer = mappedModel->primitiveMeshIDBuffer;
  }

  //  Load the file if it hasn't been already. If there are any problems then print out errors and return.
  char path [255] = IIO_PATH_TO_MODELS;
  strncat(path, filename, sizeof(path) - sizeof(IIO_PATH_TO_MODELS) - 1);
  cgltf_options options = {0};
  cgltf_data * data = NULL;
  cgltf_result result = cgltf_parse_file(&options, path, &data);
  if (result != cgltf_result_success) {
    fprintf(stderr, "Failed to parse model file: %s\n", path);
    return;
  }
  result = cgltf_load_buffers(&options, data, path);
  if (result != cgltf_result_success) {
    fprintf(stderr, "Failed to load buffers for model file: %s\n", path);
    cgltf_free(data);
    return;
  }

  //
}

void iio_load_model(
  IIOResourceManager *                      manager,
  const char *                              filename, 
  IIOModel *                                model) 

{
  if (!model) {
    fprintf(stderr, "Tried to load GLTF Model into a NULL IIOModel\n");
    return;
  }
  if (hmap_strModel_contains(&manager->modelMap, filename)) {
    /* IIOModel2 * mappedModel = hmap_strModel_at_mut(&manager->modelMap, filename);
    model->meshCount = mappedModel->meshCount;
    model->meshes = mappedModel->meshes;
    memcpy(&model->modelMatrix, &mappedModel->modelMatrix, sizeof(mat4));
    return; */
  }
  char path [255] = IIO_PATH_TO_MODELS;
  strncat(path, filename, sizeof(path) - sizeof(IIO_PATH_TO_MODELS) - 1);
  cgltf_options options = {0};
  cgltf_data * data = NULL;
  cgltf_result result = cgltf_parse_file(&options, path, &data);
  if (result != cgltf_result_success) {
    fprintf(stderr, "Failed to parse model file: %s\n", path);
    return;
  }
  result = cgltf_load_buffers(&options, data, path);
  if (result != cgltf_result_success) {
    fprintf(stderr, "Failed to load buffers for model file: %s\n", path);
    cgltf_free(data);
    return;
  }

  glm_mat4_identity(model->modelMatrix); // Initialize model matrix to identity
  if (data->meshes_count > (uint32_t) - 1) {
    fprintf(stderr, "Too many meshes in the model file: %s\n", path);
    cgltf_free(data);
    return;
  }
  model->meshCount = (uint32_t) data->meshes_count;
  model->meshes = malloc(model->meshCount * sizeof(IIOMesh));
  if (!model->meshes) {
    fprintf(stderr, "Failed to allocate memory for model meshes\n");
    cgltf_free(data);
    return;
  }
  //  Iterate through the meshes

  IIOMesh * iioMesh = model->meshes;
  cgltf_mesh * cgltfMesh = data->meshes;
  for (int i = 0; i < data->meshes_count; i++) {
    iio_extract_cgltf_mesh(&cgltfMesh[i], &iioMesh[i]);
  }
  
  cgltf_free(data);

  hmap_strModel_insert(
    &manager->modelMap, 
    IIOStringWrapper_make(filename), 
    model
  );
}

void iio_load_image(
  IIOResourceManager *                      manager, 
  const char *                              filename, 
  IIOImageHandle *                          image, 
  const VkSamplerCreateInfo *               samplerInfo) 

{
  if (hmap_strImg_contains(&manager->imageMap, filename)) {
    IIOImageHandle * mappedImage = hmap_strImg_at_mut(&manager->imageMap, filename);
    image->data = mappedImage->data;
    image->memory = mappedImage->memory;
    image->sampler = mappedImage->sampler;
    image->view = mappedImage->view;
  } else {
    char path [255] = IIO_PATH_TO_TEXTURES;
    strncat(path, filename, sizeof(path) - sizeof(IIO_PATH_TO_TEXTURES) - 1);
    iioCreateTextureImageFunc(path, &image->data, &image->memory, &image->view);
    iioCreateImageSamplerFunc(samplerInfo, &image->sampler);
    hmap_strImg_insert(
      &manager->imageMap, 
      IIOStringWrapper_make(filename), 
      (IIOImageHandle) {
        .data = image->data, 
        .memory = image->memory, 
        .sampler = image->sampler, 
        .view = image->view, 
      }
    );
  }
}

/**
 * Helper Functions
 */

void iio_extract_cgltf_mesh(
  cgltf_mesh *                              cgltfMesh, 
  IIOMesh *                                 iioMesh) 
  
{
  if (!cgltfMesh) return;
  if (!iioMesh) {
    fprintf(stderr, "Tried to extract to a NULL IIOMesh\n");
    return;
  }
  //  Get the primitives [1-*]:required
  iioMesh->primitiveCount = (uint32_t) cgltfMesh->primitives_count;
  iioMesh->primitives = malloc(iioMesh->primitiveCount * sizeof(IIOPrimitive));
  if (!iioMesh->primitives) {
    fprintf(stderr, "Failed to allocate memory for IIOMesh primitives\n");
    iioMesh->primitiveCount = 0;
    return;
  }
  //  Iterate through the primitives
  cgltf_primitive * cgltfPrimitive = cgltfMesh->primitives;
  IIOPrimitive * iioPrimitive = iioMesh->primitives;
  for (cgltf_size i = 0; i < cgltfMesh->primitives_count; i++) {
    iio_extract_cgltf_primitive(&cgltfPrimitive[i], &iioPrimitive[i]);
    if (!iioPrimitive->vertices || iioPrimitive->vertexCount == 0) {
      fprintf(stderr, "Failed to extract vertices for primitive %zu in mesh %s\n", i, cgltfMesh->name);
    }
  }
  //  TODO: handle weights
}

void iio_extract_cgltf_primitive(
  cgltf_primitive *                         cgltfPrimitive, 
  IIOPrimitive *                            iioPrimitive) 

{
  //  Initialize the IIOPrimitive
  if (!iioPrimitive) {
    fprintf(stderr, "Tried to extract to a NULL IIOPrimitive\n");
    return;
  }
  
  iioPrimitive->vertexCount = 0;
  iioPrimitive->vertices = NULL;
  iioPrimitive->indexCount = 0;
  iioPrimitive->indices = NULL;
  iioPrimitive->mode = 4; // Default to GL_TRIANGLES

  iioPrimitive->material.alphaCutoff = 0.5f; // Default alpha cutoff
  iioPrimitive->material.alphaMode = GLTF_AM_OPAQUE; // Default alpha mode
  iioPrimitive->material.doubleSided = false; // Default double-sided property
  
  memcpy(iioPrimitive->material.emissiveFactor, (float [3]) {0.0f, 0.0f, 0.0f}, sizeof(float) * 3);
  iioPrimitive->material.emissiveTexture.image = defaultRGBAImage; // Default emissive texture image
  iioPrimitive->material.emissiveTexture.imageMemory = defaultRGBAImageMemory; // Default emissive texture image memory
  iioPrimitive->material.emissiveTexture.imageView = defaultRGBAImageView; // Default emissive texture image view
  iioPrimitive->material.emissiveTexture.texCoord = 0; // Default emissive texture texCoord
  iioPrimitive->material.emissiveTexture.sampler = defaultSampler; // Default emissive texture sampler

  iioPrimitive->material.normalTexture.scale = 1.0f; // Default normal texture scale
  iioPrimitive->material.normalTexture.textureInfo.image = defaultNormalImage; // Default normal texture image
  iioPrimitive->material.normalTexture.textureInfo.imageMemory = defaultNormalImageMemory; // Default normal texture image memory
  iioPrimitive->material.normalTexture.textureInfo.imageView = defaultNormalImageView; // Default normal texture image view
  iioPrimitive->material.normalTexture.textureInfo.texCoord = 0; // Default normal texture texCoord
  iioPrimitive->material.normalTexture.textureInfo.sampler = defaultSampler; // Default normal texture sampler

  iioPrimitive->material.occlusionTexture.strength = 1.0f; // Default occlusion texture strength
  iioPrimitive->material.occlusionTexture.textureInfo.image = defaultRGBAImage; // Default occlusion texture image
  iioPrimitive->material.occlusionTexture.textureInfo.imageMemory = defaultRGBAImageMemory; // Default occlusion texture image memory
  iioPrimitive->material.occlusionTexture.textureInfo.imageView = defaultRGBAImageView; // Default occlusion texture image view
  iioPrimitive->material.occlusionTexture.textureInfo.texCoord = 0; // Default occlusion texture texCoord
  iioPrimitive->material.occlusionTexture.textureInfo.sampler = defaultSampler; // Default occlusion texture sampler

  memcpy(iioPrimitive->material.pbrMetallicRoughness.baseColorFactor, (float [4]) {1.0f, 1.0f, 1.0f, 1.0f}, sizeof(float) * 4);
  iioPrimitive->material.pbrMetallicRoughness.baseColorTextureInfo.image = defaultRGBAImage; // Default base color texture image
  iioPrimitive->material.pbrMetallicRoughness.baseColorTextureInfo.imageMemory = defaultRGBAImageMemory; // Default base color texture image memory
  iioPrimitive->material.pbrMetallicRoughness.baseColorTextureInfo.imageView = defaultRGBAImageView; // Default base color texture image view
  iioPrimitive->material.pbrMetallicRoughness.baseColorTextureInfo.sampler = defaultSampler; // Default base color texture sampler
  iioPrimitive->material.pbrMetallicRoughness.baseColorTextureInfo.texCoord = 0; // Default base color texture texCoord
  iioPrimitive->material.pbrMetallicRoughness.metallicFactor = 1.0f; // Default metallic factor
  iioPrimitive->material.pbrMetallicRoughness.metallicRoughnessTextureInfo.image = defaultRGBAImage; // Default metallic roughness texture image
  iioPrimitive->material.pbrMetallicRoughness.metallicRoughnessTextureInfo.imageMemory = defaultRGBAImageMemory; // Default metallic roughness texture image memory
  iioPrimitive->material.pbrMetallicRoughness.metallicRoughnessTextureInfo.imageView = defaultRGBAImageView; // Default metallic roughness texture image view
  iioPrimitive->material.pbrMetallicRoughness.metallicRoughnessTextureInfo.sampler = defaultSampler; // Default metallic roughness texture sampler
  iioPrimitive->material.pbrMetallicRoughness.metallicRoughnessTextureInfo.texCoord = 0; // Default metallic roughness texture texCoord
  iioPrimitive->material.pbrMetallicRoughness.roughnessFactor = 1.0f; // Default roughness factor

  
  if (!cgltfPrimitive) {
    fprintf(stderr, "Tried to extract from a NULL cgltfPrimitive\n");
    return;
  }

  //  Get the vertices [1]:required
  if (cgltfPrimitive->attributes_count == 0) {
    fprintf(stderr, "No attributes found in primitive\n");
    return;
  }
  cgltf_attribute * cgltfAttributes = cgltfPrimitive->attributes;
  cgltf_size cgltfAttributeCount = cgltfPrimitive->attributes_count;
  iio_extract_cgltf_vertices(cgltfAttributes, cgltfAttributeCount, &iioPrimitive->vertices, &iioPrimitive->vertexCount);
  if (iioPrimitive->vertexCount == 0 || !iioPrimitive->vertices) {
    fprintf(stderr, "No vertices found in primitive\n");
    return;
  }

  //  Get the indices [1]:optional
  if (cgltfPrimitive->indices) {
    iioPrimitive->indexCount = (uint32_t) cgltfPrimitive->indices->count;
    if (iioPrimitive->indexCount > 0) {
      iioPrimitive->indices = malloc(iioPrimitive->indexCount * sizeof(uint32_t));
      if (!iioPrimitive->indices) {
        fprintf(stderr, "Failed to allocate memory for primitive indices\n");
        return;
      }
      for (cgltf_size v = 0; v < cgltfPrimitive->indices->count; v++) {
        iioPrimitive->indices[v] = (uint32_t) cgltf_accessor_read_index(cgltfPrimitive->indices, v);
      }
    }
  }

  //  Get the material [1]:optional
  iio_extract_cgltf_material(cgltfPrimitive->material, &iioPrimitive->material);

  //  Get the mode [1]:optional; default:GL_TRIANGLES
  switch (cgltfPrimitive->type) {
    case cgltf_primitive_type_points:
      iioPrimitive->mode = 0; // GL_POINTS
      break;
    case cgltf_primitive_type_lines:
      iioPrimitive->mode = 1; // GL_LINES
      break;
    case cgltf_primitive_type_line_loop:
      iioPrimitive->mode = 2; // GL_LINE_LOOP
      break;
    case cgltf_primitive_type_line_strip:
      iioPrimitive->mode = 3; // GL_LINE_STRIP
      break;
    case cgltf_primitive_type_triangles:
      iioPrimitive->mode = 4; // GL_TRIANGLES
      break;
    case cgltf_primitive_type_triangle_strip:
      iioPrimitive->mode = 5; // GL_TRIANGLE_STRIP
      break;
    case cgltf_primitive_type_triangle_fan:
      iioPrimitive->mode = 6; // GL_TRIANGLE_FAN
      break;
    default:
      break;
  }

  //  TODO: handle targets
}

void iio_extract_cgltf_vertices(
  cgltf_attribute *                         cgltfAttributes, 
  cgltf_size                                cgltfAttributeCount, 
  IIOVertex **                              pVertices, 
  uint32_t *                                pVertexCount) 

{
  //  Check for NULL pointers and empty attributes
  if (!cgltfAttributes) {
    fprintf(stderr, "Tried to extract vertices from a NULL cgltfAttributes\n");
    *pVertices = NULL;
    *pVertexCount = 0;
    return;
  }
  if (cgltfAttributeCount == 0) {
    fprintf(stderr, "No attributes found in cgltfAttributes\n");
    *pVertices = NULL;
    *pVertexCount = 0;
    return;
  }
  if (!pVertices || !pVertexCount) {
    fprintf(stderr, "Tried to extract vertices to a NULL pointer\n");
    return;
  }

  //  get the vertex count and allocate memory for the vertices
  *pVertexCount = cgltfAttributes[0].data->count;
  *pVertices = malloc(*pVertexCount * sizeof(IIOVertex));
  if (!*pVertices) {
    fprintf(stderr, "Failed to allocate memory for IIOVertex array\n");
    *pVertexCount = 0;
    return;
  }

  //  
  cgltf_attribute * attribute = cgltfAttributes;
  IIOVertex * vertices = *pVertices;
  for (int i = 0; i < cgltfAttributeCount; i++) {
    cgltf_accessor * accessor = attribute->data;
    if        (attribute->type == cgltf_attribute_type_position) {
      for (int v = 0; v < accessor->count; v++) {
        cgltf_accessor_read_float(accessor, v, vertices[v].position, 3);
      }
    } else if (attribute->type == cgltf_attribute_type_normal) {
      for (int v = 0; v < accessor->count; v++) {
        cgltf_accessor_read_float(accessor, v, vertices[v].normal, 3);
      }
    } else if (attribute->type == cgltf_attribute_type_tangent) {
      for (int v = 0; v < accessor->count; v++) {
        cgltf_accessor_read_float(accessor, v, vertices[v].tangent, 4);
      }
    } else if (attribute->type == cgltf_attribute_type_texcoord) {
      for (int v = 0; v < accessor->count; v++) {
        if (attribute->index >= 2) continue; // Only handle texcoord 0 and 1
        cgltf_accessor_read_float(accessor, v, vertices[v].texCoord[attribute->index], 2);
      }
    } else if (attribute->type == cgltf_attribute_type_color) {
      for (int v = 0; v < accessor->count; v++) {
        if (attribute->index >= 1) continue; // Only handle color 0
        cgltf_size element_size = accessor->type == cgltf_type_vec4 ? 4 : 3;
        cgltf_accessor_read_float(accessor, v, vertices[v].color, element_size);
        if (element_size == 3) vertices[v].color[3] = 1.0f; // Set alpha to 1.0 if not present
      }
    } else if (attribute->type == cgltf_attribute_type_joints) {
      for (int v = 0; v < accessor->count; v++) {
        if (attribute->index >= 1) continue; // Only handle joints 0
        cgltf_accessor_read_float(accessor, v, vertices[v].joints, 4);
      }
    } else if (attribute->type == cgltf_attribute_type_weights) {
      for (int v = 0; v < accessor->count; v++) {
        if (attribute->index >= 1) continue; // Only handle weights 0
        cgltf_accessor_read_float(accessor, v, vertices[v].weights, 4);
      }
    } else {
      fprintf(stderr, "Unknown attribute type: %d\n", attribute->type);
    }
    attribute++;
  }
}

void iio_extract_cgltf_material(
  cgltf_material *                          cgltfMaterial, 
  IIOMaterial *                             iioMaterial) 

{
  if (!cgltfMaterial) {
    return;
  }
  if (!iioMaterial) {
    fprintf(stderr, "Tried to extract to a NULL IIOMaterial\n");
    return;
  }

  //  Get PBR Metallic Roughness
  if (cgltfMaterial->has_pbr_metallic_roughness) {
    //  Base Color Factor float[4]:optional; default:{1.0f, 1.0f, 1.0f, 1.0f}
    memcpy(iioMaterial->pbrMetallicRoughness.baseColorFactor, cgltfMaterial->pbr_metallic_roughness.base_color_factor, sizeof(float) * 4);

    //  Base Color Texture texture[1]:optional; default:RGBA, {1.0f, 1.0f, 1.0f, 1.0f}
    iio_extract_cgltf_texture(
      cgltfMaterial->pbr_metallic_roughness.base_color_texture.texture,
      &iioMaterial->pbrMetallicRoughness.baseColorTextureInfo.image,
      &iioMaterial->pbrMetallicRoughness.baseColorTextureInfo.imageMemory,
      &iioMaterial->pbrMetallicRoughness.baseColorTextureInfo.imageView
    );
    iioMaterial->pbrMetallicRoughness.baseColorTextureInfo.texCoord = cgltfMaterial->pbr_metallic_roughness.base_color_texture.texcoord;

    //  Metallic Factor float[1]:optional; default:1.0f
    iioMaterial->pbrMetallicRoughness.metallicFactor = cgltfMaterial->pbr_metallic_roughness.metallic_factor;

    //  Roughness Factor float[1]:optional; default:1.0f
    iioMaterial->pbrMetallicRoughness.roughnessFactor = cgltfMaterial->pbr_metallic_roughness.roughness_factor;

    //  Metallic Roughness Texture texture[1]:optional; default:RGBA, {Xf, 1.0f, 1.0f, Xf}
    iio_extract_cgltf_texture(
      cgltfMaterial->pbr_metallic_roughness.metallic_roughness_texture.texture,
      &iioMaterial->pbrMetallicRoughness.metallicRoughnessTextureInfo.image,
      &iioMaterial->pbrMetallicRoughness.metallicRoughnessTextureInfo.imageMemory,
      &iioMaterial->pbrMetallicRoughness.metallicRoughnessTextureInfo.imageView
    );
    iioMaterial->pbrMetallicRoughness.metallicRoughnessTextureInfo.texCoord = cgltfMaterial->pbr_metallic_roughness.metallic_roughness_texture.texcoord;
  }

  //  Get Normal Texture
  if (cgltfMaterial->normal_texture.texture) {
    //  Normal Texture texture[1]:optional; default:RGBA, {0.0f, 0.0f, 1.0f, 1.0f}
    iio_extract_cgltf_texture(
      cgltfMaterial->normal_texture.texture,
      &iioMaterial->normalTexture.textureInfo.image,
      &iioMaterial->normalTexture.textureInfo.imageMemory,
      &iioMaterial->normalTexture.textureInfo.imageView
    );
    iioMaterial->normalTexture.textureInfo.texCoord = cgltfMaterial->normal_texture.texcoord;
    iioMaterial->normalTexture.scale = cgltfMaterial->normal_texture.scale;
  }

  //  Get Occlusion Texture
  if (cgltfMaterial->occlusion_texture.texture) {
    //  Occlusion Texture texture[1]:optional; default:RGBA, {1.0f, 1.0f, 1.0f, 1.0f}
    iio_extract_cgltf_texture(
      cgltfMaterial->occlusion_texture.texture,
      &iioMaterial->occlusionTexture.textureInfo.image,
      &iioMaterial->occlusionTexture.textureInfo.imageMemory,
      &iioMaterial->occlusionTexture.textureInfo.imageView
    );
    iioMaterial->occlusionTexture.textureInfo.texCoord = cgltfMaterial->occlusion_texture.texcoord;
    iioMaterial->occlusionTexture.strength = cgltfMaterial->occlusion_texture.scale;
  }

  //  Get Emissive Texture
  if (cgltfMaterial->emissive_texture.texture) {
    //  Emissive Texture texture[1]:optional; default:RGBA, {1.0f, 1.0f, 1.0f, 1.0f}
    iio_extract_cgltf_texture(
      cgltfMaterial->emissive_texture.texture,
      &iioMaterial->emissiveTexture.image,
      &iioMaterial->emissiveTexture.imageMemory,
      &iioMaterial->emissiveTexture.imageView
    );
    iioMaterial->emissiveTexture.texCoord = cgltfMaterial->emissive_texture.texcoord;
  }

  //  Get Emissive Factor
  memcpy(iioMaterial->emissiveFactor, cgltfMaterial->emissive_factor, sizeof(float) * 3);

  //  Get Alpha Mode
  switch(cgltfMaterial->alpha_mode) {
    case cgltf_alpha_mode_opaque:
      iioMaterial->alphaMode = GLTF_AM_OPAQUE;
      break;
    case cgltf_alpha_mode_mask:
      iioMaterial->alphaMode = GLTF_AM_MASK;
      break;
    case cgltf_alpha_mode_blend:
      iioMaterial->alphaMode = GLTF_AM_BLEND;
      break;
    default:
      iioMaterial->alphaMode = GLTF_AM_UNDEFINED;
      break;
  }

  //  Get Alpha Cutoff
  iioMaterial->alphaCutoff = cgltfMaterial->alpha_cutoff;

  //  Get Double Sided
  iioMaterial->doubleSided = cgltfMaterial->double_sided;
}

void iio_extract_cgltf_texture(
  cgltf_texture *                           cgltfTexture, 
  VkImage *                                 image, 
  VkDeviceMemory *                          imageMemory, 
  VkImageView *                             imageView) 

{
  if (!image || !imageMemory || !imageView) {
    fprintf(stderr, "Tried to extract texture to NULL pointers\n");
    return;
  }
  if (!cgltfTexture) {
    return; // No texture provided, use default
  }
  if (!cgltfTexture->image) {
    return; // No image in texture, use default
  }
  if (cgltfTexture->image->uri) {
    char * uri = cgltfTexture->image->uri;
    char buffer [256];
    buffer[255] = 0;
    int len = snprintf(buffer, 255, "%s%s", IIO_PATH_TO_TEXTURES, uri);
    if (len < 0 || len >= sizeof(buffer)) {
      fprintf(stderr, "Failed to create texture path for %s\n", uri);
      return;
    }
    iioCreateTextureImageFunc(buffer, image, imageMemory, imageView);
  } else if (cgltfTexture->image->buffer_view) {
    if (!cgltfTexture->image->buffer_view->data) {
      fprintf(stderr, "missing data in bufferview\n");
      return;
    }
    iioCreateTextureImageFromMemoryFunc(
      cgltfTexture->image->buffer_view->data,
      cgltfTexture->image->buffer_view->size,
      image,
      imageMemory,
      imageView
    );
  } else {
    fprintf(stderr, "No uri or buffer view present in texture %p", cgltfTexture);
  }
}

/**
 *   Cleanup   *
 */

void iio_destroy_resources(
  VkDevice                                  device) 

{
  if (!device) {
    fprintf(stderr, "Tried to destroy resources with a NULL device\n");
    return;
  }
  if (defaultRGBAImageView) vkDestroyImageView(device, defaultRGBAImageView, NULL);
  if (defaultRGBAImage) vkDestroyImage(device, defaultRGBAImage, NULL);
  if (defaultRGBAImageMemory) vkFreeMemory(device, defaultRGBAImageMemory, NULL);
  if (defaultNormalImageView) vkDestroyImageView(device, defaultNormalImageView, NULL);
  if (defaultNormalImage) vkDestroyImage(device, defaultNormalImage, NULL);
  if (defaultNormalImageMemory) vkFreeMemory(device, defaultNormalImageMemory, NULL);
  if (defaultSampler) vkDestroySampler(device, defaultSampler, NULL);
}

void iio_destroy_model(
  IIOModel *                                model) 

{
  if (!model) return;
  for (uint32_t i = 0; i < model->meshCount; i++) {
    IIOMesh * mesh = &model->meshes[i];
    for (uint32_t j = 0; j < mesh->primitiveCount; j++) {
      IIOPrimitive * primitive = &mesh->primitives[j];
      free(primitive->vertices);
      free(primitive->indices);
    }
    free(mesh->primitives);
  }
  free(model->meshes);
}

void iio_destroy_image(
  VkDevice                                  device, 
  const char *                              name, 
  IIOResourceManager *                      manager) 

{
  if (hmap_strImg_contains(&manager->imageMap, name)) {
    IIOImageHandle * image = hmap_strImg_at_mut(&manager->imageMap, name);
    vkDestroyImage(device, image->data, NULL);
    vkDestroyImageView(device, image->view, NULL);
    vkDestroySampler(device, image->sampler, NULL);
    vkFreeMemory(device, image->memory, NULL);
    hmap_strImg_erase(&manager->imageMap, name);
  } else {
    fprintf(stderr, "iio_destroy_image : Image with name %s was not found\n", name);
  }
}

void iio_destroy_resource_manager(
  IIOResourceManager *                      manager) 

{

}

/**
 * Debug *
 */

void iio_load_test_model() {
  IIOModel model = {0};
  cgltf_options options = {0};
  cgltf_data * data = NULL;
  cgltf_result result = cgltf_parse_file(&options, "resources/models/Buggy.gltf", &data);
  if (result != cgltf_result_success) {
    fprintf(stderr, "Failed to parse test model file: %s\n", testModelPath);
    return;
  }
  result = cgltf_load_buffers(&options, data, "resources/models/Buggy.gltf");
  if (result != cgltf_result_success) {
    fprintf(stderr, "Failed to load buffers for test model file: %s\n", testModelPath);
    cgltf_free(data);
    return;
  }

  for (int i = 0; i < data->meshes_count; i++) {
    cgltf_mesh * mesh = &data->meshes[i];
    printf("Mesh %d: %s\n", i, mesh->name ? mesh->name : "Unnamed");
    printf("  Primitives Count: %zu\n", mesh->primitives_count);
    for (cgltf_size j = 0; j < mesh->primitives_count; j++) {
      cgltf_primitive * primitive = &mesh->primitives[j];
      printf("  Primitive %zu:\n", j);
      printf("    Attributes Count: %zu\n", primitive->attributes_count);
      for (cgltf_size k = 0; k < primitive->attributes_count; k++) {
        cgltf_attribute * attribute = &primitive->attributes[k];
        printf("    Attribute %zu: Type: %s\n", k, 
               attribute->type == cgltf_attribute_type_position ? "Position" :
               attribute->type == cgltf_attribute_type_normal ? "Normal" :
               attribute->type == cgltf_attribute_type_tangent ? "Tangent" :
               attribute->type == cgltf_attribute_type_texcoord ? "TexCoord" :
               attribute->type == cgltf_attribute_type_color ? "Color" :
               attribute->type == cgltf_attribute_type_joints ? "Joints" :
               attribute->type == cgltf_attribute_type_weights ? "Weights" : "Unknown");
      }
    }
  }

  cgltf_free(data);
}

#ifdef IIO_RESOURCE_LOADERS_TEST
int main(int argc, char * argv[]) {
  iio_initialize_default_texture_resources();
  iio_load_test_model();
  iio_destroy_resources(VK_NULL_HANDLE); // Replace with actual Vulkan device handle if available
  return 0;
}
#endif