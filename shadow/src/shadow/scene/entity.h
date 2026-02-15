#pragma once

#include "scene.h"

#include "entt.hpp"

namespace Shadow
{
	class Entity
	{
	public:
		Entity() = default;
		Entity(entt::entity handle, Scene* pScene);
		Entity(const Entity& other) = default;

		template<typename T, typename ...Args>
		T& addComponent(Args&&... args)
		{
			SH_ASSERT(!hasComponent<T>(), typeid(T).name(), "entity already has %s");
			return m_pScene->m_registry.emplace<T>(m_entityHandle, std::forward<Args>(args)...);
		}

		template<typename T>
		T& getComponent()
		{
			SH_ASSERT(hasComponent<T>(), typeid(T).name(), "entity doesn't have %s");
			return m_pScene->m_registry.get<T>(m_entityHandle);
		}

		template<typename T>
		T* tryGetComponent()
		{
			return m_pScene->m_registry.try_get<T>(m_entityHandle);
		}

		template<typename T>
		bool hasComponent()
		{
			return m_pScene->m_registry.try_get<T>(m_entityHandle) != nullptr;
		}

		template<typename T>
		void removeComponent()
		{
			SH_ASSERT(hasComponent<T>(), typeid(T).name(), "entity doesn't have %s");
			m_pScene->m_registry.remove<T>(m_entityHandle);
		}

		operator bool() const { return m_entityHandle != entt::null; }
		operator uint32_t() const { return static_cast<uint32_t>(m_entityHandle); }

		bool operator==(const Entity& other) const 
		{
			return m_entityHandle == other.m_entityHandle && m_pScene == other.m_pScene;
		}

		bool operator!=(const Entity& other) const
		{
			return !(*this == other);
		}
	private:
		entt::entity m_entityHandle{ entt::null };
		Scene* m_pScene = nullptr;
	};

	class ScriptableEntity
	{
	public:
		virtual ~ScriptableEntity() {}

		template<typename T>
		T& getComponent() { return m_entity.getComponent<T>(); }
	protected:
		virtual void onCreate() {}
		virtual void onDestroy() {}
		virtual void onUpdate(Timestep ts) {}
	private:
		Entity m_entity;
		friend class Scene;
	};
}