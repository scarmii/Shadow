#version 450 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 2) in vec3 instancePos;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform VP {
    layout(offset = 0) mat4 viewProjection;
};

out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    gl_Position = viewProjection * vec4(inPosition.xyz + instancePos, 1.0);
    fragColor = inColor;
}