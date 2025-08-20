#version 450
#pragma shader_stage(vertex)

layout(set = 0, binding = 0) uniform UniformBufferObject {
  mat4 view;
  mat4 projection;
} ubo;

layout(set = 1, binding =  0) uniform sampler2D baseColorSampler;
layout(set = 1, binding =  1) uniform sampler2D metallicRoughnessSampler;
layout(set = 1, binding =  2) uniform sampler2D normalSampler;
layout(set = 1, binding =  3) uniform sampler2D occlusionSampler;
layout(set = 1, binding =  4) uniform sampler2D emissiveSampler;
layout(set = 1, binding =  5) uniform MaterialUBO {
  vec4 baseColorFactor;
  vec4 emissiveFactor;
  float metallicFactor;
  float roughnessFactor;
  float normalScale;
  float occlusionStrength;
  float alphacutoff;
  int texCoordIndex;
} material;

layout(set = 2, binding =  0) uniform ModelUBO {
  mat4 matrix;
} modelUBO;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec2 inTexCoord[2];
layout(location = 5) in vec4 inColor;
layout(location = 6) in vec4 inJointIndices;
layout(location = 7) in vec4 inJointWeights;

layout(location = 0) out vec4 fragColor;
//
//
layout(location = 3) out vec2 fragTexCoord[2];

void main() {
  gl_Position = ubo.projection * ubo.view * modelUBO.matrix * vec4(inPosition, 1.0);
  fragColor = inColor;
  fragTexCoord[0] = inTexCoord[0];
  fragTexCoord[1] = inTexCoord[1];
}