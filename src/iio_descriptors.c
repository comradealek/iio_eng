#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include "iio_descriptors.h"
#include "iio_eng_typedef.h"
#include "iio_eng_errors.h"

/*************************************
 * descriptor pool manager functions *
 *************************************/

void iio_create_descriptor_pool_manager(
  VkDevice                        device, 
  size_t                          elementCount, 
  IIODescriptorLayoutElement *    elements, 
  size_t                          framesInFlight, 
  size_t                          initialSetCount, 
  IIODescriptorPoolManager *      manager) 
  
{
  if (!device) {
    fprintf(stderr, "Tried to create descriptor pool manager with a NULL device\n");
    return;
  } else if (!elements || elementCount == 0) {
    fprintf(stderr, "Tried to create descriptor pool manager with NULL ratios or zero ratio count\n");
    return;
  } else if(!initialSetCount) {
    fprintf(stderr, "Tried to create descriptor pool manager with zero initial set count\n");
    return;
  } else if (!manager) {
    fprintf(stderr, "Tried to return to a NULL IIODescriptorPoolManager pointer\n");
    return;
  }

  //  Initialize the manager
  memset(manager, 0, sizeof(IIODescriptorPoolManager));

  manager->isInitialized = true;
  
  //  Initialize the arrays
  manager->iioDescriptorLayoutElements = vec_DLE_init();
  manager->readyPools = deque_Pool_init();
  manager->usedPools = deque_Pool_init();
  manager->writes = vec_Write_init();

  //  Set the initial sets per pool
  manager->setsPerPool = initialSetCount;

  //  Push the elements into the iioDescriptorLayoutElements vector
  for (size_t i = 0; i < elementCount; i++) {
    vec_DLE_push(&manager->iioDescriptorLayoutElements, elements[i]);
  }

  //  Set the desciptor set layout
  iio_create_descriptor_set_layout(device, manager);
  if (manager->descriptorSetLayout == VK_NULL_HANDLE) {
    fprintf(stderr, "Failed to create descriptor set layout for descriptor pool manager\n");
    iio_destroy_descriptor_pool_manager(device, manager);
    return;
  }

  //  Create the initial descriptor pool and push it to the deque
  VkDescriptorPool pool = iio_create_descriptor_pool(device, manager);
  if (pool == VK_NULL_HANDLE) {
    fprintf(stderr, "Failed to create initial descriptor pool for descriptor pool manager\n");
    iio_destroy_descriptor_pool_manager(device, manager);
    return;
  }
  deque_Pool_push_back(&manager->readyPools, pool);
}

void iio_create_descriptor_set_layout(
  VkDevice                        device, 
  IIODescriptorPoolManager *      manager) 

{
  if (!device) {
    fprintf(stderr, "Tried to create descriptor set layout with a NULL device\n");
    return;
  } else if (!manager) {
    fprintf(stderr, "Tried to create descriptor set layout with a NULL manager\n");
    return;
  } else if (vec_DLE_is_empty(&manager->iioDescriptorLayoutElements)) {
    fprintf(stderr, "empty vec_DLE\n");
    return;
  }

  //  Create the descriptor set layout based on the iioDescriptorLayoutElements vector
  size_t ratioCount = vec_DLE_size(&manager->iioDescriptorLayoutElements);
  VkDescriptorSetLayoutBinding bindings [ratioCount];
  for (size_t i = 0; i < ratioCount; i++) {
    const IIODescriptorLayoutElement * elem = vec_DLE_at(&manager->iioDescriptorLayoutElements, i);
    bindings[i].binding = (uint32_t) i;
    bindings[i].descriptorType = elem->type;
    bindings[i].descriptorCount = elem->count;
    bindings[i].stageFlags = elem->stageFlags;
    bindings[i].pImmutableSamplers = NULL; // No immutable samplers for now
  }

  VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {0};
  layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutCreateInfo.pNext = NULL;
  layoutCreateInfo.flags = 0;
  layoutCreateInfo.bindingCount = (uint32_t) ratioCount;
  layoutCreateInfo.pBindings = bindings;

  vkCreateDescriptorSetLayout(device, &layoutCreateInfo, NULL, &manager->descriptorSetLayout);
  if (manager->descriptorSetLayout == VK_NULL_HANDLE) {
    fprintf(stderr, "Failed to create descriptor set layout\n");
    return;
  }
}

