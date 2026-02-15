#pragma once

#include "shadow.h"

#include "particleSystem.h"

#include <future>

class Sandbox2D : public Shadow::ShApp
{
public:
	Sandbox2D();
	~Sandbox2D();

	virtual void onUpdate(Shadow::Timestep ts) override;
	virtual void onRender() override;
	virtual void onImGuiRender() override;
private:
	Shadow::OrthoCameraController m_cameraController;
	glm::vec3 m_squareColor = { 0.0f, 0.2f, 0.8f };

	Shadow::Ref<Shadow::CommandBuffer> m_cmdBuffer;

	Shadow::Ref<Shadow::Texture2D> m_catTex;
	Shadow::Ref<Shadow::Texture2D> m_assasinTex;
	Shadow::Ref<Shadow::Texture2D> m_spriteSheet;
	Shadow::Ref<Shadow::Sprite2D> m_texStairs, m_texBarrel, m_texTree;

	Shadow::Ref<Shadow::Shader> m_computeShader;
	Shadow::Ref<Shadow::ComputePipeline> m_computePipe;

	ParticleSystem m_particleSystem;
	ParticleProps m_particle;

	uint32_t m_mapWidth, m_mapHeight;
	std::unordered_map<char, Shadow::Ref <Shadow::Sprite2D>> m_spriteMap;
};
