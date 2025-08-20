#ifndef IIO_DESCRIPTORS_H
#define IIO_DESCRIPTORS_H

#include <vulkan/vulkan.h>

#define IIO_MAX_SETS 4092

#define T deque_Pool, VkDescriptorPool
#include "stc/deque.h"

#define T vec_Write, VkWriteDescriptorSet
#include "stc/vec.h"

#define T hmap_Di, VkDescriptorType, int
#include "stc/hmap.h"

#define T vec_DBI, VkDescriptorBufferInfo
#include "stc/vec.h"

#define T vec_DII, VkDescriptorImageInfo
#include "stc/vec.h"

typedef struct IIODescriptorLayoutElement_S {
  VkDescriptorType                type;
  uint32_t                        count;
  VkShaderStageFlags              stageFlags; // Shader stages this descriptor is used in
} IIODescriptorLayoutElement;

#define T vec_DLE, IIODescriptorLayoutElement
#include "stc/vec.h"

typedef struct IIODescriptorPoolManager_S {
  bool                            isInitialized;

  vec_DLE                         iioDescriptorLayoutElements; // Array of descriptor set ratios
  VkDescriptorSetLayout           descriptorSetLayout; // Descriptor set layout
  
  deque_Pool                      readyPools; // Array of descriptor pools
  deque_Pool                      usedPools; // Array of used descriptor pools

  size_t                          setsPerPool; // Number of sets per pool

  vec_Write                       writes;
} IIODescriptorPoolManager;

typedef enum {
  iio_writer_type_buffer,
  iio_writer_type_image
} IIOWriterType;

typedef union {
  VkDescriptorBufferInfo          bufferInfo;
  VkDescriptorImageInfo           imageInfo;
} IIODescriptorWriteObjectInfo;

typedef struct IIODescriptorWriteInfo_S {
  IIOWriterType                   type;
  IIODescriptorWriteObjectInfo    objectInfo;
} IIODescriptorWriteInfo;

#define T vec_WriteInfo, IIODescriptorWriteInfo
#include "stc/vec.h"

typedef struct IIODescriptorSetWriter_S {
  vec_Write                       writes;
  vec_WriteInfo                   writeInfos;
} IIODescriptorSetWriter;

void iio_create_descriptor_pool_manager(
  VkDevice                        device, 
  size_t                          elementCount, 
  IIODescriptorLayoutElement *    elements, 
  size_t                          framesInFlight,
  size_t                          initialSetCount, 
  IIODescriptorPoolManager *      manager);

void iio_create_descriptor_set_layout(
  VkDevice                        device, 
  IIODescriptorPoolManager *      manager);

VkDescriptorPool iio_create_descriptor_pool(
  VkDevice                        device, 
  IIODescriptorPoolManager *      manager);

VkDescriptorPool iio_get_descriptor_pool(
  VkDevice                        device, 
  IIODescriptorPoolManager *      manager);

VkDescriptorSet iio_allocate_descriptor_set(
  VkDevice                        device, 
  IIODescriptorPoolManager *      manager);

void iio_clear_descriptor_pools(
  VkDevice                        device,
  IIODescriptorPoolManager *      manager);

void iio_destroy_descriptor_pools(
  VkDevice                        device,
  IIODescriptorPoolManager *      manager);
  
void iio_destroy_descriptor_pool_manager(
  VkDevice                        device,
  IIODescriptorPoolManager *      manager);

void iio_create_descriptor_set_writer(
  IIODescriptorSetWriter *        writer);

void iio_write_image_descriptor(
  uint32_t                        binding, 
  uint32_t                        descriptorCount, 
  VkDescriptorType                descriptorType, 
  VkSampler                       sampler, 
  VkImageView                     imageView, 
  VkImageLayout                   imageLayout, 
  IIODescriptorSetWriter *        writer);

void iio_write_buffer_descriptor(
  uint32_t                        binding,
  uint32_t                        descriptorCount,
  VkDescriptorType                descriptorType,
  VkBuffer                        buffer,
  uint32_t                        offset,
  uint32_t                        range,
  IIODescriptorSetWriter *        writer);

void iio_update_set(
  VkDevice                        device,
  VkDescriptorSet                 descriptorSet,
  IIODescriptorSetWriter *        writer);

void iio_clear_descriptor_set_writer(
  IIODescriptorSetWriter *        writer);

void iio_destroy_descriptor_set_writer(
  IIODescriptorSetWriter *        writer);
#endif