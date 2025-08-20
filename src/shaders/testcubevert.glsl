#version 450
#pragma shader_stage(vertex)


layout(set = 0, binding = 0) uniform CameraObject {
  mat4 view;
  mat4 projection;
} camera;

layout(set = 2, binding = 0) uniform ModelObject {
  mat4 matrix;
} model;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;


void main()

{
  gl_Position = camera.projection * camera.view * model.matrix * vec4(inPosition, 1.0);
  fragColor = vec4(inColor, 1.0);
  fragTexCoord = inTexCoord;
}