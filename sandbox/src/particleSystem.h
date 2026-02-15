#pragma once

#include "shadow.h"

struct ParticleProps
{
	glm::vec2 position;
	glm::vec2 velocity, velocityVariation;
	glm::vec4 colorBegin, colorEnd;
	float sizeBegin, sizeEnd, sizeVariation;
	float lifeTime = 1.0f;
};

class ParticleSystem
{
public:
	ParticleSystem(uint32_t maxParticles = 100000);

	void onUpdate(Shadow::Timestep ts);
	void onRender(const Shadow::OrthoCamera& camera);

	void emit(const ParticleProps& particleProps);
private:
	struct Particle
	{
		glm::vec2 position;
		glm::vec2 velocity;
		glm::vec4 colorBegin, colorEnd;
		float rotation = 0.0f;
		float sizeBegin, sizeEnd;

		float lifeTime = 0.5f;
		float lifeRemaining = 0.0f;

		bool active = false;
	};
	std::vector<Particle> m_particlePool;
	uint32_t m_poolIndex;
};