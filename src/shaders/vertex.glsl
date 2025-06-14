#version 450
#pragma shader_stage(vertex)

layout(binding = 0) uniform UniformBufferObject {
  mat4 model;
  mat4 view;
  mat4 projection;
} ubo;
layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
  mat4 Model = ubo.model;
  Model[3][1] += gl_InstanceIndex * 1.75; // Example transformation based on instance index
  gl_Position = ubo.projection * ubo.view * Model * vec4(inPosition, 1.0);
  fragColor = inColor;
  fragTexCoord = inTexCoord;
}