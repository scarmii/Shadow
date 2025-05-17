#version 450 core

layout(location = 0) out vec4 out_color;

layout(location = 0) flat in uint v_materialIndex;
layout(location = 1) in vec3 v_fragPos;
layout(location = 2) in vec3 v_normal;
layout(location = 3) in vec2 v_texCoords;

layout(set = 0, binding = 0) uniform Light {
	vec3 position;
	vec3 color;
} u_light;

layout(set = 0, binding = 1) uniform sampler2D u_samplers[32];

void main() {

	const float ambientStrength = 0.1;
	vec3 ambient = ambientStrength * u_light.color;

	vec3 norm = normalize(v_normal);
	vec3 lightDir = normalize(u_light.position - v_fragPos);
	float diff = max(dot(lightDir, norm), 0.0);
	vec3 diffuse = diff * u_light.color;

	vec4 color = texture(u_samplers[v_materialIndex], v_texCoords);
	out_color = vec4(color.rgb * (ambient + diffuse), 1.0);
}