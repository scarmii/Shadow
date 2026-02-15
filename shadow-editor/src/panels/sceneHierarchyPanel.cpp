#include "sceneHierarchyPanel.h"

#include "shadow/scene/components.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>

namespace Shadow
{
	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& context)
	{
		setContext(context);
	}

	void SceneHierarchyPanel::setContext(const Ref<Scene>& context)
	{
		m_context = context;
	}

	void SceneHierarchyPanel::onImGuiRender()
	{
		SH_PROFILE_FUNCTION();

		ImGui::Begin("Scene Hierarchy");
		{
			m_context->m_registry.view<entt::entity>().each([&](entt::entity entityID)
			{
				Entity entity{ entityID, m_context.get() };
				drawEntityNode(entity);
			});

			if (ImGui::IsWindowHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
				m_selectionContext = {};
		}
		ImGui::End();

		ImGui::Begin("Properties"); 
		{
			if (m_selectionContext)
				drawComponents(m_selectionContext);
		}
		ImGui::End();
	}

	void SceneHierarchyPanel::drawEntityNode(Entity entity)
	{
		auto& tag = entity.getComponent<TagComponent>().tag;
		void* ptrID = (void*)static_cast<uint64_t>(static_cast<uint32_t>(entity));

		ImGuiTreeNodeFlags flags = (m_selectionContext == entity ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
		bool opened = ImGui::TreeNodeEx(ptrID, flags, tag.c_str());

		if (ImGui::IsItemClicked())
			m_selectionContext = entity;

		if (opened)
			ImGui::TreePop();
	}

	static void drawVec3Control(const std::string label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
	{
		ImGui::PushID(label.c_str());
		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.c_str());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		if (ImGui::Button("X", buttonSize))
			values.x = resetValue;
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##x", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		if (ImGui::Button("Y", buttonSize))
			values.y = resetValue;
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		if (ImGui::Button("Z", buttonSize))
			values.z = resetValue;
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();
		ImGui::Columns(1);
		ImGui::PopID();
	}

	void SceneHierarchyPanel::drawComponents(Entity entity)
	{
		drawComponent<TagComponent>(entity, "Tag", [](Entity entity)
		{
			auto& tag = entity.getComponent<TagComponent>().tag;

			char buffer[256];
			memset(buffer, 0, sizeof(buffer));
			strcpy(buffer, tag.c_str());

			ImGui::Text("Tag"); ImGui::SameLine();
			if (ImGui::InputText("##label", buffer, sizeof(buffer)))
				tag = std::string(buffer);
		});

		drawComponent<TransformComponent>(entity, "Transform", [](Entity entity)
		{
			auto& tc = entity.getComponent<TransformComponent>();
			drawVec3Control("Translation", tc.translation);
			glm::vec3 rotation = glm::degrees(tc.rotation);
			drawVec3Control("Rotation", rotation);
			tc.rotation = glm::radians(rotation);
			drawVec3Control("Scale", tc.scale, 1.0f);
		});

		drawComponent<CameraComponent>(entity, "Camera", [](Entity entity)
		{
			auto& cameraComponent = entity.getComponent<CameraComponent>();
			auto& camera = cameraComponent.camera;

			const char* projectionTypeStrings[] = { "Perspective", "Orthographic" };
			int projectionType = static_cast<int>(camera.getProjectionType());
			const char* currProjectionTypeString = projectionTypeStrings[projectionType];

			ImGui::Text("Projection"); ImGui::SameLine();
			if (ImGui::BeginCombo("##label", currProjectionTypeString))
			{
				for (int i = 0; i < 2; i++)
				{
					bool isSelected = currProjectionTypeString == projectionTypeStrings[i];

					if (ImGui::Selectable(projectionTypeStrings[i], &isSelected))
					{
						currProjectionTypeString = projectionTypeStrings[i];
						camera.setProjectionType(static_cast<SceneCamera::ProjectionType>(i));
					}

					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			if (camera.getProjectionType() == SceneCamera::ProjectionType::Perspective)
			{
				float fov = camera.getPerspectiveVerticalFOV();
				if (ImGui::DragFloat("Vertical FOV", &fov))
					camera.setPerspectiveVerticalFOV(fov);

				float zNear = camera.getPerspectiveNearClip();
				if (ImGui::DragFloat("Near Clip", &zNear))
					camera.setPerspectiveNearClip(zNear);

				float zFar = camera.getPerspectiveFarClip();
				if (ImGui::DragFloat("Far Clip", &zFar))
					camera.setPerspectiveFarClip(zFar);
			}
			else
			{
				float orthoSize = camera.getOrthoSize();
				if (ImGui::DragFloat("Size", &orthoSize))
					camera.setOrthoSize(orthoSize);

				float orthoNear = camera.getOrthoNearClip();
				if (ImGui::DragFloat("Near Clip", &orthoNear))
					camera.setOrthoNearClip(orthoNear);

				float orthoFar = camera.getOrthoFarClip();
				if (ImGui::DragFloat("Far Clip", &orthoFar))
					camera.setOrthoFarClip(orthoFar);
			}
		});

		drawComponent<SpriteComponent>(entity, "Sprite", [](Entity entity)
		{
			auto& sc = entity.getComponent<SpriteComponent>();
			ImGui::Text("Color"); ImGui::SameLine();
			ImGui::ColorEdit4("##label", glm::value_ptr(sc.color));
		});
	}
}