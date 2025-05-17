#version 460 core

layout(location = 0) out vec4 outColor;

layout(input_attachment_index = 0, set = 1, binding = 0) uniform subpassInput inputColor;

void main()
{
    vec3 color = subpassLoad(inputColor).rgb;
    outColor = vec4(color, 1.0);
}