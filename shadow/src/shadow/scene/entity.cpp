#include "shpch.h"
#include "entity.h"

namespace Shadow
{
	Entity::Entity(entt::entity handle, Scene* pScene)
		: m_entityHandle(handle), m_pScene(pScene)
	{
	}
}