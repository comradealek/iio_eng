#include <stdio.h>
#include "iio_pipeline.h"
#include <vulkan/vulkan.h>

IIOGraphicsPipelineStates iio_create_graphics_pipeline_state() 

{
  IIOGraphicsPipelineStates state = {0};

  state.createInfo = (VkGraphicsPipelineCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
  state.stages = vec_PipelineShaderStageCreateInfo_init();

  return state;
}

void iio_create_shader_stage_create_info(
  uint32_t                                  stageCount,
  VkShaderStageFlagBits *                   stages,
  VkShaderModule *                          modules,
  IIOGraphicsPipelineStates *               state) 

{
  if (stages == NULL) {
    fprintf(stderr, "iio_create_shader_stage_info failed: stages array null\n");
    return;
  } else if (modules == NULL) {
    fprintf(stderr, "iio_create_shader_stage_info failed: modules array null\n");
    return;
  } else if (state == NULL) {
    fprintf(stderr, "iio_create_shader_stage_info failed: state null\n");
    return;
  }

  for (uint32_t i = 0; i < stageCount; i++) {
    VkPipelineShaderStageCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = stages[i],
      .module = modules[i],
      .pName = "main"};
    vec_PipelineShaderStageCreateInfo_push(&state->stages, createInfo);
  }
}

void iio_set_vertex_input_state_create_info(
  uint32_t                                  bindingDescriptionCount,
  VkVertexInputBindingDescription *         bindingDescriptions,
  uint32_t                                  attributeDescriptionCount,
  VkVertexInputAttributeDescription *       attributeDescriptions,
  IIOGraphicsPipelineStates *               state) 

{
  state->vertexInputState = (VkPipelineVertexInputStateCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = bindingDescriptionCount,
    .pVertexBindingDescriptions = bindingDescriptions,
    .vertexAttributeDescriptionCount = attributeDescriptionCount,
    .pVertexAttributeDescriptions = attributeDescriptions};
}

void iio_set_input_assembly_state_create_info(
  VkPrimitiveTopology                       topology,
  VkBool32                                  primitiveRestartEnable,
  IIOGraphicsPipelineStates *               state) 

{
  state->inputAssemblyState = (VkPipelineInputAssemblyStateCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .topology = topology,
    .primitiveRestartEnable = primitiveRestartEnable};
}

void iio_set_tessellation_state_create_info(
  IIOGraphicsPipelineStates *               state) 

{
  state->tessellationState = (VkPipelineTessellationStateCreateInfo) {
    .flags = 0,
    .pNext = 0,
    .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
    .patchControlPoints = 0,
  };
}

void iio_set_viewport_state_create_info(
  uint32_t                                  viewportCount,
  VkViewport *                              viewports,
  uint32_t                                  scissorCount,
  VkRect2D *                                scissors,
  IIOGraphicsPipelineStates *               state) 

{
  state->viewportState = (VkPipelineViewportStateCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .viewportCount = viewportCount,
    .pViewports = viewports,
    .scissorCount = scissorCount,
    .pScissors = scissors};
}

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
  IIOGraphicsPipelineStates *               state) 

{
  state->rasterizationState = (VkPipelineRasterizationStateCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .depthClampEnable = depthClampEnable,
    .rasterizerDiscardEnable = rasterizerDiscardEnable,
    .polygonMode = polygonMode,
    .cullMode = cullMode,
    .frontFace = frontFace,
    .depthBiasEnable = depthBiasEnable,
    .depthBiasConstantFactor = depthBiasConstantFactor,
    .depthBiasClamp = depthBiasClamp,
    .depthBiasSlopeFactor = depthBiasSlopeFactor,
    .lineWidth = lineWidth};
}

void iio_set_multisample_state_create_info(
  VkSampleCountFlagBits                     rasterizationSamples,
  VkBool32                                  sampleShadingEnable,
  float                                     minSampleShading,
  VkSampleMask *                            pSampleMask,
  VkBool32                                  alphaToCoverageEnable,
  VkBool32                                  alphaToOneEnable,
  IIOGraphicsPipelineStates *               state) 

{
  state->multisampleState = (VkPipelineMultisampleStateCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .rasterizationSamples = rasterizationSamples,
    .sampleShadingEnable = sampleShadingEnable,
    .minSampleShading = minSampleShading,
    .pSampleMask = pSampleMask,
    .alphaToCoverageEnable = alphaToCoverageEnable,
    .alphaToOneEnable = alphaToOneEnable};
}

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
  IIOGraphicsPipelineStates *               state) 

