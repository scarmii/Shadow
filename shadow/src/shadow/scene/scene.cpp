#include "shpch.h"
#include "scene.h"
#include "components.h"
#include "entity.h"

#include "shadow/renderer/renderer2D.h"
#include "shadow/renderer/renderer.h"

#include <glm/glm.hpp>

#define SH_EDITOR_2D

namespace Shadow
{
	Scene::Scene()
	{
	}

	Scene::~Scene()
	{
	}

	Entity Scene::createEntity(const std::string& name)
	{
		Entity entity = { m_registry.create(), this };
		entity.addComponent<TransformComponent>();
		auto& tag = entity.addComponent<TagComponent>();
		tag.tag = name.empty() ? "entity" : name;
		return entity;
	}

	void Scene::onUpdate(Timestep ts)
	{
		// update scripts
		{
			m_registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
				{
					// TODO: move to Scene::onScenePlay
					if (!nsc.pInstance)
					{
						nsc.pInstance = nsc.instantiateScript();
						nsc.pInstance->m_entity = Entity{ entity, this };
						nsc.pInstance->onCreate();
					}

					nsc.pInstance->onUpdate(ts);
			});
		}
	}

	void Scene::renderScene()
	{
		Camera* mainCamera = nullptr;
		glm::mat4 cameraTransform;

		auto view = m_registry.view<TransformComponent, CameraComponent>();
		for (auto entity : view)
		{
			auto [transform, camera] = view.get<TransformComponent, CameraComponent>(entity);

			if (camera.primary)
			{
				mainCamera = &camera.camera;
				cameraTransform = transform.getTransform();
				break;
			}
		}
#ifdef SH_EDITOR_2D
		if (mainCamera)
		{
			Renderer2D::beginScene();

			auto group = m_registry.group<TransformComponent>(entt::get<SpriteComponent>);
			for (auto entity : group)
			{
				auto [transform, sprite] = group.get<TransformComponent, SpriteComponent>(entity);
				Renderer2D::drawQuad(transform.getTransform(), sprite.color, sprite.texture);
			}

			Renderer2D::endScene(*mainCamera, cameraTransform);
		}
#else
		Renderer::begin();

		auto meshView = m_registry.group<TransformComponent>(entt::get<MeshComponent>);
		for (auto entity : meshView)
		{
			auto [transform, mesh] = meshView.get<TransformComponent, MeshComponent>(entity);
			Renderer::drawMesh(mesh.mesh, transform.getTransform());
		}

		Renderer::end(*mainCamera, cameraTransform);
#endif
	}

	void Scene::onViewportResize(uint32_t width, uint32_t height)
	{
		m_viewportWidth = width;
		m_viewportHeight = height;

		m_registry.view<CameraComponent>().each([=](auto entity, auto& cc)
		{
			if (!cc.fixedAspectRatio)
				cc.camera.setViewportSize(width, height);
		});
	}
}