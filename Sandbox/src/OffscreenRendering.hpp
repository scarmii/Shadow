#pragma once

#include "Shadow.hpp"

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


class OffscreenRendering : public Shadow::Layer
{
	struct Pipeline;
public:
	OffscreenRendering();

	virtual void onUpdate(Shadow::Timestep ts) override;
	virtual void onRender() override;
	virtual void onImGuiRender() override;

	inline const Shadow::Ref<Shadow::Texture2D> getImage() const { return m_offscreenData.renderpass->getOutput(0); }
private:
	void renderPipeline(const Pipeline& pipeline);
	Pipeline createPipeline(const Shadow::FramebufferInfo& framebufferInfo, const std::string& shaderPath);
private:
	Light m_light{};
	Shadow::Mesh m_mesh;
	glm::mat4 m_mvp, m_model;

	struct OffscreenData
	{
		Shadow::Ref<Shadow::GraphicsPipeline> pipe;
		Shadow::Ref<Shadow::Shader> shader;
		Shadow::Ref<Shadow::Renderpass> renderpass;
		Shadow::Ref<Shadow::UniformBuffer> ubo;
	} m_offscreenData;

	struct Pipeline
	{
		Shadow::Ref<Shadow::GraphicsPipeline> graphicsPipeline;
		Shadow::Ref<Shadow::Shader> shader;
		Shadow::Ref<Shadow::Renderpass> renderpass;
	} m_defaultPipe, m_reversedColorPipe, m_blackWhitePipe;

	Shadow::Ref<Shadow::VertexBuffer> m_squareVB;
	Shadow::Ref<Shadow::IndexBuffer> m_squareIB;

	//Shadow::Ref<Shadow::RenderBuffer> m_squareVB;
	//Shadow::Ref<Shadow::RenderBuffer> m_squareIB;

	Shadow::PerspectiveCameraController m_cameraController;
	Shadow::CursorMode m_cursorMode;

	int m_postEffects = 0;
	bool m_reversedColor = false, m_blackWhite = false;
};