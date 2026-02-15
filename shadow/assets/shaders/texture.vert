#version 450 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;
layout(location = 2) in vec2 a_texCoords;
layout(location = 3) in uint a_texIndex;
layout(location = 4) in float a_tilingFactor;

layout(push_constant) uniform PushConstant {
    layout(offset = 0)  mat4 viewProjection;
};

layout(location = 0) out vec4 v_color;
layout(location = 1) out vec2 v_texCoords;
layout(location = 2) out uint v_texIndex;
layout(location = 3) out float v_tilingFactor;

void main()
{
    gl_Position = viewProjection * vec4(a_position, 1.0);
    v_color = a_color;
    v_texCoords = a_texCoords;
    v_texIndex = a_texIndex;
    v_tilingFactor = a_tilingFactor;
}