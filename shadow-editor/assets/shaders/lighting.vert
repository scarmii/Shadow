#version 450 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoords;
layout(location = 3) in uint a_materialIndex;

layout(location = 0) out uint v_materialIndex;
layout(location = 1) out vec3 v_fragPos;
layout(location = 2) out vec3 v_normal;
layout(location = 3) out vec2 v_texCoords;

layout(push_constant) uniform MVPMatrix {
	mat4 mvp;
};

void main()
{
	gl_Position = mvp * vec4(a_position, 1.0);
	v_materialIndex = a_materialIndex;
	v_fragPos = a_position;
	v_normal = a_normal;
	v_texCoords = a_texCoords;
}