{
  state->depthStencilStateExists = true;
  state->depthStencilState = (VkPipelineDepthStencilStateCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .pNext = NULL,
    .flags = flags,
    .depthTestEnable = depthTestEnable,
    .depthWriteEnable = depthWriteEnable,
    .depthCompareOp = depthCompareOp,
    .depthBoundsTestEnable = depthBoundsTestEnable,
    .stencilTestEnable = stencilTestEnable,
    .front = front,
    .back = back,
    .minDepthBounds = minDepthBounds,
    .maxDepthBounds = maxDepthBounds};
}

void iio_set_color_blend_state_create_info(
  VkPipelineColorBlendStateCreateFlags      flags,
  VkBool32                                  logicOpEnable,
  VkLogicOp                                 logicOp,
  uint32_t                                  attachmentCount,
  VkPipelineColorBlendAttachmentState *     colorBlendAttachments,
  float                                     blendConstants [4],
  IIOGraphicsPipelineStates *               state) 

{
  state->colorBlendState = (VkPipelineColorBlendStateCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .pNext = NULL,
    .flags = flags,
    .logicOpEnable = logicOpEnable,
    .logicOp = logicOp,
    .attachmentCount = attachmentCount,
    .pAttachments = colorBlendAttachments};
  state->colorBlendState.blendConstants[0] = blendConstants[0];
  state->colorBlendState.blendConstants[1] = blendConstants[1];
  state->colorBlendState.blendConstants[2] = blendConstants[2];
  state->colorBlendState.blendConstants[3] = blendConstants[3];
}

void iio_set_dynamic_state_create_info(
  uint32_t                                  dynamicStateCount,
  VkDynamicState *                          dynamicStates,
  IIOGraphicsPipelineStates *               state) 

{
  state->dynamicState = (VkPipelineDynamicStateCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .dynamicStateCount = dynamicStateCount,
    .pDynamicStates = dynamicStates};
}

void iio_set_rendering_info(
  VkFormat *                                colorAttachmentFormat,
  VkFormat                                  depthAttachmentFormat,
  VkFormat                                  stencilAttachmentFormat,
  IIOGraphicsPipelineStates *               state)
  
{
  state->renderingInfo = (VkPipelineRenderingCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
    .pNext = NULL,
    .viewMask = 0,
    .colorAttachmentCount = 1,
    .pColorAttachmentFormats = colorAttachmentFormat,
    .depthAttachmentFormat = depthAttachmentFormat,
    .stencilAttachmentFormat = stencilAttachmentFormat};
}

void iio_create_graphics_pipeline_layout(
  VkDevice                                  device,
  uint32_t                                  setLayoutCount,
  VkDescriptorSetLayout *                   setLayouts,
  uint32_t                                  pushConstantRangeCount,
  VkPushConstantRange *                     pushConstantRanges,
  IIOGraphicsPipelineManager *              manager) 

{
  VkPipelineLayoutCreateInfo createInfo = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .setLayoutCount = setLayoutCount,
    .pSetLayouts = setLayouts,
    .pushConstantRangeCount = pushConstantRangeCount,
    .pPushConstantRanges = pushConstantRanges};

  vkCreatePipelineLayout(device, &createInfo, NULL, &manager->layout);
}

void iio_create_graphics_pipeline(
  VkDevice                                  device,
  IIOGraphicsPipelineManager *              manager,
  bool                                      useDynamicRendering,
  VkRenderPass                              renderPass,
  uint32_t                                  subpass,
  IIOGraphicsPipelineStates *               state) 

{
  VkGraphicsPipelineCreateInfo createInfo = {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext = useDynamicRendering ? &state->renderingInfo : NULL,
    .flags = 0,
    .stageCount = vec_PipelineShaderStageCreateInfo_size(&state->stages),
    .pStages = state->stages.data,
    .pVertexInputState = &state->vertexInputState,
    .pInputAssemblyState = &state->inputAssemblyState,
    .pTessellationState = &state->tessellationState,
    .pViewportState = &state->viewportState,
    .pRasterizationState = &state->rasterizationState,
    .pMultisampleState = &state->multisampleState,
    .pDepthStencilState = state->depthStencilStateExists ? &state->depthStencilState : NULL,
    .pColorBlendState = &state->colorBlendState,
    .pDynamicState = &state->dynamicState,
    .layout = manager->layout,
    .renderPass = useDynamicRendering ? VK_NULL_HANDLE : renderPass,
    .subpass = useDynamicRendering ? 0 : subpass,
    .basePipelineHandle = VK_NULL_HANDLE,
    .basePipelineIndex = 0};
  
  

  vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &createInfo, NULL, &manager->pipeline);
}

void iio_destroy_graphics_pipeline(
  VkDevice                                  device,
  IIOGraphicsPipelineManager *              manager) 

{
  if (manager->layout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, manager->layout, NULL);
  if (manager->pipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, manager->pipeline, NULL);
  manager->layout = VK_NULL_HANDLE;
  manager->pipeline = VK_NULL_HANDLE;
}
