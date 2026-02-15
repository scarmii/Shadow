#pragma once

#include "shadow.h"

class RenderGraphDemo : public Shadow::ShApp
{
public:
	RenderGraphDemo();
	~RenderGraphDemo();

	virtual void onRender() override;
	virtual void onImGuiRender() override;
	virtual void onUpdate(Shadow::Timestep ts) override;
private:
	void setupComputeRenderGraph();
	void setupInputAttachmentRenderGraph();
private:
	struct Light
	{
		alignas(16) glm::vec3 position;
		alignas(16) glm::vec3 color;
	};

	struct Particle
	{
		glm::vec2 position;
		glm::vec2 velocity;
		glm::vec4 color;
		float life;

		Particle()
			: position(0.0f), velocity(0.0f), color(1.0f), life(0.0f)
		{
		}
	};

	Light m_light;

	Shadow::RenderGraph m_computeRG;
	Shadow::RenderGraph m_inputAttachmentRG;
	Shadow::Mesh m_mangle;
	Shadow::PerspectiveCameraController m_cameraController;

	Shadow::Ref<Shadow::VertexBuffer> m_offscreenQuadVB;
	Shadow::Ref<Shadow::VertexBuffer> m_inputAttachmentQuadVB;
	Shadow::Ref<Shadow::IndexBuffer> m_quadIB;

	struct OffscreenPass
	{
		Shadow::Ref<Shadow::GraphicsPipeline> pipeline;
		Shadow::Ref<Shadow::Shader> shader;
		Shadow::Ref<Shadow::UniformBuffer> ubo;
	} m_offscreenPass;

	Shadow::Ref<Shadow::GraphicsPipeline> m_attachmentWritePipe;

	struct ComputeWrite
	{
		Shadow::Ref<Shadow::ComputePipeline> pipeline;
		Shadow::Ref<Shadow::Shader> shader;
	} m_computeWrite;

	struct ComputeRead
	{
		Shadow::Ref<Shadow::GraphicsPipeline> pipeline;
		Shadow::Ref<Shadow::Shader> shader;
	} m_computeRead;

	struct AttachmentRead
	{
		Shadow::Ref<Shadow::GraphicsPipeline> pipeline;
		Shadow::Ref<Shadow::Shader> shader;	
	} m_attachmentRead;
};