VkDescriptorPool iio_create_descriptor_pool(
  VkDevice                        device, 
  IIODescriptorPoolManager *      manager) 
  
{
  VkDescriptorPool newPool = {0};
  if (!device) {
    fprintf(stderr, "Tried to create descriptor pool with a NULL device\n");
    return newPool;
  } else if (!manager) {
    fprintf(stderr, "Tried to create descriptor pool with a NULL manager\n");
    return newPool;
  } else if (vec_DLE_is_empty(&manager->iioDescriptorLayoutElements)) {
    fprintf(stderr, "Uninitialized descriptor pool manager\n");
    return newPool;
  }

  //  Count the number of each descriptor type needed for the pool
  hmap_Di poolRatios = hmap_Di_init();
  
  for (size_t i = 0; i < vec_DLE_size(&manager->iioDescriptorLayoutElements); i++) {
    const IIODescriptorLayoutElement * elem = vec_DLE_at(&manager->iioDescriptorLayoutElements, i);
    bool exists = hmap_Di_contains(&poolRatios, elem->type);
    if (exists) {
      *hmap_Di_at_mut(&poolRatios, elem->type) += elem->count;
    } else {
      hmap_Di_insert(&poolRatios, elem->type, elem->count);
    }
  }

  //  Create the pool sizes array
  size_t poolSizeCount = hmap_Di_size(&poolRatios);
  VkDescriptorPoolSize poolSizes [poolSizeCount];
  size_t i = 0;
  for (c_each_kv(key, value, hmap_Di, poolRatios)) {
    poolSizes[i].type = *key;
    poolSizes[i].descriptorCount = (uint32_t) *value * manager->setsPerPool;
    i++;
  }

  //  Create the descriptor pool
  VkDescriptorPoolCreateInfo poolCreateInfo = {0};
  poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolCreateInfo.pNext = NULL;
  poolCreateInfo.flags = 0;
  poolCreateInfo.maxSets = (uint32_t) (manager->setsPerPool);
  poolCreateInfo.poolSizeCount = (uint32_t) poolSizeCount;
  poolCreateInfo.pPoolSizes = poolSizes;

  VkResult result = vkCreateDescriptorPool(device, &poolCreateInfo, NULL, &newPool);
  if (result != VK_SUCCESS) {
    iio_vk_error(result, __LINE__, __FILE__);
    fprintf(stderr, "Failed to create descriptor pool\n");
  }

  manager->setsPerPool = manager->setsPerPool * 2 > IIO_MAX_SETS ? IIO_MAX_SETS : manager->setsPerPool * 2;

  hmap_Di_drop(&poolRatios);

  return newPool;
}

VkDescriptorPool iio_get_descriptor_pool(
  VkDevice                        device, 
  IIODescriptorPoolManager *      manager) 

{
  VkDescriptorPool getPool = VK_NULL_HANDLE;
  if (deque_Pool_is_empty(&manager->readyPools)) {
    getPool = iio_create_descriptor_pool(device, manager);
  } else {
    getPool = deque_Pool_pull_back(&manager->readyPools);
  }
  return getPool;
}

VkDescriptorSet iio_allocate_descriptor_set(
  VkDevice                        device, 
  IIODescriptorPoolManager *      manager) 

{
  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

  //  Get a descriptor pool to allocate from
  VkDescriptorPool descriptorPool = iio_get_descriptor_pool(device, manager);

  //  Allocate the descriptor set
  VkDescriptorSetAllocateInfo allocateInfo = {0};
  allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocateInfo.pNext = NULL;
  allocateInfo.pSetLayouts = &manager->descriptorSetLayout;
  allocateInfo.descriptorPool = descriptorPool;
  allocateInfo.descriptorSetCount = 1;
  
  VkResult result = vkAllocateDescriptorSets(device, &allocateInfo, &descriptorSet);
  if (result == VK_SUCCESS) {
    goto lbl_success;
  } else if (result != VK_ERROR_OUT_OF_POOL_MEMORY && result != VK_ERROR_FRAGMENTED_POOL) {
    return VK_NULL_HANDLE;
  }

  //  If we run out of memory in a descriptor pool, get another one and try again
  deque_Pool_push_back(&manager->usedPools, descriptorPool);
  descriptorPool = iio_get_descriptor_pool(device, manager);

  allocateInfo.descriptorPool = descriptorPool;

  result = vkAllocateDescriptorSets(device, &allocateInfo, &descriptorSet);

  //  If it fails, then print an error message and return a VK_NULL_HANDLE
  if (result != VK_SUCCESS) {
    fprintf(stdout, "Failed to allocate a descriptor set\n");
    return VK_NULL_HANDLE;
  }

lbl_success:
  fprintf(stdout, "descriptor set 0x%x allocated to descriptor pool 0x%x\n", descriptorSet, descriptorPool);
  deque_Pool_push_back(&manager->readyPools, descriptorPool);
  return descriptorSet;
}

