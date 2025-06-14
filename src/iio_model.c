#include <stdlib.h>
#include <stdio.h>

#include <vulkan/vulkan.h>
#include "cgltf.h"
#include "cglm/cglm.h"

#include "iio_model.h"
#include "iio_eng_typedef.h"

/**
 *   constants
 */

const char * testModelPath = "resources/models/Buggy.gltf";

const vec4 pbrMetallicRoughnessBaseColor_DEFAULT = {1.0f, 1.0f, 1.0f, 1.0f};

const vec3 pbrMetallicRoughness_DEFAULT = {1.0f, 1.0f, 1.0f};

const float pbrOcclusion_DEFAULT [1] = {1.0f};

const vec3 pbrEmissive_DEFAULT = {1.0f, 1.0f, 1.0f};

/**
 *   descriptors   *
 */

static VkVertexInputBindingDescription iio_get_iiovertex_binding_description() {
  VkVertexInputBindingDescription bindingDescription = {0};

  bindingDescription.binding = 0; // Binding index
  bindingDescription.stride = sizeof(IIOVertex); // Size of each vertex
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Each vertex is a separate instance

  return bindingDescription;
}

static VkVertexInputAttributeDescription * iio_get_iiovertex_attribute_descriptions(int * count) {
  *count = IIOVERTEX_ATTRIBUTE_COUNT;
  static VkVertexInputAttributeDescription attributeDescriptions[IIOVERTEX_ATTRIBUTE_COUNT];

  attributeDescriptions[0].binding = 0; // Binding index
  attributeDescriptions[0].location = 0; // Location in the shader
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

/*****************
 *    Loaders    *
 *****************/

void iio_load_model(const char * path, IIOModel * model) {
  cgltf_options options = {0};
  cgltf_data * data = NULL;
  cgltf_result result = cgltf_parse_file(&options, path, &data);
  if (result != cgltf_result_success) {
    fprintf(stderr, "Failed to parse model file: %s\n", path);
    exit(1);
  }
  result = cgltf_load_buffers(&options, data, path);
  if (result != cgltf_result_success) {
    fprintf(stderr, "Failed to load buffers for model file: %s\n", path);
    cgltf_free(data);
    exit(1);
  }

  model->meshCount = (uint32_t) data->meshes_count;
  model->meshes = malloc(model->meshCount * sizeof(IIOMesh));
  if (!model->meshes) {
    fprintf(stderr, "Failed to allocate memory for model meshes\n");
    cgltf_free(data);
    exit(1);  //  TODO: handle this better
  }
  //  Iterate through the meshes
  for (int i = 0; i < data->meshes_count; i++) {
    cgltf_mesh * mesh = &data->meshes[i];
    IIOMesh * iioMesh = &model->meshes[i];

    iioMesh->primitiveCount = (uint32_t) mesh->primitives_count;
    iioMesh->primitives = malloc(model->meshes[i].primitiveCount * sizeof(IIOPrimitive));
    if (!iioMesh->primitives) {
      fprintf(stderr, "Failed to allocate memory for mesh primitives\n");
      cgltf_free(data);
      exit(1);  //  TODO: handle this better
    }
    //  Iterate through the primitives
    for (cgltf_size j = 0; j < mesh->primitives_count; j++) {
      cgltf_primitive * primitive = &mesh->primitives[j];
      IIOPrimitive * iioPrimitive = &iioMesh->primitives[j];

      //  Get the vertices
      iioPrimitive->vertexCount = (uint32_t) primitive->attributes[0].data->count;
      iioPrimitive->vertices = malloc(iioPrimitive->vertexCount * sizeof(IIOVertex));
      if (!iioPrimitive->vertices) {
        fprintf(stderr, "Failed to allocate memory for primitive vertices\n");
        cgltf_free(data);
        exit(1);  //  TODO: handle this better
      }
      IIOVertex * vertices = iioPrimitive->vertices;
      //  Iterate through the attributes
      for (cgltf_size k = 0; k < primitive->attributes_count; k++) {
        cgltf_attribute * attribute = &primitive->attributes[k];
        cgltf_accessor * accessor = attribute->data;
        if        (attribute->type == cgltf_attribute_type_position) {
          for (cgltf_size v = 0; v < accessor->count; v++) {
            cgltf_accessor_read_float(accessor, v, vertices[v].position, 3);
          }
        } else if (attribute->type == cgltf_attribute_type_normal) {
          for (cgltf_size v = 0; v < accessor->count; v++) {
            cgltf_accessor_read_float(accessor, v, vertices[v].normal, 3);
          }
        } else if (attribute->type == cgltf_attribute_type_tangent) {
          for (cgltf_size v = 0; v < accessor->count; v++) {
            cgltf_accessor_read_float(accessor, v, vertices[v].tangent, 4);
          }
        } else if (attribute->type == cgltf_attribute_type_texcoord) {
          if (attribute->index >= 2) continue;
          for (cgltf_size v = 0; v < accessor->count; v++) {
            cgltf_accessor_read_float(accessor, v, vertices[v].texCoord[attribute->index], 2);
          }
        } else if (attribute->type == cgltf_attribute_type_color) {
          if (attribute->index >= 1) continue;
          cgltf_size element_size = accessor->type == cgltf_type_vec4 ? 4 : 3;
          for (cgltf_size v = 0; v < accessor->count; v++) {
            cgltf_accessor_read_float(accessor, v, vertices[v].color, element_size);
            if (element_size == 3) vertices[v].color[3] = 1.0f;
          }
        } else if (attribute->type == cgltf_attribute_type_joints) {
          if (attribute->index >= 1) continue;
          for (cgltf_size v = 0; v < accessor->count; v++) {
            cgltf_accessor_read_float(accessor, v, vertices[v].joints, 4);
          }
        } else if (attribute->type == cgltf_attribute_type_weights) {
          if (attribute->index >= 1) continue;
          for (cgltf_size v = 0; v < accessor->count; v++) {
            cgltf_accessor_read_float(accessor, v, vertices[v].weights, 4);
          }
        }
      }

      //  Get the indices
      iioPrimitive->indexCount = (uint32_t) primitive->indices->count;
      if (iioPrimitive->indexCount > 0) {
        iioPrimitive->indices = malloc(iioPrimitive->indexCount * sizeof(uint32_t));
        if (!iioPrimitive->indices) {
          fprintf(stderr, "Failed to allocate memory for primitive indices\n");
          cgltf_free(data);
          exit(1);  //  TODO: handle this better
        }
        for (cgltf_size v = 0; v < primitive->indices->count; v++) {
          iioPrimitive->indices[v] = (uint32_t) cgltf_accessor_read_index(primitive->indices, v);
        }
      } else {
        iioPrimitive->indices = NULL;
      }

      //  TODO: handle material
      

      //  Set the mode
      iioPrimitive->mode = primitive->type == cgltf_primitive_type_invalid ? 4 : primitive->type; // Default to GL_TRIANGLES

      //  TODO: handle targets
    }
  }
  
  cgltf_free(data);
}

/**
 *   Cleanup   *
 */

void iio_destroy_model(IIOModel * model) {
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

/**
 * Debug *
 */

void iio_load_test_model() {
  IIOModel model = {0};
  iio_load_model(testModelPath, &model);
  for (int i = 0; i < model.meshCount; i++) {
    IIOMesh mesh = model.meshes[i];
    fprintf(stdout, "Mesh %d has %d primitives\n", i, mesh.primitiveCount);
    for (int j = 0; j < mesh.primitiveCount; j++) {
      IIOPrimitive primitive = mesh.primitives[j];
      fprintf(stdout, "\tPrimitive %d has %d vertices\n", j, primitive.vertexCount);
      fprintf(stdout, "\t             and %d indices\n", primitive.vertexCount);
    }
  }
}