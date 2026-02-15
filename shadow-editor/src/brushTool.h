#pragma once

#include "shadow.h"

#include <glm/glm.hpp>

struct ImVec2;

struct BrushProperties
{
	glm::vec4 color{ 1.0f };
	float width = 15.0f;
	int circle = 0;
};

class PaintTool
{
public:
	PaintTool();
	~PaintTool() = default;

	void onRender();
	void onImGuiRender();
	void onViewportRender();
	void setShaderInputImage();
private:
	BrushProperties m_properties;
	glm::vec4 m_clearColor{ 0.025f,0.025f,0.025f,1.0f };

	struct UboData
	{
		ImVec2 mousePos;
	} m_uboData;

	Shadow::Ref<Shadow::UniformBuffer> m_ubo;
	Shadow::Ref<Shadow::UniformBuffer> m_brushPropsUbo;

	Shadow::Ref<Shadow::Shader> m_shader;
	Shadow::Ref<Shadow::ComputePipeline> m_computePipe;
};
