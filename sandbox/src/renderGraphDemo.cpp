#include "renderGraphDemo.h"

#include <imgui/imgui.h>

using namespace Shadow;

RenderGraphDemo::RenderGraphDemo()
	: m_mangle("C:/dev/shadow/shadow/assets/meshes/mangle/fnaf_mangle.glb"), m_cameraController(45.0f, 1280.0f/720.0f)
{
	{
		m_cameraController.setCameraTranslationSpeed(5.0f);

		float squareVertices[] = {
			-1.0f,-1.0f,  0.0f,0.0f,
			-1.0f, 1.0f,  0.0f,1.0f,
			 1.0f, 1.0f,  1.0f,1.0f,
			 1.0f,-1.0f,  1.0f,0.0f
		};

		uint32_t indices[6] = { 0,1,2,2,3,0 };

		VertexInput quadDesc({ VertexAttribType::Vec2f, VertexAttribType::Vec2f });
		m_offscreenQuadVB = VertexBuffer::create(squareVertices, sizeof(squareVertices), quadDesc.getStride());
		m_quadIB = IndexBuffer::create(indices, 6);

		std::string assetsPath = "C:/dev/Shadow/Shadow/assets/";
		m_offscreenPass.shader = Renderer::getShaderLibrary().load("attachment_write", assetsPath + "shaders/lighting.vert.spv", assetsPath + "shaders/lighting.frag.spv");
		m_computeRead.shader = Shader::create("comp_read", assetsPath + "shaders/offscreen.vert.spv", assetsPath + "shaders/reversedColor.frag.spv");

		GraphicsPipeConfiguration pipeConfig{};
		pipeConfig.vertexInput = &m_mangle.getVertexInput();
		m_offscreenPass.pipeline = GraphicsPipeline::create(pipeConfig);

		pipeConfig.vertexInput = &quadDesc;
		pipeConfig.instanceInput = nullptr;
		m_computeRead.pipeline = GraphicsPipeline::create(pipeConfig);

		m_computeWrite.shader = Shader::create("compute_write", assetsPath + "shaders/particle.comp.spv");
		m_computeWrite.pipeline = ComputePipeline::create(m_computeWrite.shader);

		m_offscreenPass.ubo = UniformBuffer::create(m_offscreenPass.shader->getResource("u_light").size);
		m_offscreenPass.shader->setInput("u_light", m_offscreenPass.ubo);
		m_offscreenPass.shader->setInput("u_samplers", m_mangle);

		setupInputAttachmentRenderGraph();
		setupComputeRenderGraph();

		RenderGraphWriter writer;
		writer.begin("src/rendergraph.json");
		writer.writeData(m_computeRG);
		writer.writeData(m_inputAttachmentRG);
		writer.end();
	}
}

RenderGraphDemo::~RenderGraphDemo()
{
}

void RenderGraphDemo::onRender()
{
	auto& cmdBuffer = VulkanContext::getDevice()->getCmdBuffer();
	m_computeRG.execute(cmdBuffer);
	//m_inputAttachmentRG.execute(cmdBuffer);
}

void RenderGraphDemo::onImGuiRender()
{
	float frameRate = ShApp::get().getFrameRate();
	uint32_t fps = static_cast<uint32_t>(1000.0f / frameRate);

	ImGui::Begin("Renderer state");
	{
		ImGui::Text("FPS: %u", fps);
		ImGui::Text("Frame time: %f ms", frameRate);
		ImGui::ColorEdit3("Light color", &m_light.color.r);
	}
	ImGui::End();
}

void RenderGraphDemo::onUpdate(Shadow::Timestep ts)
{
	m_offscreenPass.ubo->setData_RT(&m_light, m_offscreenPass.ubo->getSize());
	m_cameraController.onUpdate(ts);
}

