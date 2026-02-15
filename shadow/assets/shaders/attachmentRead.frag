#version 450 core

layout(location = 0) out vec4 outColor;

layout(input_attachment_index = 0, set = 0, binding = 1) uniform subpassInput inputColor;

void main()
{
    vec3 color = subpassLoad(inputColor).rgb;
    float average = 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;
    outColor = vec4(average, average, average, 1.0);
}