void iio_clear_descriptor_pools(
  VkDevice                        device,
  IIODescriptorPoolManager *      manager) 

{
  for (uint32_t i = 0; i < deque_Pool_size(&manager->readyPools); i++) {
    VkDescriptorPool * descriptorPool = deque_Pool_at_mut(&manager->readyPools, i);
    vkResetDescriptorPool(device, *descriptorPool, 0);
  }
  for (uint32_t i = 0; i < deque_Pool_size(&manager->usedPools); i++) {
    VkDescriptorPool * descriptorPool = deque_Pool_at_mut(&manager->usedPools, i);
    vkResetDescriptorPool(device, *descriptorPool, 0);
    deque_Pool_push_back(&manager->readyPools, *descriptorPool);
  }
  deque_Pool_clear(&manager->usedPools);
};

void iio_destroy_descriptor_pools(
  VkDevice                        device,
  IIODescriptorPoolManager *      manager) 

{
  fprintf(stdout, "destroying ready descriptor pools (is empty: %u)\n", deque_Pool_is_empty(&manager->readyPools));
  for (uint32_t i = 0; i < deque_Pool_size(&manager->readyPools); i++) {
    VkDescriptorPool * descriptorPool = deque_Pool_at_mut(&manager->readyPools, i);
    fprintf(stdout, "destroying descriptor pool 0x%x\n", *descriptorPool);
    vkDestroyDescriptorPool(device, *descriptorPool, 0);
  }
  deque_Pool_clear(&manager->readyPools);
  fprintf(stdout, "destorying used descriptor pools (is empty: %u)\n", deque_Pool_is_empty(&manager->usedPools));
  for (uint32_t i = 0; i < deque_Pool_size(&manager->usedPools); i++) {
    VkDescriptorPool * descriptorPool = deque_Pool_at_mut(&manager->usedPools, i);
    fprintf(stdout, "destroying descriptor pool 0x%x\n", *descriptorPool);
    vkDestroyDescriptorPool(device, *descriptorPool, 0);
  }
  deque_Pool_clear(&manager->usedPools);
}

void iio_destroy_descriptor_pool_manager(
  VkDevice                        device, 
  IIODescriptorPoolManager *      manager) 
  
{
  fprintf(stdout, "Destroying descriptor pool manager at %p\n", manager);
  if (!device) {
    fprintf(stderr, "Tried to destroy descriptor pool manager with a NULL device\n");
    return;
  } else if (!manager) {
    fprintf(stderr, "Tried to destroy NULL descriptor pool manager\n");
    return;
  }

  iio_destroy_descriptor_pools(device, manager);
  vec_DLE_drop(&manager->iioDescriptorLayoutElements);
  deque_Pool_drop(&manager->readyPools);
  deque_Pool_drop(&manager->usedPools);
  if (manager->descriptorSetLayout != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(device, manager->descriptorSetLayout, NULL);
  }
  
  memset(manager, 0, sizeof(IIODescriptorPoolManager));
}

/****************************
 *     Writer functions     *
 ****************************/

void iio_create_descriptor_set_writer(
  IIODescriptorSetWriter *        writer) 

{
  if (writer == NULL) {
    fprintf(stderr, "iio_create_descriptor_set_writer failed: writer null\n");
    return;
  }

  writer->writes = vec_Write_init();
  writer->writeInfos = vec_WriteInfo_init();
}

void iio_write_image_descriptor(
  uint32_t                        binding, 
  uint32_t                        descriptorCount, 
  VkDescriptorType                descriptorType, 
  VkSampler                       sampler, 
  VkImageView                     imageView, 
  VkImageLayout                   imageLayout, 
  IIODescriptorSetWriter *        writer) 

