#include "editor.h"

#include "imgui/imgui.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/ext.hpp>
#include <imgui_internal.h>

#include <algorithm>

#define SH_EDITOR_2D
//#define BRUSH_TOOL

namespace Shadow
{
	Editor::Editor()
		: ShApp("shadow-editor"), m_mvp(1.0f), m_model(1.0f), m_cursorMode(CursorMode::Normal), 
		m_cameraControllers{{45.0f, 1280.0f / 720.0f}, {1280.0f / 720.0f, true}},
		m_meshes{createRef<Mesh>("C:/dev/shadow/shadow-editor/assets/meshes/kuromi_my_melody/scene.gltf"), 
			createRef<Mesh>("C:/dev/shadow/shadow-editor/assets/meshes/foxy/scene.gltf")}
	{
		EventDispatcher::get().addReciever(SH_ON_EVENT_FN(onWindowResizedEvent, WindowResizedEvent));

		m_cameraControllers.ortho.setCameraTranslationSpeed(1.0f);
		m_cameraControllers.perspective.setCameraTranslationSpeed(5.0f);
		m_cameraControllers.perspective.getCamera().setPosition({ 0.0f,0.75f,3.0f });

		auto& window = ShApp::get().getWindow();
		window.setCursorMode(m_cursorMode);

		m_light.position = glm::vec3(1.2f, 1.0f, 2.0f);
		m_light.color = { 1.0f,1.0f,1.0f };

		m_meshes.foxy->setMaterialIndexOffset(m_meshes.kuromi->getTextures().size());
		m_meshes.kuromi->setPositionOffset(glm::vec3{ -1.5f,0.0f,0.0f });

		GraphicsPipeConfiguration pipelineConfig{};
		pipelineConfig.vertexInput = &m_meshes.foxy->getVertexInput();
		m_offscreenPass.pipeline = GraphicsPipeline::create(pipelineConfig);
		m_offscreenPass.pipeline->setName("sh_editor_offscreen_pipeline");

		std::string assetsPath = "C:/dev/shadow/shadow-editor/";
		m_offscreenPass.shader = Renderer::getShaderLibrary().load("lighting", assetsPath + "assets/shaders/lighting.vert.spv", assetsPath + "assets/shaders/lighting.frag.spv");

		FramebufferInfo framebufferInfo{};
		window.getFramebufferSize(framebufferInfo.width, framebufferInfo.height);
		framebufferInfo.layers = 1;
		framebufferInfo.samples = 1;

		SubpassAttachment colorAttachment{};
		colorAttachment.name = "color";
		colorAttachment.format = ImageFormat::RGBA8_Unorm;
		colorAttachment.imageUsage = ImageUsage::ColorAttachment | ImageUsage::SampledImage | ImageUsage::StorageImage;
		colorAttachment.loadOp = AttachmentLoadOp::Clear;
		colorAttachment.finalLayout = ImageLayout::ShaderReadOnlyOptimal;

		SubpassAttachment depthAttachment{};
		depthAttachment.name = "depth";
		depthAttachment.format = ImageFormat::Depth32f;
		depthAttachment.imageUsage = ImageUsage::DepthAttachment;
		depthAttachment.loadOp = AttachmentLoadOp::Clear;
		depthAttachment.finalLayout = ImageLayout::DepthStencilAttachmentOptimal;

		Subpass subpass{};
		subpass.pipeline = m_offscreenPass.pipeline;
		subpass.shader = m_offscreenPass.shader;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachment;
		subpass.pDepthAttachment = &depthAttachment;

		RenderPassConfig renderPassConfig{};
		renderPassConfig.framebufferInfo = framebufferInfo;
		renderPassConfig.subpassCount = 1;
		renderPassConfig.pSubpasses = &subpass;
		renderPassConfig.firstRenderpass = true;
		m_offscreenPass.renderPass = RenderPass::create(renderPassConfig, "offscreen_pass");
		m_offscreenPass.renderPass->logRenderPassInfo();

		m_offscreenPass.ubo = UniformBuffer::create(m_offscreenPass.shader->getResource("u_light").size);
		m_offscreenPass.shader->setInput("u_light", m_offscreenPass.ubo);

		std::string vertShaderPath = "assets/shaders/offscreen.vert.spv";
		ShaderLibrary& shaderLib = Renderer::getShaderLibrary();

		m_computePass.shader = shaderLib.load("kernel_compute", assetsPath + "assets/shaders/particle.comp.spv");
		m_computePass.shader->setInput("inputImage", m_offscreenPass.renderPass->getImage("color"));
		m_computePass.shader->setInput("resultImage", m_offscreenPass.renderPass->getImage("color"));

		m_computePass.pipeline = ComputePipeline::create(m_computePass.shader);
		m_computePass.pipeline->setName("sh_editor_compute_pipeline");

#ifdef BRUSH_TOOL
		setupPaintToolRenderGraph();
#endif
		auto& imGuiLayer = ShApp::get().getImGuiLayer();

#ifndef SH_EDITOR_2D
		const Ref<Texture2D>& image = Renderer::getImageOut();
#else
		auto& image = Renderer2D::getColorOutput();
#endif
		m_imTextureID = imGuiLayer->addTexture(image);

		auto& foxyTextures = m_meshes.foxy->getTextures();
		auto& kuromiTextures = m_meshes.kuromi->getTextures();
		std::array<Ref<Texture2D>, 32> textureSlots;

		for (size_t i = 0; i < kuromiTextures.size(); i++)
			textureSlots[i] = kuromiTextures[i];

		for (size_t i = 0; i < foxyTextures.size(); i++)
			textureSlots[i + kuromiTextures.size()] = foxyTextures[i];

		// creating entities
		{
			std::string assetsPath = "C:/dev/shadow/shadow-editor/assets/";
			m_activeScene = createRef<Scene>();

			m_cameraEntity = m_activeScene->createEntity("camera A");
			m_cameraEntity.addComponent<CameraComponent>();

			m_secondCamera = m_activeScene->createEntity("camera B");
			m_secondCamera.addComponent<CameraComponent>();
			auto& cc = m_secondCamera.getComponent<CameraComponent>();
			cc.primary = false;

			m_squareEntity = m_activeScene->createEntity("square entity");
			m_squareEntity.addComponent<SpriteComponent>(glm::vec4{ 0.3f,0.1f,0.8f,1.0f });

			auto pinkSquare = m_activeScene->createEntity("pink square");
			pinkSquare.addComponent<SpriteComponent>(glm::vec4{ 1.0f,0.5f,1.0f,1.0f }/*, Texture2D::create(assetsPath + "textures/ghosty.png"*//*)*/);

			Entity meshEntity = m_activeScene->createEntity("mesh");
			auto& meshComponent = meshEntity.addComponent<MeshComponent>(assetsPath + "meshes/mangle/fnaf_mangle.glb");
		}

		class CameraController : public ScriptableEntity
		{
		public:
			void onCreate() 
			{
				auto& translation = getComponent<TransformComponent>().translation;
				/*translation.x = rand() % 10 - 5.0f;*/
			}

			void onDestroy()
			{
			}

			void onUpdate(Timestep ts)
			{
				auto& translation = getComponent<TransformComponent>().translation;
				float speed = 5.0f;

				if (Input::isKeyPressed(KeyCode::A))
					translation.x -= speed * ts;
				if (Input::isKeyPressed(KeyCode::D))
					translation.x += speed * ts;
				if (Input::isKeyPressed(KeyCode::W))
					translation.y += speed * ts;
				if (Input::isKeyPressed(KeyCode::S))
					translation.y -= speed * ts;
			}
		};

		m_cameraEntity.addComponent<NativeScriptComponent>().bind<CameraController>();
		m_sceneHierarchyPanel.setContext(m_activeScene);
	}