void RenderGraphDemo::setupComputeRenderGraph()
{
	m_light.position = glm::vec3(1.2f, 1.0f, 2.0f);
	m_light.color = { 1.0f,1.0f,1.0f };

	AttachmentInfo color, depth;
	color.format = ImageFormat::RGBA8_Unorm;
	color.loadOp = AttachmentLoadOp::Clear;
	depth.format = ImageFormat::Depth32f;
	depth.loadOp = AttachmentLoadOp::Clear;

	auto& cmdBuffer = VulkanContext::getDevice()->getCmdBuffer();

	auto offscreenPass = m_computeRG.addDrawPass("offscreen", m_offscreenPass.pipeline, m_offscreenPass.shader);
	offscreenPass->addColorOutput("inputColor", color);
	offscreenPass->setDepthStencilOutput("depth_out", depth);
	offscreenPass->setCallback([this, cmdBuffer]()
		{
			const Window& win = ShApp::get().getWindow();
			cmdBuffer->setViewport(0.0f, 0.0f, win.getWidth(), win.getHeight());
			cmdBuffer->setPushConstants(&m_cameraController.getCamera().getVPMatrix(), sizeof(glm::mat4), ShaderStage::Vertex);
			cmdBuffer->drawMesh(m_mangle);
		});

	auto computePass = m_computeRG.addComputePass("compute_pass", m_computeWrite.pipeline);
	computePass->addStorageInput("inputColor");
	computePass->addStorageOutput("inputColor");

	computePass->setCallback([this, cmdBuffer]()
		{
			const Ref<Texture2D>& texture = m_computeRG.getImageResource("inputColor").getTexture();
			cmdBuffer->dispatch(texture->getWidth() / 16, texture->getHeight() / 16, 1);
		});

	auto computeRead = m_computeRG.addDrawPass("compute_read", m_computeRead.pipeline, m_computeRead.shader);
	computeRead->addTextureInput("inputColor");
	computeRead->addColorOutput("storage_output", color);
	computeRead->setCallback([this, cmdBuffer]()
		{
			const Window& win = ShApp::get().getWindow();
			cmdBuffer->setViewport(0.0f, 0.0f, win.getWidth(), win.getHeight());
			cmdBuffer->drawIndexed(m_offscreenQuadVB, m_quadIB);
		});

	FramebufferInfo framebufferInfo{};
	ShApp::get().getWindow().getFramebufferSize(framebufferInfo.width, framebufferInfo.height);
	m_computeRG.setup(framebufferInfo);

	const Ref<Texture2D>& image = m_computeRG.getImageResource("inputColor").getTexture();
	m_computeWrite.shader->setInput("inputImage", image);
	m_computeWrite.shader->setInput("resultImage", image);
	m_computeRead.shader->setInput("u_sampler", image);
}

void RenderGraphDemo::setupInputAttachmentRenderGraph()
{
	std::string assetsPath = "C:/dev/Shadow/Shadow/assets/";
	m_attachmentRead.shader = Shader::create("attachment_read", assetsPath + "shaders/attachmentReadVert.spv", assetsPath + "shaders/attachmentReadFrag.spv");

	m_attachmentWritePipe = GraphicsPipeline::create(m_offscreenPass.pipeline->getConfiguration());

	GraphicsPipeConfiguration pipeConfig{};
	VertexInput quadDesc({ VertexAttribType::Vec2f });
	pipeConfig.vertexInput = &quadDesc;
	m_attachmentRead.pipeline = GraphicsPipeline::create(pipeConfig);

	float quadVertices[] = {
		-1.0f,-1.0f,
		-1.0f, 1.0f,
		 1.0f, 1.0f,
		 1.0f,-1.0f
	};
	m_inputAttachmentQuadVB = VertexBuffer::create(quadVertices, sizeof(quadVertices), quadDesc.getStride());

	AttachmentInfo color, depth;
	color.format = ImageFormat::RGBA8_Unorm;
	color.loadOp = AttachmentLoadOp::Clear;
	depth.format = ImageFormat::Depth32f;
	depth.loadOp = AttachmentLoadOp::Clear;

	auto& cmdBuffer = VulkanContext::getDevice()->getCmdBuffer();

	auto attachmentWrite = m_inputAttachmentRG.addDrawPass("attachment_write",  m_attachmentWritePipe, m_offscreenPass.shader);
	attachmentWrite->addColorOutput("input_color", color);
	attachmentWrite->setDepthStencilOutput("depth", depth);
	attachmentWrite->setCallback([this, cmdBuffer]()
		{
			const Window& win = ShApp::get().getWindow();
			cmdBuffer->setViewport(0.0f, 0.0f, win.getWidth(), win.getHeight());
			cmdBuffer->setPushConstants(&m_cameraController.getCamera().getVPMatrix(), sizeof(glm::mat4), ShaderStage::Vertex);
			cmdBuffer->drawMesh(m_mangle);
		});

	auto attachmentRead = m_inputAttachmentRG.addDrawPass("attachment_read", m_attachmentRead.pipeline, m_attachmentRead.shader);

	attachmentRead->addColorOutput("output_color", color);
	attachmentRead->addInputAttachment("input_color");
	attachmentRead->setCallback([this, cmdBuffer]()
		{
			const Window& win = ShApp::get().getWindow();
			cmdBuffer->setViewport(0.0f, 0.0f, win.getWidth(), win.getHeight());
			cmdBuffer->drawIndexed(m_inputAttachmentQuadVB, m_quadIB);
		});

	FramebufferInfo framebufferInfo{};
	ShApp::get().getWindow().getFramebufferSize(framebufferInfo.width, framebufferInfo.height);
	m_inputAttachmentRG.setup(framebufferInfo);

	m_attachmentRead.shader->setInput("inputColor", "input_color");
}


