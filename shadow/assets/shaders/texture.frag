#version 450 core

layout(location = 0) out vec4 o_color;

layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_texCoords;
layout(location = 2) in flat uint v_texIndex;
layout(location = 3) in flat float v_tilingFactor;

layout(set = 0, binding = 0) uniform sampler2D u_samplers[10];

void main()
{
	vec4 color = v_color * texture(u_samplers[v_texIndex], v_texCoords * v_tilingFactor);

	if (color.a == 0.0)
		discard;
	else
		o_color = color;
}