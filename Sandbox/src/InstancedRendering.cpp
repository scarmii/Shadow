#include "InstancedRendering.hpp"

#include "imgui/imgui.h"
#include <vulkan/vulkan.h>

using namespace Shadow;

InstancedRendering::InstancedRendering()
	: m_cameraController(45.0f, 1280.0f/720.0f)
{
	m_cameraController.setCameraTranslationSpeed(30.0f);

	auto& window = ShEngine::get().getWindow();

	m_aspectRatio = static_cast<float>(window.getWidth()) / static_cast<float>(window.getHeight());
	m_lastX = window.getWidth() / 2.0f;
	m_lastY = window.getHeight() / 2.0f;

	FramebufferInfo info{};
	ShEngine::get().getWindow().getFramebufferSize(info.width, info.height);

	std::array<SubpassAttachment, 4> attachments{};
	attachments[0].ref = 0;
	attachments[0].format = ImageFormat::RGBA8;
	attachments[0].usage = AttachmentUsage::ColorAttachment | AttachmentUsage::SubpassInput;

	attachments[1].ref = 1;
	attachments[1].usage = AttachmentUsage::DepthAttachment;
	attachments[1].format = ImageFormat::Depth32f;

	Subpass subpasses[2];
	subpasses[0].colorAttachmentCount = 1;
	subpasses[0].pColorAttachments = &attachments[0];
	subpasses[0].pDepthAttachment = &attachments[1];

	subpasses[1].colorAttachmentCount = 0;
	subpasses[1].pDepthAttachment = nullptr;

	uint32_t inputAttachmentRef = 0;
	subpasses[1].inputAttachmentCount = 1;
	subpasses[1].pInputAttachmentRefs = &inputAttachmentRef;

	RenderpassConfig config{};
	config.subpassCount = 2;
	config.pSubpasses = subpasses;
	config.framebufferInfo = info;
	config.firstRenderpass = true;
	config.swapchainTarget = true;	   
	m_renderpass = Renderpass::create(config);

	std::string assetsPath = "C:/dev/Shadow/Shadow/assets/";
	m_shaders.attachmentWrite = Shader::create("attachmentWrite", assetsPath + "shaders/attachmentWriteVert.spv", assetsPath + "shaders/attachmentWriteFrag.spv");
	m_shaders.attachmentRead = Shader::create("attachmentRead", assetsPath + "shaders/attachmentReadVert.spv", assetsPath + "shaders/attachmentReadFrag.spv");

	float vertices[] = {
			+0.1f, +0.1f, +0.1f,	+0.0f, +0.9f, +0.9f,  // vertex 0
			-0.1f, +0.1f, +0.1f,	+0.0f, +0.9f, +0.9f,  // vertex 1
			+0.1f, -0.1f, +0.1f,	+0.0f, +0.9f, +0.9f,  // vertex 2
			-0.1f, -0.1f, +0.1f,	+0.0f, +0.9f, +0.9f,  // vertex 3

			+0.1f, +0.1f, +0.1f,	+0.1f, +0.1f, +1.0f,  // vertex 0
			+0.1f, -0.1f, +0.1f,	+0.1f, +0.1f, +1.0f,  // vertex 1
			+0.1f, +0.1f, -0.1f,	+0.1f, +0.1f, +1.0f,  // vertex 2
			+0.1f, -0.1f, -0.1f,	+0.1f, +0.1f, +1.0f,  // vertex 3

			+0.1f, +0.1f, +0.1f,	+0.1f, +0.1f, +0.1f,  // vertex 0
			+0.1f, +0.1f, -0.1f,	+0.1f, +0.1f, +0.1f,  // vertex 1
			-0.1f, +0.1f, +0.1f,	+0.1f, +0.1f, +0.1f,  // vertex 2
			-0.1f, +0.1f, -0.1f,	+0.1f, +0.1f, +0.1f,  // vertex 3

			+0.1f, +0.1f, -0.1f,	+1.0f, +0.1f, +0.1f,  // vertex 0
			+0.1f, -0.1f, -0.1f,	+1.0f, +0.1f, +0.1f,  // vertex 1
			-0.1f, +0.1f, -0.1f,	+1.0f, +0.1f, +0.1f,  // vertex 2
			-0.1f, -0.1f, -0.1f,	+1.0f, +0.1f, +0.1f,  // vertex 3

			-0.1f, +0.1f, +0.1f,	+0.1f, +1.0f, +0.1f,  // vertex 0
			-0.1f, +0.1f, -0.1f,	+0.1f, +1.0f, +0.1f,  // vertex 1
			-0.1f, -0.1f, +0.1f,	+0.1f, +1.0f, +0.1f,  // vertex 2
			-0.1f, -0.1f, -0.1f,	+0.1f, +1.0f, +0.1f,  // vertex 3

			+0.1f, -0.1f, +0.1f,	+0.7f, +0.7f, +0.7f,  // vertex 0
			-0.1f, -0.1f, +0.1f,	+0.7f, +0.7f, +0.7f,  // vertex 1
			+0.1f, -0.1f, -0.1f,	+0.7f, +0.7f, +0.7f,  // vertex 2
			-0.1f, -0.1f, -0.1f,	+0.7f, +0.7f, +0.7f   // vertex 3
	};

	m_vertexBuffers.attachmentWrite = VertexBuffer::create(vertices, sizeof(vertices), 24);


	std::vector<glm::vec3> transforms;
	const int start = -65 / 2;
	const int end = 65 / 2;

	for (int x = start; x <= end; x++)
	{
		for (int y = start; y <= end; y++)
		{
			for (int z = start; z <= end; z++)
			{
				glm::vec3 translation{};
				translation.x = (float)x * 5.0f;
				translation.y = (float)y * 5.0f;
				translation.z = (float)z * 5.0f;
				transforms.emplace_back(translation);
			}
		}
	}

	m_instanceCount = transforms.size();
	m_instanceBuffer = VertexBuffer::create(transforms.data(), transforms.size() * sizeof(glm::vec3), sizeof(glm::vec3));

	uint32_t indices[] = {
		// face 0:
		0,1,2,
		2,1,3,
		// face 1:
		4,5,6,
		6,5,7,
		// face 2:
		8,9,10,
		10,9,11,
		// face 3:
		12,13,14,
		14,13,15,
		// face 4:
		16,17,18,
		18,17,19,
		// face 5:
		20,21,22,
		22,21,23,
	};

	m_indexBuffers.attachmentWrite = IndexBuffer::create(indices, sizeof(indices)/sizeof(uint32_t));

	Shadow::VertexInput vertexDesc(
		{
			Shadow::VertexAttribType::Vec3f,
			Shadow::VertexAttribType::Vec3f
		});

	Shadow::VertexInput instanceDesc({ Shadow::VertexAttribType::Vec3f });

	Shadow::GraphicsPipeConfiguration pipeConfig{};
	pipeConfig.renderpass = m_renderpass;
	pipeConfig.subpass = 0;
	pipeConfig.shader = m_shaders.attachmentWrite;
	pipeConfig.vertexInput = &vertexDesc;
	pipeConfig.instanceInput = &instanceDesc;
	m_pipelines.attachmentWrite = GraphicsPipeline::create(pipeConfig);

	float quadVertices[] = {
		-1.0f,-1.0f,
		-1.0f, 1.0f,
		 1.0f, 1.0f,
		 1.0f, -1.0f
	};

	uint32_t quadIndices[] = { 0,1,2,2,3,0 };
#ifdef OLD
	m_vertexBuffers.attachmentRead = Shadow::VertexBuffer::create(quadVertices, sizeof(quadVertices), sizeof(float)*2, Shadow::BufferUsage::VertexBuffer);
	m_indexBuffers.attachmentRead = Shadow::IndexBuffer::create(quadIndices, sizeof(quadIndices) / sizeof(uint32_t), false);
#else
	m_vertexBuffers.attachmentRead = VertexBuffer::create(quadVertices, sizeof(quadVertices), sizeof(float) * 2);
	m_indexBuffers.attachmentRead = IndexBuffer::create(quadIndices, sizeof(quadIndices)/sizeof(uint32_t));
#endif

	Shadow::VertexInput quadDesc({ Shadow::VertexAttribType::Vec2f });

	pipeConfig.renderpass = m_renderpass;
	pipeConfig.subpass = 1;
	pipeConfig.shader = m_shaders.attachmentRead;
	pipeConfig.vertexInput = &quadDesc;
	pipeConfig.instanceInput = nullptr;
	m_pipelines.attachmentRead = GraphicsPipeline::create(pipeConfig);

	m_pipelines.attachmentRead->setSubpassInput("inputColor", 0);
}