{
  IIODescriptorWriteInfo writeInfo = {0};
  writeInfo.objectInfo.imageInfo.sampler = sampler;
  writeInfo.objectInfo.imageInfo.imageView = imageView;
  writeInfo.objectInfo.imageInfo.imageLayout = imageLayout;
  writeInfo.type = iio_writer_type_image;

  VkWriteDescriptorSet write = {0};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.pNext = NULL;
  write.dstSet = VK_NULL_HANDLE;
  write.dstBinding = binding;
  write.dstArrayElement = 0;
  write.descriptorCount = descriptorCount;
  write.descriptorType = descriptorType;
  write.pImageInfo = NULL; //  will be written to later
  write.pBufferInfo = NULL;
  write.pTexelBufferView = NULL;

  vec_WriteInfo_push_back(&writer->writeInfos, writeInfo);
  vec_Write_push_back(&writer->writes, write);
}

void iio_write_buffer_descriptor(
  uint32_t                        binding,
  uint32_t                        descriptorCount,
  VkDescriptorType                descriptorType,
  VkBuffer                        buffer,
  uint32_t                        offset,
  uint32_t                        range,
  IIODescriptorSetWriter *        writer) 

{
  IIODescriptorWriteInfo writeInfo = {0};
  writeInfo.objectInfo.bufferInfo.buffer = buffer;
  writeInfo.objectInfo.bufferInfo.offset = offset;
  writeInfo.objectInfo.bufferInfo.range = range;

  VkWriteDescriptorSet write = {0};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.pNext = NULL;
  write.dstSet = VK_NULL_HANDLE;
  write.dstBinding = binding;
  write.dstArrayElement = 0;
  write.descriptorCount = descriptorCount;
  write.descriptorType = descriptorType;
  write.pImageInfo = NULL;
  write.pBufferInfo = NULL; // will be written to later
  write.pTexelBufferView = NULL;

  vec_WriteInfo_push_back(&writer->writeInfos, writeInfo);
  vec_Write_push_back(&writer->writes, write);
}

void iio_update_set(
  VkDevice                        device,
  VkDescriptorSet                 descriptorSet,
  IIODescriptorSetWriter *        writer) 

{
  if (device == VK_NULL_HANDLE) {
    fprintf(stdout, "iio_update_set failed: Tried to update descriptor set with NULL device handle\n");
    return;
  } else if (descriptorSet == VK_NULL_HANDLE) {
    fprintf(stdout, "iio_update_set failed: Tried to update descriptor set with NULL descriptor set handle");
    return;
  } else if (writer == NULL) {
    fprintf(stdout, "iio_update_set failed: Tried to update descriptor set with NULL descriptor manager pointer");
    return;
  } else if (vec_Write_is_empty(&writer->writes)) {
    fprintf(stdout, "iio_update_set failed: Trid to update descriptor set with uninitialized write array");
    return;
  }
  uint32_t descriptorWriteCount = vec_Write_size(&writer->writes);
  for (uint32_t i = 0; i < descriptorWriteCount; i++) {
    VkWriteDescriptorSet * write = vec_Write_at_mut(&writer->writes, i);
    const IIODescriptorWriteInfo * writeInfo = vec_WriteInfo_at(&writer->writeInfos, i);
    write->dstSet = descriptorSet;
    if (writeInfo->type == iio_writer_type_buffer) {
      write->pBufferInfo = &writeInfo->objectInfo.bufferInfo;
    } else if (writeInfo->type == iio_writer_type_image) {
      write->pImageInfo = &writeInfo->objectInfo.imageInfo;
    } else {
      fprintf(stderr, "iio_update_set failed: invalid IIODescriptorWriteInfo type in writeInfos");
      return;
    }
  }

  const VkWriteDescriptorSet * pDescriptorWrites = writer->writes.data;
  vkUpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, 0, NULL);
  iio_clear_descriptor_set_writer(writer);
}

void iio_clear_descriptor_set_writer(
  IIODescriptorSetWriter *        writer) 

{
  vec_Write_clear(&writer->writes);
  vec_WriteInfo_clear(&writer->writeInfos);
}

void iio_destroy_descriptor_set_writer(
  IIODescriptorSetWriter *        writer) 

{
  vec_Write_drop(&writer->writes);
  vec_WriteInfo_drop(&writer->writeInfos);
}
