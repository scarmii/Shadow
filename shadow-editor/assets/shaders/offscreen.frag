#version 450 core

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 v_TexCoords;

layout(set = 0, binding = 0) uniform sampler2D u_sampler;

void main()
{
    outColor = texture(u_sampler, v_TexCoords);
}