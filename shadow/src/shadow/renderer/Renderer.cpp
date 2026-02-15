#include "shpch.h"
#include "shadow/core/core.h"	

#include "shadow/renderer/renderer.h"
#include "shadow/renderer/rendergraph.h"

namespace Shadow
{
	struct RendererData
	{
		static const uint32_t maxBatches = 100;
		static const uint32_t maxVertices = 100000000;
		static const uint32_t maxIndices = 100000000;
		static const uint32_t maxTextureSlots = 32; // TODO: RenderCapabilities

		ShaderLibrary shaderLib;
		bool synchronized = false;

		std::vector<Ref<RenderGraph>> rendergraphs;
		uint32_t exeRendergraphId = 0;
		Ref<Texture2D> imageOut = nullptr;

		Ref<CommandBuffer> transferCmdBuffer;
		Ref<Semaphore> transferAvailableSemaphore, transferCompleteSemaphore;

		Mesh::Vertex* vertexBufferData = nullptr;
		Mesh::Vertex* vertexBufferPtr = nullptr;

		Ref<Shader> meshShader = nullptr;
		Ref<GraphicsPipeline> meshPipeline = nullptr;
		Ref<VertexBuffer> meshVertexBuffer;
		Ref<IndexBuffer> meshIndexBuffer;

		Ref<Texture2D> whiteTexture = nullptr;
		std::array<Ref<Texture2D>, maxTextureSlots> textures;
		uint32_t textureIndex = 1; // 0 = white texture

		std::array<BufferRegion, maxBatches> bufferRegions;
		std::array<DrawArgs, maxBatches> drawArgs;

		uint32_t batchIndex = 0;
		uint32_t vertexOffset = 0;
		uint32_t indexCount = 0;
		uint32_t indexOffset = 0;
	};
	static RendererData* s_data;

	void Renderer::init()
	{
		s_data = new RendererData();

		s_data->transferAvailableSemaphore = Semaphore::create(true);
		s_data->transferCompleteSemaphore = Semaphore::create();

		s_data->transferCmdBuffer = CommandBuffer::create(QueueType::Transfer);
		s_data->transferCmdBuffer->addWaitSemaphore(s_data->transferAvailableSemaphore, PipelineStages::Transfer);
		s_data->transferCmdBuffer->addSignalSemaphore(s_data->transferCompleteSemaphore, PipelineStages::Transfer);

		s_data->whiteTexture = Texture2D::create(1, 1);
		uint32_t whiteTextureData = 0xffffffff;
		s_data->whiteTexture->setData(&whiteTextureData);
		s_data->textures[0] = s_data->whiteTexture;
		s_data->textureIndex = 1;

		s_data->meshVertexBuffer = VertexBuffer::create(sizeof(Mesh::Vertex) * RendererData::maxVertices, sizeof(Mesh::Vertex));
		s_data->vertexBufferData = new Mesh::Vertex[RendererData::maxVertices];
		s_data->vertexBufferPtr = s_data->vertexBufferData;

		Ref<RenderGraph> rendergraph = createRef<RenderGraph>();
		s_data->rendergraphs.emplace_back(rendergraph);

		std::string shaderPath = "C:/dev/shadow/shadow/assets/shaders/";
		s_data->meshShader = Renderer::getShaderLibrary().load("renderer_mesh_draw", shaderPath + "lighting.vert.spv", shaderPath + "lighting.frag.spv");

		VertexInput meshVertexInput{
			VertexAttribType::Vec3f,
			VertexAttribType::Vec3f,
			VertexAttribType::Vec2f,
			VertexAttribType::Uint
		};

		GraphicsPipeConfiguration meshPipeConfig{};
		meshPipeConfig.vertexInput = &meshVertexInput;
		s_data->meshPipeline = GraphicsPipeline::create(meshPipeConfig);

		AttachmentInfo color{};
		color.format = ImageFormat::RGBA8_Unorm;
		color.loadOp = AttachmentLoadOp::Clear;

		AttachmentInfo depth{};
		depth.format = ImageFormat::Depth32f;
		depth.loadOp = AttachmentLoadOp::Clear;

		auto& drawPass = rendergraph->addDrawPass("renderer_mesh_draw", s_data->meshPipeline, s_data->meshShader);
		ImageResource& colorRes = drawPass->addColorOutput("color_out", color);
		colorRes.addImageUsage(ImageUsage::StorageImage); // temp?
		drawPass->setDepthStencilOutput("depth_out", depth);

		rendergraph->setup(glm::vec4{ 1.0f,0.5f,0.9f,1.0f });
		s_data->imageOut = colorRes.getTexture();
	}

	void Renderer::shutdown()
	{
		delete s_data;
	}

	void Renderer::begin()
	{
		synchronizeRendering();

		s_data->indexOffset = 0;
		s_data->vertexOffset = 0;
		s_data->batchIndex = 0;
		s_data->textureIndex = 1;
		s_data->vertexBufferPtr = s_data->vertexBufferData;
	}

