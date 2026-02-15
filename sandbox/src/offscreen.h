#pragma once

#include "shadow.h"

#include <imgui/imgui.h>

#include <glm/glm.hpp>

struct Light 
{
    alignas(16) glm::vec3 position;
	alignas(16) glm::vec3 color;
};

enum class PostEffects 
{
	None =          0,
	ReversedColor = 1,
	BlackWhite =    2
};

class OffscreenDemo : public Shadow::ShApp
{
	struct Pipeline;
public:
	OffscreenDemo();

	virtual void onUpdate(Shadow::Timestep ts) override;
	virtual void onRender() override;
	virtual void onImGuiRender() override;
private:
	void renderPipeline(Pipeline& pipeline) const;
	Pipeline createPipeline(const Shadow::FramebufferInfo& framebufferInfo, const std::string& shaderPath) const;
private:
	Light m_light{};
	Shadow::Mesh m_mesh;
	glm::mat4 m_mvp, m_model;

	struct OffscreenData
	{
		Shadow::Ref<Shadow::GraphicsPipeline> pipe;
		Shadow::Ref<Shadow::Shader> shader;
		Shadow::Ref<Shadow::RenderPass> renderpass;
		Shadow::Ref<Shadow::UniformBuffer> ubo;
	} m_offscreenData;

	struct Pipeline
	{
		Shadow::Ref<Shadow::GraphicsPipeline> m_graphicsPipeline;
		Shadow::Ref<Shadow::Shader> shader;
		Shadow::Ref<Shadow::RenderPass> renderpass;
	} m_defaultPipe, m_reversedColorPipe, m_blackWhitePipe;

	Shadow::Ref<Shadow::VertexBuffer> m_squareVB;
	Shadow::Ref<Shadow::IndexBuffer> m_squareIB;

	Shadow::Ref<Shadow::Shader> m_computeShader;
	Shadow::Ref<Shadow::ComputePipeline> m_computePipe;

	Shadow::PerspectiveCameraController m_cameraController;
	Shadow::CursorMode m_cursorMode;

	ImTextureID m_imguiTextureID;

	int m_postEffects = 0;
	bool m_reversedColor = false, m_blackWhite = false;
};