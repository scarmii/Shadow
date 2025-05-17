#version 450 core

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 v_TexCoords;

layout(set = 0, binding = 0) uniform sampler2D u_sampler;

void main()
{
    vec4 color = texture(u_sampler, v_TexCoords);
    float average = 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;
    outColor = vec4(average, average, average, 1.0);
}