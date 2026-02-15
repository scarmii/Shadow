#pragma once

#define SH_ALLOC_TRACE
#define SHADOW_ENTRY
#include "shadow.h"
#include "panels/sceneHierarchyPanel.h"
#include "brushTool.h"

#include <imgui/imgui.h>

namespace Shadow
{
	enum class PostEffects
	{
		None = 0,
		ReversedColor = 1,
		BlackWhite = 2
	};

	struct Light
	{
		alignas(16) glm::vec3 position;
		alignas(16) glm::vec3 color;
	};

	class Editor : public ShApp
	{
	public:
		Editor();
		~Editor();

		virtual void onUpdate(Shadow::Timestep ts) override;
		virtual void onRender() override;
		virtual void onImGuiRender() override;
	private:
		bool onWindowResizedEvent(const WindowResizedEvent& e);
		void setupPaintToolRenderGraph();
	private:
		Light m_light{};
		glm::mat4 m_mvp, m_model;

		Ref<Scene> m_activeScene;
		Entity m_squareEntity;
		Entity m_cameraEntity;
		Entity m_secondCamera;
		bool m_primaryCamera = true;
		PaintTool m_paintTool;
		CursorMode m_cursorMode;

		struct CameraControllers
		{
			PerspectiveCameraController perspective;
			OrthoCameraController ortho;
		} m_cameraControllers;

		glm::vec2 m_viewportSize;
		bool m_viewportSizeChanged = false;
		ImTextureID m_imTextureID;

		// panels
		SceneHierarchyPanel m_sceneHierarchyPanel;

		struct Meshes
		{
			Ref<Mesh> kuromi;
			Ref<Mesh> foxy;
		} m_meshes;

		struct OffscreenData
		{
			Ref<GraphicsPipeline> pipeline;
			Ref<Shader> shader;
			Ref<UniformBuffer> ubo;
			Ref<RenderPass> renderPass;
		} m_offscreenPass;

		struct ComputePass
		{
			Ref<Shader> shader;
			Ref<ComputePipeline> pipeline;
		} m_computePass;

		Ref<VertexBuffer> m_quadVertexBuffer;
		Ref<IndexBuffer> m_quadIndexBuffer;

		struct DrawSettings
		{
			bool computePass;
		} m_settings;
	};

	ShApp* createApp()
	{
		return new Editor();
	}
}