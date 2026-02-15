#pragma once

#include "shadow/core/timestep.h"
#include "entt.hpp"

namespace Shadow
{
	class Entity;
	class Camera;

	class Scene
	{
	public:
		Scene();
		~Scene();

		Entity createEntity(const std::string& name = std::string());
		
		void onUpdate(Timestep ts);
		void renderScene();
		void onViewportResize(uint32_t width, uint32_t height);
	private:
		entt::registry m_registry;
		uint32_t m_viewportWidth = 0, m_viewportHeight = 0;

		friend class Entity;
		friend class SceneHierarchyPanel;
	};
}