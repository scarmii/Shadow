#pragma once

#include "shadow/core/types.h"
#include "shadow/scene/scene.h"
#include "shadow/scene/entity.h"

namespace Shadow
{
	struct TagComponent;

	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(const Ref<Scene>& scene);

		void setContext(const Ref<Scene>& scene);

		void onImGuiRender();
	private:
		void drawEntityNode(Entity entity);
		void drawComponents(Entity entity);

		template<typename T>
		void drawComponent(Entity entity, const std::string& name, const std::function<void(Entity)>& fn)
		{
			if (entity.hasComponent<T>())
			{
				bool opened = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, name.c_str());
				if (opened)
				{
					fn(entity);
					ImGui::TreePop();
				}
			}
		}

		template<>
		void drawComponent<TagComponent>(Entity entity, const std::string& name, const std::function<void(Entity)>& fn)
		{
			if (entity.hasComponent<TagComponent>())
				fn(entity);
		}
	private:
		Ref<Scene> m_context;
		Entity m_selectionContext;
	};
}