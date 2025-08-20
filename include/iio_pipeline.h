#ifndef IIO_PIPELINE_H
#define IIO_PIPELINE_H

#include <vulkan/vulkan.h>

#define T vec_GPCreateInfo, VkGraphicsPipelineCreateInfo
#include "stc/vec.h"

#define T vec_Pipeline, VkPipeline
#include "stc/vec.h"

#define T vec_PipelineShaderStageCreateInfo, VkPipelineShaderStageCreateInfo
#include "stc/vec.h"

typedef struct IIOGraphicsPipelineStates_S {
  VkGraphicsPipelineCreateInfo              createInfo;

  vec_PipelineShaderStageCreateInfo         stages;
  VkPipelineVertexInputStateCreateInfo      vertexInputState;
  VkPipelineInputAssemblyStateCreateInfo    inputAssemblyState;
  VkPipelineTessellationStateCreateInfo     tessellationState;
  VkPipelineViewportStateCreateInfo         viewportState;
  VkPipelineRasterizationStateCreateInfo    rasterizationState;
  VkPipelineMultisampleStateCreateInfo      multisampleState;
  bool                                      depthStencilStateExists;
  VkPipelineDepthStencilStateCreateInfo     depthStencilState;
  VkPipelineColorBlendStateCreateInfo       colorBlendState;
  VkPipelineDynamicStateCreateInfo          dynamicState;
  VkPipelineRenderingCreateInfo             renderingInfo;
  
} IIOGraphicsPipelineStates;

typedef struct IIOGraphicsPipelineManager_S {
  VkPipeline                                pipeline;
  VkPipelineLayout                          layout;
} IIOGraphicsPipelineManager;

IIOGraphicsPipelineStates iio_create_graphics_pipeline_state();

void iio_create_shader_stage_create_info(
  uint32_t                                  stageCount,
  VkShaderStageFlagBits *                   stages,
  VkShaderModule *                          modules,
  IIOGraphicsPipelineStates *               state);

void iio_set_vertex_input_state_create_info(
  uint32_t                                  bindingDescriptionCount,
  VkVertexInputBindingDescription *         bindingDescriptions,
  uint32_t                                  attributeDescriptionCount,
  VkVertexInputAttributeDescription *       attributeDescriptions,
  IIOGraphicsPipelineStates *               state);

void iio_set_input_assembly_state_create_info(
  VkPrimitiveTopology                       topology,
  VkBool32                                  primitiveRestartEnable,
  IIOGraphicsPipelineStates *               state);

void iio_set_tessellation_state_create_info(
  IIOGraphicsPipelineStates *               state);

void iio_set_viewport_state_create_info(
  uint32_t                                  viewportCount,
  VkViewport *                              viewports,
  uint32_t                                  scissorCount,
  VkRect2D *                                scissors,
  IIOGraphicsPipelineStates *               state);

void iio_set_rasterization_state_create_info(
  VkBool32                                  depthClampEnable,
  VkBool32                                  rasterizerDiscardEnable,
  VkPolygonMode                             polygonMode,
  VkCullModeFlags                           cullMode,
  VkFrontFace                               frontFace,
  VkBool32                                  depthBiasEnable,
  float                                     depthBiasConstantFactor,
  float                                     depthBiasClamp,
  float                                     depthBiasSlopeFactor,
  float                                     lineWidth,
  IIOGraphicsPipelineStates *               state);

void iio_set_multisample_state_create_info(
  VkSampleCountFlagBits                     rasterizationSamples,
  VkBool32                                  sampleShadingEnable,
  float                                     minSampleShading,
  VkSampleMask *                            pSampleMask,
  VkBool32                                  alphaToCoverageEnable,
  VkBool32                                  alphaToOneEnable,
  IIOGraphicsPipelineStates *               state);

void iio_set_depth_stencil_state_create_info(
  VkPipelineDepthStencilStateCreateFlags    flags,
  VkBool32                                  depthTestEnable,
  VkBool32                                  depthWriteEnable,
  VkCompareOp                               depthCompareOp,
  VkBool32                                  depthBoundsTestEnable,
  VkBool32                                  stencilTestEnable,
  VkStencilOpState                          front,
  VkStencilOpState                          back,
  float                                     minDepthBounds,
  float                                     maxDepthBounds,
  IIOGraphicsPipelineStates *               state);

void iio_set_color_blend_state_create_info(
  VkPipelineColorBlendStateCreateFlags      flags,
  VkBool32                                  logicOpEnable,
  VkLogicOp                                 logicOp,
  uint32_t                                  attachmentCount,
  VkPipelineColorBlendAttachmentState *     colorBlendAttachments,
  float                                     blendConstants [4],
  IIOGraphicsPipelineStates *               state);

void iio_set_dynamic_state_create_info(
  uint32_t                                  dynamicStateCount,
  VkDynamicState *                          dynamicStates,
  IIOGraphicsPipelineStates *               state);

void iio_set_rendering_info(
  VkFormat *                                colorAttachmentFormat,
  VkFormat                                  depthAttachmentFormat,
  VkFormat                                  stencilAttachmentFormat,
  IIOGraphicsPipelineStates *               state);

void iio_create_graphics_pipeline_layout(
  VkDevice                                  device,
  uint32_t                                  setLayoutCount,
  VkDescriptorSetLayout *                   setLayouts,
  uint32_t                                  pushConstantRangeCount,
  VkPushConstantRange *                     pushConstantRanges,
  IIOGraphicsPipelineManager *              manager);

void iio_create_graphics_pipeline(
  VkDevice                                  device,
  IIOGraphicsPipelineManager *              manager,
  bool                                      useDynamicRendering,
  VkRenderPass                              renderPass,
  uint32_t                                  subpass,
  IIOGraphicsPipelineStates *               state);

void iio_destroy_graphics_pipeline(
  VkDevice                                  device,
  IIOGraphicsPipelineManager *              manager);

#endif