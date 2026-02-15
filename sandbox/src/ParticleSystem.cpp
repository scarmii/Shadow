#include "particleSystem.h"

#include <imgui/imgui.h>
#include <GLFW/glfw3.h>

#include <glm/gtc/constants.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/compatibility.hpp>

using namespace Shadow;

ParticleSystem::ParticleSystem(uint32_t maxParticles)
	: m_poolIndex(maxParticles-1)
{
	m_particlePool.resize(maxParticles);
}

void ParticleSystem::onUpdate(Timestep ts)
{
	for (auto& particle : m_particlePool)
	{
		if (!particle.active)
			continue;

		if (particle.lifeRemaining <= 0.0f)
		{
			particle.active = false;
			continue;
		}

		particle.lifeRemaining -= ts;
		particle.position += particle.velocity * (float)ts;
		particle.rotation += 0.01f * ts;
	}
}

void ParticleSystem::onRender(const OrthoCamera& camera)
{
	for (auto& particle : m_particlePool)
	{
		if (!particle.active)
			continue;

		// Fade away particles
		float life = particle.lifeRemaining / particle.lifeTime;
		glm::vec4 color = glm::lerp(particle.colorEnd, particle.colorBegin, life);

		float size = glm::lerp(particle.sizeEnd, particle.sizeBegin, life);
		glm::vec3 pos = { particle.position, 0.7f };
		Renderer2D::drawRotatedQuad(pos, { size*0.25f, size*0.25f }, particle.rotation, color);
	}
}

void ParticleSystem::emit(const ParticleProps& particleProps)
{
	Particle& particle = m_particlePool[m_poolIndex];
	particle.active = true;
	particle.position = particleProps.position;
	particle.rotation = Random::_float() * 2.0f * glm::pi<float>();

	// Velocity
	particle.velocity = particleProps.velocity;
	particle.velocity.x += particleProps.velocityVariation.x * (Random::_float() - 0.5f);
	particle.velocity.y += particleProps.velocityVariation.y * (Random::_float() - 0.5f);

	// Color
	particle.colorBegin = particleProps.colorBegin;
	particle.colorEnd = particleProps.colorEnd;

	particle.lifeTime = particleProps.lifeTime;
	particle.lifeRemaining = particleProps.lifeTime;
	particle.sizeBegin = particleProps.sizeBegin + particleProps.sizeVariation * (Random::_float() - 0.5f);
	particle.sizeEnd = particleProps.sizeEnd;

	m_poolIndex = --m_poolIndex % m_particlePool.size();
}