InstancedRendering::~InstancedRendering()
{
}

void InstancedRendering::onUpdate(Shadow::Timestep ts)
{
	m_cameraController.onUpdate(ts);

	if (Shadow::Input::isMouseButtonPressed(SH_MOUSE_BUTTON_LEFT))
		Shadow::ShEngine::get().getWindow().setCursorMode(Shadow::CursorMode::Hidden);
	else if (Shadow::Input::isMouseButtonPressed(SH_MOUSE_BUTTON_RIGHT))
		Shadow::ShEngine::get().getWindow().setCursorMode(Shadow::CursorMode::Normal);
}

void InstancedRendering::onRender()
{
	auto& cmdBuffer = Renderer::getCmdBuffer();

	float width = static_cast<float>(ShEngine::get().getWindow().getWidth());
	float height = static_cast<float>(ShEngine::get().getWindow().getHeight());

	cmdBuffer->setViewport(0, 0, width, height);
	cmdBuffer->beginRenderPass(m_pipelines.attachmentWrite, &m_cameraController.getCamera().getVPMatrix());
	cmdBuffer->drawInstanced(m_vertexBuffers.attachmentWrite, m_instanceBuffer, m_indexBuffers.attachmentWrite, m_instanceCount);
	cmdBuffer->nextSubpass(m_pipelines.attachmentRead);
	cmdBuffer->drawIndexed(m_vertexBuffers.attachmentRead, m_indexBuffers.attachmentRead);
	cmdBuffer->endRenderPass();
}

void InstancedRendering::onImGuiRender() 
{
	float frameRate = Shadow::ShEngine::get().getFrameRate();
	uint32_t fps = static_cast<uint32_t>(1000.0f / frameRate);

	ImGui::Begin("Renderer state");
	ImGui::Text("FPS: %u", fps);
	ImGui::Text("Frame time: %f ms", frameRate);
	ImGui::Text("Instance count: %u", m_instanceCount);
	ImGui::Text("Instance count: %u", m_instanceCount);
	ImGui::Text("Vertices count: %u", m_vertexBuffers.attachmentWrite->getVertexCount() * m_instanceCount);
	ImGui::Text("Indices count: %u", m_indexBuffers.attachmentWrite->getCount() * m_instanceCount);
	ImGui::End();
}