	void Renderer::end(const Camera& camera, const glm::mat4& transform)
	{
		SH_PROFILE_RENDERER_FUNCTION();

		s_data->meshShader->setInput("u_samplers", s_data->textureIndex, s_data->textures.data(), 0);
		flush();

		s_data->transferCmdBuffer->begin();
		s_data->meshVertexBuffer->setData(s_data->transferCmdBuffer, s_data->vertexBufferData, s_data->batchIndex, s_data->bufferRegions.data());
		s_data->transferCmdBuffer->end();
		s_data->transferCmdBuffer->submit();

		auto& cmdBuffer = VulkanContext::getDevice()->getCmdBuffer();
		glm::mat4 viewProj = camera.getProjection() * glm::inverse(transform);

		s_data->rendergraphs[s_data->exeRendergraphId]->getPass(0)->setCallback([viewProj, cmdBuffer]()
		{
			const Window& window = ShApp::get().getWindow();

			cmdBuffer->setViewport(0, 0, static_cast<float>(window.getWidth()), static_cast<float>(window.getHeight()));
			cmdBuffer->setPushConstants(&viewProj, sizeof(glm::mat4), ShaderStage::Vertex);

			for (uint32_t i = 0; i < s_data->batchIndex; i++)
			{
				//cmdBuffer->drawIndexed(s_data->vertexBuffer, s_data->indexBuffer, s_data->drawArgs[i]); // TODO
				cmdBuffer->drawIndexed(s_data->meshVertexBuffer, s_data->meshIndexBuffer);
			}
		});

		s_data->rendergraphs[s_data->exeRendergraphId]->execute(cmdBuffer);
	}

	void Renderer::flush()
	{
		uint32_t dataSize = static_cast<uint32_t>((uint8_t*)s_data->vertexBufferPtr - ((uint8_t*)s_data->vertexBufferData + s_data->vertexOffset));

		BufferRegion& region = s_data->bufferRegions[s_data->batchIndex];
		region.offset = s_data->vertexOffset;
		region.size = dataSize;

		DrawArgs args{};
		args.indexCount = s_data->indexCount;
		args.firstVertex = s_data->vertexOffset / sizeof(Mesh::Vertex);
		s_data->drawArgs[s_data->batchIndex] = args;

		s_data->indexOffset += s_data->indexCount;
		s_data->indexCount = 0;
		s_data->batchIndex++;
		s_data->vertexOffset += dataSize;
	}

	void Renderer::resizeFramebuffers(uint32_t width, uint32_t height)
	{
		for (auto& rendegraph : s_data->rendergraphs)
			rendegraph->resizeFramebuffers(width, height);
	}

	void Renderer::synchronizeRendering()
	{
		if (!s_data->synchronized)
		{
			s_data->synchronized = true;
			auto& cmdBuffer = VulkanContext::getDevice()->getCmdBuffer();
			cmdBuffer->addWaitSemaphore(s_data->transferCompleteSemaphore, PipelineStages::VertexInput);
			cmdBuffer->addSignalSemaphore(s_data->transferAvailableSemaphore, PipelineStages::VertexInput);
		}
	}

	void Renderer::drawMesh(const Ref<Mesh>& mesh, glm::mat4& trasnform)
	{
		SH_PROFILE_RENDERER_FUNCTION();

		auto& vertices = mesh->getVertices();
		memcpy(s_data->vertexBufferPtr, vertices.data(), vertices.size() * sizeof(Mesh::Vertex));
		s_data->vertexBufferPtr += vertices.size();

		s_data->meshIndexBuffer = mesh->getIndexBuffer();

		// TODO: indices handle

		for (auto& material : mesh->getTextures())
		{
			s_data->textures[s_data->textureIndex] = material;
			s_data->textureIndex++;
		}
	}

	uint32_t Renderer::addRenderGraph(const Ref<RenderGraph>& rendergraph)
	{
		s_data->rendergraphs.emplace_back(rendergraph);
		return static_cast<uint32_t>(s_data->rendergraphs.size() - 1);
	}

	void Renderer::setExeRenderGraph(uint32_t rendergraphId)
	{
		s_data->exeRendergraphId = rendergraphId;
	}

	void Renderer::setColorOutput(const Ref<Texture2D>& image)
	{
		s_data->imageOut = image;
	}

	ShaderLibrary& Renderer::getShaderLibrary()
	{
		return s_data->shaderLib;
	}

	RendererType Renderer::getRendererType()
	{
		return RendererType::Vulkan;
	}

	const Ref<Texture2D>& Renderer::getImageOut()
	{
		return s_data->imageOut;
	}

	const Ref<CommandBuffer>& Renderer::getTransferCmdBuffer()
	{
		return s_data->transferCmdBuffer;
	}
}