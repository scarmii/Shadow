#include "OffscreenRendering.hpp"

#include "Shadow/Vulkan/VulkanRenderpass.hpp"

#include<imgui/imgui.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/ext.hpp>

#include <vulkan/vulkan.h>

using namespace Shadow;

OffscreenRendering::OffscreenRendering()
	: m_mesh("C:/dev/Shadow/Shadow/assets/meshes/yuuka/scene.gltf"),
	m_mvp(1.0f), m_model(1.0f), m_cursorMode(CursorMode::Normal), m_cameraController(45.0f, 1280.0f / 720.0f)
{	
	m_cameraController.setCameraTranslationSpeed(5.0f);

	auto& window = ShEngine::get().getWindow();
	window.setCursorMode(m_cursorMode);

	FramebufferInfo framebufferInfo{};
	window.getFramebufferSize(framebufferInfo.width, framebufferInfo.height);

	// offscreen renderpass setup
	SubpassAttachment colorAttachment{};
	colorAttachment.ref = 0;
	colorAttachment.format = ImageFormat::RGBA8;
	colorAttachment.usage = AttachmentUsage::ColorAttachment | AttachmentUsage::RenderpassInput;

	SubpassAttachment depthAttachment{};
	depthAttachment.ref = 1;
	depthAttachment.usage = AttachmentUsage::DepthAttachment;
	depthAttachment.format = ImageFormat::Depth32f;
	
	Subpass subpass{};
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachment;
	subpass.pDepthAttachment = &depthAttachment;

	RenderpassConfig rpConfig{};
	rpConfig.subpassCount = 1;
	rpConfig.pSubpasses = &subpass;
	rpConfig.framebufferInfo = framebufferInfo;
	rpConfig.firstRenderpass = true;
	rpConfig.swapchainTarget = false;
	m_offscreenData.renderpass = Renderpass::create(rpConfig);

	std::string assetsPath = "C:/dev/Shadow/Shadow/assets/";

	m_offscreenData.shader = Renderer::getShaderLibrary().load("lighting", assetsPath + "shaders/lighting.vert.spv", assetsPath + "shaders/lighting.frag.spv");

	m_offscreenData.ubo = UniformBuffer::create(m_offscreenData.shader->getResource("u_light").size);
	m_offscreenData.shader->writeDescriptorSet("u_light", m_offscreenData.ubo);

	m_offscreenData.shader->writeDescriptorSet("u_samplers", m_mesh);

	m_light.position = glm::vec3(1.2f, 1.0f, 2.0f);
	m_light.color = { 1.0f,1.0f,1.0f };

	auto& vertInput = m_mesh.getVertexInput();

	GraphicsPipeConfiguration pipeConfig{};
	pipeConfig.renderpass = m_offscreenData.renderpass;
	pipeConfig.subpass = 0;
	pipeConfig.shader = m_offscreenData.shader;
	pipeConfig.vertexInput = &vertInput;
	pipeConfig.instanceInput = nullptr;
	m_offscreenData.pipe = GraphicsPipeline::create(pipeConfig);

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
#ifdef OLD
	m_squareVB = Shadow::VertexBuffer::create(squareVertices, sizeof(squareVertices), sizeof(float) * 4, Shadow::BufferUsage::VertexBuffer);
	m_squareIB = Shadow::IndexBuffer::create(indices, 6, false);
#else
	m_squareVB = VertexBuffer::create(squareVertices, sizeof(squareVertices), sizeof(float) * 4);
	m_squareIB = IndexBuffer::create(indices, 6);
#endif
}

