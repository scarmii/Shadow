#include "offscreen.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <backends/imgui_impl_vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/ext.hpp>

using namespace Shadow;

OffscreenDemo::OffscreenDemo()
	: m_mesh("C:/dev/Shadow/Shadow/assets/meshes/yuuka/scene.gltf"),
	m_mvp(1.0f), m_model(1.0f), m_cursorMode(CursorMode::Normal), m_cameraController(45.0f, 1280.0f / 720.0f)
{	
	m_cameraController.setCameraTranslationSpeed(5.0f);

	auto& window = ShApp::get().getWindow();
	window.setCursorMode(m_cursorMode);

	FramebufferInfo framebufferInfo{};
	window.getFramebufferSize(framebufferInfo.width, framebufferInfo.height);

	std::string assetsPath = "C:/dev/Shadow/Shadow/assets/";

	m_offscreenData.shader = Renderer::getShaderLibrary().load("lighting", assetsPath + "shaders/lighting.vert.spv", assetsPath + "shaders/lighting.frag.spv");
	m_offscreenData.ubo = UniformBuffer::create(m_offscreenData.shader->getResource("u_light").size);
	m_offscreenData.shader->setInput("u_light", m_offscreenData.ubo);
	m_offscreenData.shader->setInput("u_samplers", m_mesh);

	m_light.position = glm::vec3(1.2f, 1.0f, 2.0f);
	m_light.color = { 1.0f,1.0f,1.0f };

	auto& vertInput = m_mesh.getVertexInput();

	GraphicsPipeConfiguration pipeConfig{};
	pipeConfig.vertexInput = &vertInput;
	pipeConfig.instanceInput = nullptr;
	m_offscreenData.pipe = GraphicsPipeline::create(pipeConfig);

	// offscreen renderpass setup
	SubpassAttachment colorAttachment{};
	colorAttachment.name = "offscreen";
	colorAttachment.format = ImageFormat::RGBA8_Unorm;
	colorAttachment.imageUsage = ImageUsage::ColorAttachment | ImageUsage::SampledImage| ImageUsage::StorageImage;
	colorAttachment.finalLayout = ImageLayout::ShaderReadOnlyOptimal;

	SubpassAttachment depthAttachment{};
	depthAttachment.name = "depth";
	depthAttachment.imageUsage = ImageUsage::DepthAttachment;
	depthAttachment.format = ImageFormat::Depth32f;
	depthAttachment.finalLayout = ImageLayout::DepthStencilAttachmentOptimal;

	Subpass subpass{};
	subpass.pipeline = m_offscreenData.pipe;
	subpass.shader = m_offscreenData.shader;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachment;
	subpass.pDepthAttachment = &depthAttachment;

	RenderPassConfig rpConfig{};
	rpConfig.subpassCount = 1;
	rpConfig.pSubpasses = &subpass;
	rpConfig.framebufferInfo = framebufferInfo;
	rpConfig.firstRenderpass = true;
	m_offscreenData.renderpass = RenderPass::create(rpConfig, "offscreen");

	m_defaultPipe = createPipeline(framebufferInfo, assetsPath + "shaders/offscreen.frag.spv");
	m_reversedColorPipe = createPipeline(framebufferInfo, assetsPath + "shaders/reversedColor.frag.spv");
	m_blackWhitePipe = createPipeline(framebufferInfo, assetsPath + "shaders/blackWhite.frag.spv");

	float squareVertices[] = {
		-1.0f,-1.0f,  0.0f,0.0f,
		-1.0f, 1.0f,  0.0f,1.0f,
		 1.0f, 1.0f,  1.0f,1.0f,
		 1.0f,-1.0f,  1.0f,0.0f
	};

	uint32_t indices[6] = { 0,1,2,2,3,0 };

	m_squareVB = VertexBuffer::create(squareVertices, sizeof(squareVertices), sizeof(float) * 4);
	m_squareIB = IndexBuffer::create(indices, 6);

	m_computeShader = Shader::create("compute", "C:/dev/Shadow/Shadow/assets/shaders/particle.comp.spv");
	m_computePipe = ComputePipeline::create(m_computeShader);

	m_computeShader->setInput("inputImage", m_offscreenData.renderpass->getImage("offscreen"));
	m_computeShader->setInput("resultImage", m_offscreenData.renderpass->getImage("offscreen"));

	const Ref<Texture2D>& image = m_offscreenData.renderpass->getImage(0);
	m_imguiTextureID = (ImTextureID)ImGui_ImplVulkan_AddTexture(image->getSampler(), image->getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void OffscreenDemo::onUpdate(Timestep ts)
{
	m_cameraController.onUpdate(ts);

	if (Input::isKeyPressed(KeyCode::Up))
		m_model = glm::translate(m_model, glm::vec3(0.0f, 0.0f, -0.01f));
	if (Input::isKeyPressed(KeyCode::Down))
		m_model = glm::translate(m_model, glm::vec3(0.0f, 0.0f, 0.01f));
	if (Input::isKeyPressed(KeyCode::Left))
		m_model = glm::translate(m_model, glm::vec3(-0.01f, 0.0f, 0.0f));
	if (Input::isKeyPressed(KeyCode::Right))
		m_model = glm::translate(m_model, glm::vec3(0.01f, 0.0f, 0.0f));

	m_mvp = m_cameraController.getCamera().getVPMatrix() * m_model;
	m_offscreenData.ubo->setData_RT(&m_light, m_offscreenData.ubo->getSize());

	//if (Shadow::Input::isMouseButtonPressed(SH_MOUSE_BUTTON_1))
	//	Shadow::ShEngine::getInstance().getWindow().setCursorMode(Shadow::CursorMode::Hidden);
	//if (Shadow::Input::isMouseButtonPressed(SH_MOUSE_BUTTON_2))
	//	Shadow::ShEngine::getInstance().getWindow().setCursorMode(Shadow::CursorMode::Normal);
}

void OffscreenDemo::onRender()
{
	auto& renderCmdBuffer = VulkanContext::getDevice()->getCmdBuffer();
	auto& window = ShApp::get().getWindow();
	
	renderCmdBuffer->setViewport(0, 0, window.getWidth(), window.getHeight());
	renderCmdBuffer->beginRenderPass(m_offscreenData.renderpass);
	renderCmdBuffer->setPushConstants(&m_cameraController.getCamera().getVPMatrix(), sizeof(glm::mat4), ShaderStage::Vertex);
	renderCmdBuffer->drawMesh(m_mesh);
	renderCmdBuffer->endRenderPass();

	auto& image = m_offscreenData.renderpass->getImage(0);

	renderCmdBuffer->transitionImageLayout(image,
		ImageLayout::ShaderReadOnlyOptimal, ImageLayout::General,
		PipelineStages::ColorAttachmentOutput, PipelineStages::ComputeShader,
		AccessFlags::ColorAttachmentWrite, AccessFlags::ShaderRead);

	renderCmdBuffer->transitionImageLayout(image,
		ImageLayout::General, ImageLayout::General,
		PipelineStages::ComputeShader, PipelineStages::ComputeShader,
		AccessFlags::ShaderRead, AccessFlags::ShaderWrite);

	renderCmdBuffer->beginComputePass(m_computePipe);
	renderCmdBuffer->dispatch(image->getWidth() / 16, image->getHeight() / 16, 1);
	renderCmdBuffer->endComputePass();

	renderCmdBuffer->transitionImageLayout(image,
		ImageLayout::General, ImageLayout::ShaderReadOnlyOptimal,
		PipelineStages::ComputeShader, PipelineStages::FragmentShader,
		AccessFlags::ShaderWrite, AccessFlags::ShaderRead);

	renderCmdBuffer->beginRenderPass(m_defaultPipe.renderpass);
	renderCmdBuffer->drawIndexed(m_squareVB, m_squareIB);
	renderCmdBuffer->endRenderPass();

	//if (m_postEffects == static_cast<uint32_t>(PostEffects::ReversedColor))
	//	renderPipeline(m_reversedColorPipe);
	//else if (m_postEffects == static_cast<uint32_t>(PostEffects::BlackWhite))
	//	renderPipeline(m_blackWhitePipe);
	//else
	//	renderPipeline(m_defaultPipe);
}

void OffscreenDemo::onImGuiRender()
{
	SH_PROFILE_FUNCTION();

	float frameRate = Shadow::ShApp::get().getFrameRate();
	uint32_t fps = static_cast<uint32_t>(1000.0f / frameRate);

	static bool p_open = true;
	static bool opt_fullscreen = true;
	static bool opt_padding = true;
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

	// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
	// because it would be confusing to have two docking targets within each others.
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	if (opt_fullscreen)
	{
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	}
	else
	{
		dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
	}

	// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
	// and handle the pass-thru hole, so we ask Begin() to not render a background.
	if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
		window_flags |= ImGuiWindowFlags_NoBackground;

	// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
	// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
	// all active windows docked into it will lose their parent and become undocked.
	// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
	// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
	if (!opt_padding)
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace Demo", &p_open, window_flags);
	if (!opt_padding)
		ImGui::PopStyleVar();

	if (opt_fullscreen)
		ImGui::PopStyleVar(2);

	// Submit the DockSpace
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
	}

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit")) ShApp::get().close();
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	ImGui::Begin("Post Effects");
	{
		ImGui::RadioButton("None", &m_postEffects, 0);
		ImGui::RadioButton("Reversed color", &m_postEffects, 1);
		ImGui::RadioButton("Black & white", &m_postEffects, 2);
	}
	ImGui::End();

	ImGui::Begin("Renderer state");
	ImGui::Text("FPS: %u", fps);
	ImGui::Text("Frame time: %f ms", frameRate);
	ImGui::ColorEdit3("Light color", &m_light.color.r);

	Ref<Texture2D> tex = m_offscreenData.renderpass->getImage(0);
	ImGui::Image(m_imguiTextureID, { static_cast<float>(tex->getWidth()), static_cast<float>(tex->getHeight())});

	ImGui::ShowDemoWindow();
	ImGui::End();

	ImGui::End();
}

void OffscreenDemo::renderPipeline(Pipeline& pipeline) const
{
	auto& drawCmdBuffer = VulkanContext::getDevice()->getCmdBuffer();
	drawCmdBuffer->beginRenderPass(pipeline.renderpass);
	drawCmdBuffer->drawIndexed(m_squareVB, m_squareIB);
	drawCmdBuffer->endRenderPass();
}

OffscreenDemo::Pipeline OffscreenDemo::createPipeline(const FramebufferInfo& framebufferInfo, const std::string& fragShaderSpv) const
{
	Pipeline pipe{};
	pipe.shader = Shader::create("reversedColor", "C:/dev/Shadow/Shadow/assets/shaders/offscreen.vert.spv", fragShaderSpv);

	VertexInput squareVertexDesc({VertexAttribType::Vec2f, VertexAttribType::Vec2f });
	GraphicsPipeConfiguration pipeConfig{};
	pipeConfig.vertexInput = &squareVertexDesc;

	pipe.m_graphicsPipeline = GraphicsPipeline::create(pipeConfig);
	pipe.shader->setInput("u_sampler", m_offscreenData.renderpass->getImage(0));

	SubpassAttachment colorAttachment{};
	colorAttachment.name = "swapchain_target";
	colorAttachment.loadOp = AttachmentLoadOp::Clear;
	colorAttachment.swapchainTarget = true;

	Subpass subpass{};
	subpass.pipeline = pipe.m_graphicsPipeline;
	subpass.shader = pipe.shader;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachment;

	RenderPassConfig config{};
	config.subpassCount = 1;
	config.pSubpasses = &subpass;
	config.framebufferInfo = framebufferInfo;
	pipe.renderpass = RenderPass::create(config);

	return pipe;
}