	Editor::~Editor()
	{
	}

	void Editor::onUpdate(Timestep ts)
	{
#ifndef SH_EDITOR_2D
		m_cameraControllers.perspective.onUpdate(ts);
#else
		m_cameraControllers.ortho.onUpdate(ts);
#endif

		m_activeScene->onViewportResize(static_cast<uint32_t>(m_viewportSize.x), static_cast<uint32_t>(m_viewportSize.y));
		m_activeScene->onUpdate(ts);

		if (Input::isKeyPressed(KeyCode::Up))
			m_model = glm::translate(m_model, glm::vec3(0.0f, 0.0f, -0.01f));
		if (Input::isKeyPressed(KeyCode::Down))
			m_model = glm::translate(m_model, glm::vec3(0.0f, 0.0f, 0.01f));
		if (Input::isKeyPressed(KeyCode::Left))
			m_model = glm::translate(m_model, glm::vec3(-0.01f, 0.0f, 0.0f));
		if (Input::isKeyPressed(KeyCode::Right))
			m_model = glm::translate(m_model, glm::vec3(0.01f, 0.0f, 0.0f));
	}

	void Editor::onRender()
	{
#ifndef SH_EDITOR_2D
		//auto& cmdBuffer = VulkanContext::getDevice()->getCmdBuffer();
		//auto& window = ShApp::get().getWindow();
		//auto& image = m_offscreenPass.renderPass->getImage("color");

		//cmdBuffer->beginRenderPass(m_offscreenPass.renderPass);
		//cmdBuffer->setViewport(0, 0, window.getWidth(), window.getHeight());
		//cmdBuffer->setPushConstants(&m_cameraControllers.perspective.getCamera().getVPMatrix(), sizeof(glm::mat4), ShaderStage::Vertex);

		m_activeScene->renderScene();

		/*cmdBuffer->endRenderPass();

		if (m_settings.computePass)
		{
			cmdBuffer->transitionImageLayout(image,
				ImageLayout::ShaderReadOnlyOptimal, ImageLayout::General,
				PipelineStages::ColorAttachmentOutput, PipelineStages::ComputeShader,
				AccessFlags::ColorAttachmentWrite, AccessFlags::ShaderRead);

			cmdBuffer->transitionImageLayout(image,
				ImageLayout::General, ImageLayout::General,
				PipelineStages::ComputeShader, PipelineStages::ComputeShader,
				AccessFlags::ShaderRead, AccessFlags::ShaderWrite);

			cmdBuffer->beginComputePass(m_computePass.pipeline);
			cmdBuffer->dispatch(image->getWidth() / 16, image->getHeight() / 16, 1);
			cmdBuffer->endComputePass();

			cmdBuffer->transitionImageLayout(image,
				ImageLayout::General, ImageLayout::ShaderReadOnlyOptimal,
				PipelineStages::ComputeShader, PipelineStages::FragmentShader,
				AccessFlags::ShaderWrite, AccessFlags::ShaderRead);
		}*/
#else
		Renderer2D::resetStats();
		m_activeScene->renderScene();

#ifdef BRUSH_TOOL
		if (Input::isMouseButtonPressed(MouseCode::ButtonLeft))
			m_paintTool.onRender();
#endif
#endif
	}