void OffscreenRendering::onUpdate(Timestep ts)
{
	m_cameraController.onUpdate(ts);

	if (Input::isKeyPressed(SH_KEY_UP))
		m_model = glm::translate(m_model, glm::vec3(0.0f, 0.0f, -0.01f));
	if (Input::isKeyPressed(SH_KEY_DOWN))
		m_model = glm::translate(m_model, glm::vec3(0.0f, 0.0f, 0.01f));
	if (Input::isKeyPressed(SH_KEY_LEFT))
		m_model = glm::translate(m_model, glm::vec3(-0.01f, 0.0f, 0.0f));
	if (Input::isKeyPressed(SH_KEY_RIGHT))
		m_model = glm::translate(m_model, glm::vec3(0.01f, 0.0f, 0.0f));

	m_mvp = m_cameraController.getCamera().getVPMatrix() * m_model;
	m_offscreenData.ubo->setData_RT(&m_light, m_offscreenData.ubo->getSize());

	//if (Shadow::Input::isMouseButtonPressed(SH_MOUSE_BUTTON_1))
	//	Shadow::ShEngine::getInstance().getWindow().setCursorMode(Shadow::CursorMode::Hidden);
	//if (Shadow::Input::isMouseButtonPressed(SH_MOUSE_BUTTON_2))
	//	Shadow::ShEngine::getInstance().getWindow().setCursorMode(Shadow::CursorMode::Normal);
}

void OffscreenRendering::onRender()
{
	uint32_t width = 0, height = 0;
	ShEngine::get().getWindow().getFramebufferSize(width, height);

	Renderer::setViewport(0, 0, width, height);
	Renderer::beginRenderPass(m_offscreenData.pipe, &m_mvp);
	Renderer::drawMesh(m_mesh);
	Renderer::endRenderPass();

	if (m_postEffects == static_cast<uint32_t>(PostEffects::ReversedColor))
		renderPipeline(m_reversedColorPipe);
	else if (m_postEffects == static_cast<uint32_t>(PostEffects::BlackWhite))
		renderPipeline(m_blackWhitePipe);
	else
		renderPipeline(m_defaultPipe);
}

void OffscreenRendering::onImGuiRender()
{
	float frameRate = Shadow::ShEngine::get().getFrameRate();
	uint32_t fps = static_cast<uint32_t>(1000.0f / frameRate);

	ImGui::Begin("Renderer state");
	ImGui::Text("FPS: %u", fps);
	ImGui::Text("Frame time: %f ms", frameRate);
	ImGui::ColorEdit3("Light color", &m_light.color.r);
	ImGui::End();

	ImGui::Begin("Post Effects"); 
	{
		ImGui::RadioButton("None", &m_postEffects, 0);
		ImGui::RadioButton("Reversed color", &m_postEffects, 1);
		ImGui::RadioButton("Black & white", &m_postEffects, 2);
	}
	ImGui::End();
}

void OffscreenRendering::renderPipeline(const Pipeline& pipeline)
{
	Renderer::beginRenderPass(pipeline.graphicsPipeline);
	Renderer::drawIndexed(m_squareVB, m_squareIB);
	Renderer::endRenderPass();
}

OffscreenRendering::Pipeline OffscreenRendering::createPipeline(const FramebufferInfo& framebufferInfo, const std::string& fragShaderSpv)
{
	Pipeline pipe{};

	RenderpassConfig config{};
	config.subpassCount = 0;
	config.framebufferInfo = framebufferInfo;
	config.swapchainTarget = true;
	pipe.renderpass = Renderpass::create(config);

	pipe.shader = Shader::create("reversedColor", "C:/dev/Shadow/Shadow/assets/shaders/offscreen.vert.spv", fragShaderSpv);

	VertexInput squareVertexDesc({VertexAttribType::Vec2f, VertexAttribType::Vec2f });
	GraphicsPipeConfiguration pipeConfig{};
	pipeConfig.renderpass = pipe.renderpass;
	pipeConfig.subpass = 0;
	pipeConfig.shader = pipe.shader;
	pipeConfig.vertexInput = &squareVertexDesc;

	pipe.graphicsPipeline = GraphicsPipeline::create(pipeConfig);
	pipe.graphicsPipeline->setRenderpassInput("u_sampler", 0, m_offscreenData.renderpass);

	return pipe;
}