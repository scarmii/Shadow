#pragma once

#include "Shadow.hpp"

class ParticleSystem : public Shadow::Layer
{
public:
	virtual void onAttach() override;
	virtual void onDetach() override;
	virtual void onUpdate(Shadow::Timestep ts) override;
	virtual void onRender() override;
	virtual void onImGuiRender() override;
private:
	bool onMouseMove(const Shadow::MouseMovedEvent& e);
private:
	struct Particle
	{
		glm::vec2 pos;
		glm::vec2 vel;
		glm::vec4 gradientPos;
	};

	struct
	{
		Shadow::Ref<Shadow::Texture2D> particle;
		Shadow::Ref<Shadow::Texture2D> gradient;
	} m_textures;

	glm::vec2 m_mousePos;

	Shadow::Ref<Shadow::StorageBuffer> m_particles;

	struct Compute
	{
		Shadow::Ref<Shadow::Shader> shader;
		Shadow::Ref<Shadow::ComputePipeline> pipeline;
		Shadow::Ref<Shadow::UniformBuffer> uniformBuffer;

		struct UniformData
		{
			float deltaT;
			float dstX;
			float dstY;
			int32_t particleCount = 256 * 1024;
		} uniformData;
	} m_compute;

	struct Graphics
	{
		Shadow::Ref<Shadow::Shader> shader;
		Shadow::Ref<Shadow::GraphicsPipeline> pipeline;
		Shadow::Ref<Shadow::Renderpass> renderpass;
	} m_graphics;
};