	void Editor::onImGuiRender()
	{
		SH_PROFILE_FUNCTION();

		float frameRate = Shadow::ShApp::get().getFrameRate();
		uint32_t fps = static_cast<uint32_t>(1000.0f / frameRate);

		static bool p_open = true;
		static bool opt_fullscreen = true;
		static bool opt_padding = true;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

		ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
		m_sceneHierarchyPanel.onImGuiRender();

#ifndef SH_EDITOR_2D
		ImGui::Begin("Draw settings");
		{
			ImGui::Checkbox("compute pass", &m_settings.computePass);
		}
		ImGui::End();

		ImGui::Begin("Renderer state");
		{
			ImGui::Text("FPS: %u", fps);
			ImGui::Text("Frame time: %f ms", frameRate);
			ImGui::ColorEdit3("Light color", &m_light.color.r);
		}
		ImGui::End();
#else

#ifdef BRUSH_TOOL
		m_paintTool.onImGuiRender();
#endif

		ImGui::Begin("Stats");
		{
			auto& stats = Renderer2D::getStats();
			ImGui::Text("Renderer2D Stats: ");
			ImGui::Text("Draw Calls: %u", stats.drawCall);
			ImGui::Text("Quads: %u", stats.quadCount);
			ImGui::Text("Vertices: %u", stats.getTotalVertexCount());
			ImGui::Text("Indices: %u", stats.getTotalIndexCount());

			ImGui::Text("FPS: %u", fps);
			ImGui::Text("Frame Time: %f ms", frameRate);
		}
		ImGui::End();
#endif
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f,0.0f));
		ImGui::Begin("Viewport");
		{
			bool windowFocused = ImGui::IsWindowFocused();
			bool windowHovered = ImGui::IsWindowHovered();

			auto& imguiLayer = ShApp::get().getImGuiLayer();
			imguiLayer->blockEvents(!windowFocused || !windowHovered);

			ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
			if (m_viewportSize != *(glm::vec2*)&viewportPanelSize)
			{
				m_viewportSizeChanged = true;
				m_viewportSize = { viewportPanelSize.x, viewportPanelSize.y };

				if (viewportPanelSize.x != 0 && viewportPanelSize.y != 0)
				{

#ifndef SH_EDITOR_2D
					const Ref<Texture2D>& image = Renderer::getImageOut();
					Renderer::resizeFramebuffers(static_cast<uint32_t>(m_viewportSize.x), static_cast<uint32_t>(m_viewportSize.y));
#else
					auto& image = Renderer2D::getColorOutput();
					Renderer2D::resizeFramebuffers(static_cast<uint32_t>(m_viewportSize.x), static_cast<uint32_t>(m_viewportSize.y));
#endif
					imguiLayer->removeTexture(m_imTextureID);
					m_imTextureID = imguiLayer->addTexture(image);
					
					imguiLayer->getCommandBuffer()->transitionImageLayout(image,
						ImageLayout::Undefined, ImageLayout::ShaderReadOnlyOptimal,
						PipelineStages::TopOfPipe, PipelineStages::FragmentShader,
						AccessFlags::None, AccessFlags::ShaderRead);

#ifdef BRUSH_TOOL
					m_paintTool.setShaderInputImage();
#endif
#ifndef SH_EDITOR_2D
					m_computePass.shader->setInput("inputImage", image);
					m_computePass.shader->setInput("resultImage", image);
#endif
				}
			}
			else
				m_viewportSizeChanged = false;

			ImGui::Image(m_imTextureID, { viewportPanelSize.x, viewportPanelSize.y });

#ifdef BRUSH_TOOL
			m_paintTool.onViewportRender();
#endif
		}
		ImGui::End();
		ImGui::PopStyleVar(2);
	}

	bool Editor::onWindowResizedEvent(const WindowResizedEvent& e)
	{
		auto& imguiLayer = ShApp::get().getImGuiLayer();
		imguiLayer->removeTexture(m_imTextureID);

#ifndef SH_EDITOR_2D
		const Ref<Texture2D>& image = Renderer::getImageOut();
#else
		auto& image = Renderer2D::getColorOutput();
#endif
		m_imTextureID = imguiLayer->addTexture(image);
		return false;
	}

	void Editor::setupPaintToolRenderGraph()
	{
		AttachmentInfo color{};
		color.format = ImageFormat::RGBA8_Unorm;
		color.loadOp = AttachmentLoadOp::Load;

		AttachmentInfo depth{};
		depth.format = ImageFormat::Depth32f;
		depth.loadOp = AttachmentLoadOp::Clear;

		Ref<RenderGraph> rendergraph = createRef<RenderGraph>();
		auto& drawPass = rendergraph->addDrawPass("paint_pass", Renderer2D::getGraphicsPipeline(), Renderer2D::getShader());
		ImageResource& colorRes = drawPass->addColorOutput("color", color);
		colorRes.addImageUsage(ImageUsage::StorageImage);
		drawPass->setDepthStencilOutput("depth", depth);
		rendergraph->setup();

		uint32_t id = Renderer2D::addRenderGraph(rendergraph);
		Renderer2D::setExeRenderGraph(id);
		Renderer2D::setColorOutput(colorRes.getTexture());
	}
}