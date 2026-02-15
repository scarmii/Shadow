#pragma once

#include "sceneCamera.h"
#include "entity.h"
#include "shadow/renderer/mesh.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Shadow
{
	struct TagComponent
	{
		std::string tag;

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string& tag)
			: tag(tag) {}
	};

	struct TransformComponent
	{
		glm::vec3 translation = { 0.0f,0.0f,0.0f };
		glm::vec3 rotation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 scale = { 1.0f,1.0f,1.0f };

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::vec3& translation)
			: translation(translation) {}

		glm::mat4 getTransform() const
		{
			glm::mat4 rot = glm::rotate(glm::mat4(1.0f), rotation.x, { 1, 0, 0 })
				* glm::rotate(glm::mat4(1.0f), rotation.y, { 0, 1, 0 })
				* glm::rotate(glm::mat4(1.0f), rotation.z, { 0, 0, 1 });

			return glm::translate(glm::mat4(1.0f), translation) * rot * glm::scale(glm::mat4(1.0f), scale);
		}
	};

	struct CameraComponent
	{
		SceneCamera camera;
		bool primary = true; // TODO: moving to Scene?
		bool fixedAspectRatio = false;

		CameraComponent() = default;
		CameraComponent(const CameraComponent&) = default;
	};

	struct SpriteComponent
	{
		glm::vec4 color{ 1.0f,1.0f,1.0f,1.0f };
		Ref<Texture2D> texture = nullptr;

		SpriteComponent() = default;
		SpriteComponent(const SpriteComponent&) = default;

		SpriteComponent(const glm::vec4 color)
			: color(color) {}

		SpriteComponent(const Ref<Texture2D>& texture)
			: texture(texture) {}

		SpriteComponent(const glm::vec4& color, const Ref<Texture2D>& texture)
			: color(color), texture(texture) {}
	};

	struct MeshComponent
	{
		Ref<Mesh> mesh;

		MeshComponent(const MeshComponent&) = default;

		MeshComponent(const std::string& path)
			: mesh(createRef<Mesh>(path)) {}

		MeshComponent(const Ref<Mesh>& mesh)
			: mesh(mesh) {}
	};

	struct NativeScriptComponent
	{
		ScriptableEntity* pInstance = nullptr;

		ScriptableEntity* (*instantiateScript)();
		void (*destroyScript)(NativeScriptComponent*);

		template<typename T>
		void bind()
		{
			instantiateScript = []() { return static_cast<ScriptableEntity*>(new T()); };
			destroyScript = [](NativeScriptComponent* nsc) { delete nsc->pInstance; nsc->pInstance = nullptr; };
		}
	};
}