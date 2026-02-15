#include "shpch.h"
#include "shadow/core/core.h"
#include "shadow/core/shApp.h"

#include "shadow/renderer/renderer2D.h"
#include "shadow/renderer/rendergraph.h"
#include "shadow/vulkan/renderPass.h"
#include "shadow/utils/rendergraphWriter.h"
#include "shadow/renderer/renderer.h"

#include <glm/ext.hpp>
#include <unordered_map>

#define RENDERER2D_USE_RENDERGRAPH

namespace Shadow
{
	struct QuadVertex
	{
		glm::vec3 position;
		glm::vec4 color;
		glm::vec2 texCoords;
		uint32_t texIndex;
		float tilingFactor;
	};

	struct Renderer2DData
	{
		static const uint32_t maxBatches = 100;
		static const uint32_t maxQuads = 1000;
		static const uint32_t maxVertices = maxQuads * 4;
		static const uint32_t maxIndices = maxQuads * 6;
		static const uint32_t maxTextureSlots = 32; // TODO: RenderCapabilities

		std::vector<Ref<RenderGraph>> rendergraphs;
		uint32_t exeRendergraphId = 0;
		Ref<Texture2D> colorOutput;

		Ref<RenderPass> renderPass;
		Ref<GraphicsPipeline> graphicsPipeline;
		Ref<Shader> shader;
		Ref<VertexBuffer> quadVertexBuffer;
		Ref<IndexBuffer> quadIndexBuffer;
		Ref<VertexBuffer> instanceBuffer;
		Ref<Texture2D> whiteTexture;

		uint32_t quadIndexCount = 0;
		uint32_t quadVertexOffset = 0;
		uint32_t quadIndexOffset = 0;

		QuadVertex* quadVertexBufferBase = nullptr;
		QuadVertex* quadVertexBufferPtr = nullptr;

		std::array<Ref<Texture2D>, maxTextureSlots> textureSlots;
		uint32_t textureSlotIndex = 1; // 0 = white texture

		std::array<BufferRegion, maxBatches> bufferRegions;
		std::array<DrawArgs, maxBatches> drawArgs;
		uint32_t batchIndex = 0;

		glm::vec4 quadVertexPositions[4];

		struct QuadInstance
		{
			glm::mat4 transform;
			glm::vec4 color;
			uint32_t texIndex = 0;
			float tilingFactor = 1.0f;
		};

		std::array<QuadInstance, 1000> quadInstances;
		uint32_t instanceCount = 0;

		Renderer2D::Statistics stats;
	};
	static Renderer2DData* s_data;

	void Renderer2D::init()                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
	{
		s_data = new Renderer2DData();
		s_data->whiteTexture = Texture2D::create(1, 1);
		uint32_t whiteTextureData = 0xffffffff;
		s_data->whiteTexture->setData(&whiteTextureData);

		GraphicsPipeConfiguration pipeConfig{};

#ifdef RENDERER2D_INSTANCED
		VertexDescription quadVertDescription({
			VertexAttributeType::Vec3f,
			VertexAttributeType::Vec2f
		});

		VertexDescription quadInstanceDescription({
			VertexAttributeType::Mat4x4,
			VertexAttributeType::Vec4f,
			VertexAttributeType::Uint,
			VertexAttributeType::Float
		});

		pipeInfo.instanceDescription = &quadInstanceDescription;
#else

		std::string assetsPath = "C:/dev/Shadow/Shadow/assets/";
		s_data->shader = Shader::create("texture", assetsPath + "shaders/texture.vert.spv", assetsPath + "shaders/texture.frag.spv");

		VertexInput quadVertInput({ 
			VertexAttribType::Vec3f, 
			VertexAttribType::Vec4f,
			VertexAttribType::Vec2f,
			VertexAttribType::Uint,
			VertexAttribType::Float
		});

		pipeConfig.instanceInput = nullptr;
#endif
		pipeConfig.vertexInput = &quadVertInput;
		pipeConfig.states.blendState.blendEnable = true;
		s_data->graphicsPipeline = GraphicsPipeline::create(pipeConfig);
		s_data->graphicsPipeline->setName("renderer2D_pipeline");

		FramebufferInfo framebufferInfo{};
		ShApp::get().getWindow().getFramebufferSize(framebufferInfo.width, framebufferInfo.height);
		framebufferInfo.samples = 1;
		framebufferInfo.layers = 1;

#ifndef RENDERER2D_USE_RENDERGRAPH
		SubpassAttachment colorAttachment{};
		colorAttachment.name = "color_out";
		colorAttachment.imageUsage = ImageUsage::ColorAttachment | ImageUsage::RenderPassInput;
		colorAttachment.format = ImageFormat::RGBA8;
		colorAttachment.loadOp = AttachmentLoadOp::Clear;
		colorAttachment.finalLayout = ImageLayout::ShaderReadOnlyOptimal;

		SubpassAttachment depthAttachment{};
		depthAttachment.name = "depth_out";
		depthAttachment.format = ImageFormat::Depth32f;
		depthAttachment.imageUsage = ImageUsage::DepthAttachment;
		depthAttachment.finalLayout = ImageLayout::DepthStencilAttachmentOptimal;

		Subpass subpass{};
		subpass.pipeline = s_rendererData->graphicsPipeline;
		subpass.shader = s_rendererData->shader;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachment;
		subpass.pDepthAttachment = &depthAttachment;
		subpass.inputAttachmentCount = 0;

		RenderPassConfig renderpassConfig{};
		renderpassConfig.subpassCount = 1;
		renderpassConfig.pSubpasses = &subpass;
		renderpassConfig.framebufferInfo = framebufferInfo;
		renderpassConfig.firstRenderpass = true;
		s_rendererData->renderPass = RenderPass::create(renderpassConfig);
#else
		Ref<RenderGraph> rendergraph = createRef<RenderGraph>();
		s_data->rendergraphs.emplace_back(rendergraph);

		AttachmentInfo color{};
		color.format = ImageFormat::RGBA8_Unorm;
		color.loadOp = AttachmentLoadOp::Clear;

		AttachmentInfo depth{};
		depth.format = ImageFormat::Depth32f;
		depth.loadOp = AttachmentLoadOp::Clear;

		auto& drawPass = rendergraph->addDrawPass("renderer2D_draw", s_data->graphicsPipeline, s_data->shader);
		ImageResource& colorRes = drawPass->addColorOutput("color_out", color);
		colorRes.addImageUsage(ImageUsage::StorageImage); // temp
		drawPass->setDepthStencilOutput("depth_out", depth);
		rendergraph->setup(framebufferInfo);

		s_data->colorOutput = colorRes.getTexture();

		RenderGraphWriter writer;
		writer.begin("renderer2D-rendergraph.json");
		writer.writeData(*rendergraph);
		writer.end();
#endif

#ifdef RENDERER2D_INSTANCED
		float vertices[5 * 4]{
			-0.5f,-0.5f,0.0f,  0.0f,0.0f,
			 0.5f,-0.5f,0.0f,  1.0f,0.0f,
			 0.5f, 0.5f,0.0f,  1.0f,1.0f,
			-0.5f, 0.5f,0.0f,  0.0f,1.0f
		};
		s_data->quadVertexBuffer = VertexBuffer::create(vertices, sizeof(vertices), sizeof(float) * 5, BufferUsage::Static);

		uint32_t indices[6] = { 0,1,2,2,3,0 };
		s_data->quadIndexBuffer = IndexBuffer::create(indices, sizeof(indices) / sizeof(uint32_t), BufferUsage::Static);

		s_data->instanceBuffer = VertexBuffer::create(sizeof(s_data->quadInstances[0]) * s_data->quadInstances.size());
		s_data->shader = Shader::create("texture", assetsPath + "shaders/instanced2d.vert.spv", assetsPath + "shaders/texture.frag.spv");
#else
		s_data->quadVertexBuffer = VertexBuffer::create(Renderer2DData::maxVertices * sizeof(QuadVertex) * Renderer2DData::maxBatches, quadVertInput.getStride());
		s_data->quadVertexBufferBase = new QuadVertex[Renderer2DData::maxVertices * Renderer2DData::maxBatches];

		uint32_t* quadIndices = new uint32_t[Renderer2DData::maxIndices];

		uint32_t offset = 0;
		for (uint32_t i = 0; i < Renderer2DData::maxIndices; i += 6)
		{
			quadIndices[i + 0] = offset + 0;
			quadIndices[i + 1] = offset + 1;
			quadIndices[i + 2] = offset + 2;

			quadIndices[i + 3] = offset + 2;
			quadIndices[i + 4] = offset + 3;
			quadIndices[i + 5] = offset + 0;

			offset += 4;
		}

		s_data->quadIndexBuffer = IndexBuffer::create(quadIndices, Renderer2DData::maxIndices);
		delete[] quadIndices;
#endif

		s_data->textureSlots[0] = s_data->whiteTexture;
		s_data->textureSlotIndex = 1;

		s_data->quadIndexCount = 0;
		s_data->quadVertexBufferPtr = s_data->quadVertexBufferBase;
		
		s_data->quadVertexPositions[0] = { -0.5f,-0.5f,0.0f,1.0f };
		s_data->quadVertexPositions[1] = {  0.5f,-0.5f,0.0f,1.0f };
		s_data->quadVertexPositions[2] = {  0.5f, 0.5f,0.0f,1.0f };
		s_data->quadVertexPositions[3] = { -0.5f, 0.5f,0.0f,1.0f };
	}

	void Renderer2D::shutdown()
	{
#ifndef RENDERER2D_INSTANCED
		delete[] s_data->quadVertexBufferBase;
#endif 
		delete s_data;

		SH_TRACE("Renderer2D::shutdown");
	}

	void Renderer2D::beginScene()
	{
		SH_PROFILE_RENDERER_FUNCTION();

		Renderer::synchronizeRendering();

#ifdef RENDERER2D_INSTANCED
		s_data->instanceCount = 0;
#endif
		s_data->quadIndexOffset = 0;
		s_data->quadVertexOffset = 0;
		s_data->batchIndex = 0;
		s_data->textureSlotIndex = 1;
		s_data->quadVertexBufferPtr = s_data->quadVertexBufferBase;
	}

	void Renderer2D::endScene(const Camera& camera, const glm::mat4& transform)
	{
		SH_PROFILE_RENDERER_FUNCTION();

		s_data->shader->setInput("u_samplers", s_data->textureSlotIndex, s_data->textureSlots.data(), 0);
		flush();

		auto& transferCmdBuffer = Renderer::getTransferCmdBuffer();
		transferCmdBuffer->begin();
		s_data->quadVertexBuffer->setData(transferCmdBuffer, s_data->quadVertexBufferBase, s_data->batchIndex, s_data->bufferRegions.data());
		transferCmdBuffer->end();
		transferCmdBuffer->submit();

		auto& cmdBuffer = VulkanContext::getDevice()->getCmdBuffer();
		glm::mat4 viewProj = camera.getProjection() * glm::inverse(transform);

#ifdef RENDERER2D_USE_RENDERGRAPH
		s_data->rendergraphs[s_data->exeRendergraphId]->getPass(0)->setCallback([viewProj, cmdBuffer]()
		{
			const Window& window = ShApp::get().getWindow();

			cmdBuffer->setViewport(0, 0, static_cast<float>(window.getWidth()), static_cast<float>(window.getHeight()));
			cmdBuffer->setPushConstants(&viewProj, sizeof(glm::mat4), ShaderStage::Vertex);

			for (uint32_t i = 0; i < s_data->batchIndex; i++)
				cmdBuffer->drawIndexed(s_data->quadVertexBuffer, s_data->quadIndexBuffer, s_data->drawArgs[i]);

		});

		s_data->rendergraphs[s_data->exeRendergraphId]->execute(cmdBuffer);
#else
		const Window& window = ShApp::get().getWindow();

		cmdBuffer->beginRenderPass(s_rendererData->renderPass);
		cmdBuffer->setViewport(0, 0, static_cast<float>(window.getWidth()), static_cast<float>(window.getHeight()));
		cmdBuffer->setPushConstants(&viewProj, sizeof(glm::mat4), ShaderStage::Vertex);

		for (uint32_t i = 0; i < s_rendererData->batchIndex; i++)
			cmdBuffer->drawIndexed(s_rendererData->quadVertexBuffer, s_rendererData->quadIndexBuffer, s_rendererData->drawArgs[i]);

		cmdBuffer->endRenderPass();
#endif
	}

	void Renderer2D::endScene(const OrthoCamera& camera)
	{
		SH_PROFILE_RENDERER_FUNCTION();

		s_data->shader->setInput("u_samplers", s_data->textureSlotIndex, s_data->textureSlots.data(), 0);
		flush();

		auto& transferCmdBuffer = Renderer::getTransferCmdBuffer();
		transferCmdBuffer->begin();
		s_data->quadVertexBuffer->setData(transferCmdBuffer, s_data->quadVertexBufferBase, s_data->batchIndex, s_data->bufferRegions.data());
		transferCmdBuffer->end();
		transferCmdBuffer->submit();

		auto& cmdBuffer = VulkanContext::getDevice()->getCmdBuffer();

#ifdef RENDERER2D_USE_RENDERGRAPH
		s_data->rendergraphs[s_data->exeRendergraphId]->getPass(0)->setCallback([camera, cmdBuffer]()
			{
				const Window& window = ShApp::get().getWindow();

				cmdBuffer->setViewport(0, 0, static_cast<float>(window.getWidth()), static_cast<float>(window.getHeight()));
				cmdBuffer->setPushConstants(&camera.getVPMatrix(), sizeof(glm::mat4), ShaderStage::Vertex);

				for (uint32_t i = 0; i < s_data->batchIndex; i++)
					cmdBuffer->drawIndexed(s_data->quadVertexBuffer, s_data->quadIndexBuffer, s_data->drawArgs[i]);

			});

		s_data->rendergraphs[s_data->exeRendergraphId]->execute(cmdBuffer);
#else
		const Window& window = ShApp::get().getWindow();

		cmdBuffer->beginRenderPass(s_rendererData->renderPass);

		cmdBuffer->setViewport(0, 0, static_cast<float>(window.getWidth()), static_cast<float>(window.getHeight()));
		cmdBuffer->setPushConstants(&camera.getVPMatrix(), sizeof(glm::mat4), ShaderStage::Vertex);

		for (uint32_t i = 0; i < s_rendererData->batchIndex; i++)
			cmdBuffer->drawIndexed(s_rendererData->quadVertexBuffer, s_rendererData->quadIndexBuffer, s_rendererData->drawArgs[i]);

		cmdBuffer->endRenderPass();
#endif
	}

	void Renderer2D::flush()
	{
		SH_PROFILE_RENDERER_FUNCTION();

#ifdef RENDERER2D_INSTANCED
		s_data->instanceBuffer->setData(s_data->quadInstances.data(), 0);
		Renderer::drawInstanced(s_data->quadVertexBuffer, s_data->quadIndexBuffer, s_data->instanceBuffer, s_data->instanceCount);
#else
		uint32_t dataSize = static_cast<uint32_t>((uint8_t*)s_data->quadVertexBufferPtr - ((uint8_t*)s_data->quadVertexBufferBase+s_data->quadVertexOffset));

		BufferRegion& region = s_data->bufferRegions[s_data->stats.drawCall];
		region.offset = s_data->quadVertexOffset;
		region.size = dataSize;

		DrawArgs args{};
		args.indexCount = s_data->quadIndexCount;
		args.firstVertex = s_data->quadVertexOffset / sizeof(QuadVertex);
		s_data->drawArgs[s_data->stats.drawCall] = args;

		s_data->quadIndexOffset += s_data->quadIndexCount;
		s_data->quadIndexCount = 0;
		s_data->batchIndex++;
		s_data->quadVertexOffset += dataSize;
#endif
		s_data->stats.drawCall++;
	}

	void Renderer2D::resizeFramebuffers(uint32_t width, uint32_t height)
	{
		for (auto& rendergraph : s_data->rendergraphs)
			rendergraph->resizeFramebuffers(width, height);
	}

	static void setVerticesData(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, const glm::vec2* texCoords, uint32_t texIndex, float tilingFactor)
	{
		s_data->quadVertexBufferPtr->position = position;
		s_data->quadVertexBufferPtr->color = color;
		s_data->quadVertexBufferPtr->texCoords = { texCoords[0].x, texCoords[0].y };
		s_data->quadVertexBufferPtr->texIndex = texIndex;
		s_data->quadVertexBufferPtr->tilingFactor = tilingFactor;
		s_data->quadVertexBufferPtr++;

		s_data->quadVertexBufferPtr->position = { position.x + size.x, position.y, position.z };
		s_data->quadVertexBufferPtr->color = color;
		s_data->quadVertexBufferPtr->texCoords = { texCoords[1].x, texCoords[1].y };
		s_data->quadVertexBufferPtr->texIndex = texIndex;
		s_data->quadVertexBufferPtr->tilingFactor = tilingFactor;
		s_data->quadVertexBufferPtr++;

		s_data->quadVertexBufferPtr->position = { position.x + size.x, position.y + size.y, position.z };
		s_data->quadVertexBufferPtr->color = color;
		s_data->quadVertexBufferPtr->texCoords = { texCoords[2].x, texCoords[2].y };
		s_data->quadVertexBufferPtr->texIndex = texIndex;
		s_data->quadVertexBufferPtr->tilingFactor = tilingFactor;
		s_data->quadVertexBufferPtr++;

		s_data->quadVertexBufferPtr->position = { position.x, position.y + size.y, position.z };
		s_data->quadVertexBufferPtr->color = color;
		s_data->quadVertexBufferPtr->texCoords = { texCoords[3].x, texCoords[3].y };
		s_data->quadVertexBufferPtr->texIndex = texIndex;
		s_data->quadVertexBufferPtr->tilingFactor = tilingFactor;
		s_data->quadVertexBufferPtr++;

		s_data->quadIndexCount += 6;
	}

	static void setVerticesData(const glm::mat4& transform, const glm::vec4& color, const glm::vec2* texCoords, uint32_t texIndex, float tilingFactor)
	{
		s_data->quadVertexBufferPtr->position = transform * s_data->quadVertexPositions[0];
		s_data->quadVertexBufferPtr->color = color;
		s_data->quadVertexBufferPtr->texCoords = { texCoords[0].x, texCoords[0].y };
		s_data->quadVertexBufferPtr->texIndex = texIndex;
		s_data->quadVertexBufferPtr->tilingFactor = tilingFactor;
		s_data->quadVertexBufferPtr++;

		s_data->quadVertexBufferPtr->position = transform * s_data->quadVertexPositions[1];
		s_data->quadVertexBufferPtr->color = color;
		s_data->quadVertexBufferPtr->texCoords = { texCoords[1].x, texCoords[1].y };
		s_data->quadVertexBufferPtr->texIndex = texIndex;
		s_data->quadVertexBufferPtr->tilingFactor = tilingFactor;
		s_data->quadVertexBufferPtr++;

		s_data->quadVertexBufferPtr->position = transform * s_data->quadVertexPositions[2];
		s_data->quadVertexBufferPtr->color = color;
		s_data->quadVertexBufferPtr->texCoords = { texCoords[2].x, texCoords[2].y };
		s_data->quadVertexBufferPtr->texIndex = texIndex;
		s_data->quadVertexBufferPtr->tilingFactor = tilingFactor;
		s_data->quadVertexBufferPtr++;

		s_data->quadVertexBufferPtr->position = transform * s_data->quadVertexPositions[3];
		s_data->quadVertexBufferPtr->color = color;
		s_data->quadVertexBufferPtr->texCoords = { texCoords[3].x, texCoords[3].y };
		s_data->quadVertexBufferPtr->texIndex = texIndex;
		s_data->quadVertexBufferPtr->tilingFactor = tilingFactor;
		s_data->quadVertexBufferPtr++;

		s_data->quadIndexCount += 6;
	}

	static uint32_t retrieveTexIndex(const Ref<Texture2D>& texture)
	{
		uint32_t textureIndex = 0;

		for (uint32_t i = 1; i < s_data->textureSlotIndex; i++)
		{
			if (*s_data->textureSlots[i].get() == *texture.get())
			{
				textureIndex = i;
				break;
			}
		}

		if (textureIndex == 0)
		{
			textureIndex = s_data->textureSlotIndex;
			s_data->textureSlots[textureIndex] = texture;
			s_data->textureSlotIndex++;
		}

		return textureIndex;
	}

	void Renderer2D::drawQuad(const QuadProperties& properties)
	{
		uint32_t texIndex = properties.texture ? retrieveTexIndex(properties.texture) : 0;

#ifndef RENDERER2D_INSTANCED
		if (s_data->quadIndexCount >= Renderer2DData::maxIndices)
			flush();

		constexpr glm::vec2 texCoords[4] = {
			{0.0f,0.0f},
			{1.0f,0.0f},
			{1.0f,1.0f},
			{0.0f,1.0f}
		};

		setVerticesData(properties.position, properties.size, properties.color,
			properties.sprite ? properties.sprite->getTexCoords() : texCoords, 
			texIndex, properties.tilingFactor);
#else
		s_data->quadInstances[s_data->instanceCount].transform = 
			glm::translate(glm::mat4(1.0f), properties.position) * glm::scale(glm::mat4(1.0f), { properties.size, 1.0 });

		s_data->quadInstances[s_data->instanceCount].color = properties.color;
		s_data->quadInstances[s_data->instanceCount].texIndex = texIndex;
		s_data->quadInstances[s_data->instanceCount++].tilingFactor = properties.tilingFactor;
#endif

#ifdef RENDERER_STATISTICS
		s_data->stats.quadCount++;
#endif
	}

	void Renderer2D::drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
	{
		drawQuad({ position.x, position.y, 0.0f }, size, color);
	}

	void Renderer2D::drawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
	{
		const uint32_t texIndex = 0;
		const float tilingFactor = 1.0f;

#ifndef RENDERER2D_INSTANCED
		if (s_data->quadIndexCount >= Renderer2DData::maxIndices)
			flush();

		constexpr glm::vec2 texCoords[4] = {
			{0.0f,0.0f},
			{1.0f,0.0f},
			{1.0f,1.0f},
			{0.0f,1.0f}
		};

		setVerticesData(position, size, color, texCoords, 0, 1.0f);
#else
		s_data->quadInstances[s_data->instanceCount].transform = 
			glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), { size, 1.0 });
		s_data->quadInstances[s_data->instanceCount++].color = color;
#endif

#ifdef RENDERER_STATISTICS
		s_data->stats.quadCount++;
#endif
	}

	void Renderer2D::drawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, const Ref<Sprite2D>& subTex, float tilingFactor)
	{
		drawQuad({ position.x, position.y, 0.0f }, size, texture, subTex, tilingFactor);
	}

	void Renderer2D::drawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, const Ref<Sprite2D>& subTex, float tilingFactor)
	{
		constexpr glm::vec4 color = { 1.0f,1.0f,1.0f,1.0f };
		uint32_t texIndex = (texture ? retrieveTexIndex(texture) : 0);

#ifndef RENDERER2D_INSTANCED
		if (s_data->quadIndexCount >= Renderer2DData::maxIndices)
			flush();

		constexpr glm::vec2 texCoords[4] = {
			{0.0f,0.0f},
			{1.0f,0.0f},
			{1.0f,1.0f},
			{0.0f,1.0f}
		};

		setVerticesData(position, size, color,
			subTex? subTex->getTexCoords() : texCoords, texIndex, tilingFactor);
#else
		s_data->quadInstances[s_data->instanceCount].transform = 
			glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), { size, 1.0 });

		s_data->quadInstances[s_data->instanceCount].color = { 1.0f,1.0f,1.0f,1.0f };
		s_data->quadInstances[s_data->instanceCount].texIndex = texIndex;
		s_data->quadInstances[s_data->instanceCount++].tilingFactor = tilingFactor;
#endif

#ifdef RENDERER_STATISTICS
		s_data->stats.quadCount++;
#endif
	}

	void Renderer2D::drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color, const Ref<Texture2D>& texture, const Ref<Sprite2D>& subTex, float tilingFactor)
	{
		drawQuad(glm::vec3{ position, 0.0f }, size, color, texture);
	}

	void Renderer2D::drawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, const Ref<Texture2D>& texture, const Ref<Sprite2D>& subTex, float tilingFactor)
	{
		uint32_t texIndex = (texture ? retrieveTexIndex(texture) : 0);

#ifndef RENDERER2D_INSTANCED
		if (s_data->quadIndexCount >= Renderer2DData::maxIndices)
			flush();

		constexpr glm::vec2 texCoords[4] = {
			{0.0f,0.0f},
			{1.0f,0.0f},
			{1.0f,1.0f},
			{0.0f,1.0f}
		};

		setVerticesData(position, size, color, 
			subTex? subTex->getTexCoords() : texCoords, texIndex, tilingFactor);
#else
		s_data->quadInstances[s_data->instanceCount].transform =
			glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), { size, 1.0 });

		s_data->quadInstances[s_data->instanceCount].color = color;
		s_data->quadInstances[s_data->instanceCount].texIndex = texIndex;
		s_data->quadInstances[s_data->instanceCount++].tilingFactor = tilingFactor;
#endif

#ifdef RENDERER_STATISTICS
		s_data->stats.quadCount++;
#endif
	}

	void Renderer2D::drawQuad(const glm::mat4& transform, const glm::vec4& color)
	{
		if (s_data->quadIndexCount >= Renderer2DData::maxIndices)
			flush();

		constexpr glm::vec2 texCoords[4] = {
			{0.0f,0.0f},
			{1.0f,0.0f},
			{1.0f,1.0f},
			{0.0f,1.0f}
		};

		uint32_t whiteTexIndex = 0;
		setVerticesData(transform, color, texCoords, whiteTexIndex, 1.0f);

#ifdef RENDERER_STATISTICS
		s_data->stats.quadCount++;
#endif
	}

	void Renderer2D::drawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, const Ref<Sprite2D>& subTex, float tilingFactor)
	{
		uint32_t texIndex = (texture ? retrieveTexIndex(texture) : 0);

		if (s_data->quadIndexCount >= Renderer2DData::maxIndices)
			flush();

		constexpr glm::vec2 texCoords[4] = {
			{0.0f,0.0f},
			{1.0f,0.0f},
			{1.0f,1.0f},
			{0.0f,1.0f}
		};

		setVerticesData(transform, glm::vec4{ 1.0f,1.0f,1.0f,1.0f },
			subTex ? subTex->getTexCoords() : texCoords,
			texIndex, tilingFactor);

#ifdef RENDERER_STATISTICS
		s_data->stats.quadCount++;
#endif
	}

	void Renderer2D::drawQuad(const glm::mat4& transform, const glm::vec4& color, const Ref<Texture2D>& texture, const Ref<Sprite2D>& subTex, float tilingFactor)
	{
		uint32_t texIndex = (texture ? retrieveTexIndex(texture) : 0);

		if (s_data->quadIndexCount >= Renderer2DData::maxIndices)
			flush();

		constexpr glm::vec2 texCoords[4] = {
			{0.0f,0.0f},
			{1.0f,0.0f},
			{1.0f,1.0f},
			{0.0f,1.0f}
		};

		setVerticesData(transform, color,
			subTex ? subTex->getTexCoords() : texCoords,
			texIndex, tilingFactor);

#ifdef RENDERER_STATISTICS
		s_data->stats.quadCount++;
#endif
	}

	void Renderer2D::drawRotatedQuad(const QuadProperties& properties, float angle)
	{
		uint32_t texIndex = properties.texture ? retrieveTexIndex(properties.texture) : 0;

#ifdef  RENDERER2D_INSTANCED
		s_data->quadInstances[s_data->instanceCount].transform = glm::translate(glm::mat4(1.0f), properties.position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(angle), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { properties.size, 1.0 });

		s_data->quadInstances[s_data->instanceCount].color = properties.color;
		s_data->quadInstances[s_data->instanceCount].texIndex = texIndex;
		s_data->quadInstances[s_data->instanceCount++].tilingFactor = properties.tilingFactor;
#else
		if (s_data->quadIndexCount >= Renderer2DData::maxIndices)
			flush();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), properties.position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(angle), { 0.0f,0.0f,1.0f })
			* glm::scale(glm::mat4(1.0f), { properties.size.x, properties.size.y, 1.0f });

		constexpr glm::vec2 texCoords[4] = {
			{0.0f,0.0f},
			{1.0f,0.0f},
			{1.0f,1.0f},
			{0.0f,1.0f}
		};

		setVerticesData(transform, properties.color,
			properties.sprite? properties.sprite->getTexCoords() : texCoords,
			texIndex, properties.tilingFactor);
#endif

#ifdef RENDERER_STATISTICS
		s_data->stats.quadCount++;
#endif
	}

	void Renderer2D::drawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float angle, const glm::vec4& color)
	{
		drawRotatedQuad({ position.x, position.y, 0.0f }, size, angle, color);
	}

	void Renderer2D::drawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float angle, const glm::vec4& color)
	{
#ifdef RENDERER2D_INSTANCED
		s_data->quadInstances[s_data->instanceCount].transform =
			glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), angle, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size, 1.0 });

		s_data->quadInstances[s_data->instanceCount++].color = color;
#else
		if (s_data->quadIndexCount >= Renderer2DData::maxIndices)
			flush();

		const uint32_t texIndex = 0;
		const float tilingFactor = 1.0f;

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), angle, { 0.0f,0.0f,1.0f })
			* glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f });

		constexpr glm::vec2 texCoords[4] = {
			{0.0f,0.0f},
			{1.0f,0.0f},
			{1.0f,1.0f},
			{0.0f,1.0f}
		};

		setVerticesData(transform, color, texCoords, texIndex, tilingFactor);
#endif

#ifdef RENDERER_STATISTICS
		s_data->stats.quadCount++;
#endif
	}

	void Renderer2D::drawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float angle, const Ref<Texture2D>& texture, const Ref<Sprite2D>& subTex, float tilingFactor)
	{
		drawRotatedQuad({ position.x, position.y, 0.0f }, size, angle, texture, subTex, tilingFactor);
	}

	void Renderer2D::drawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float angle, const Ref<Texture2D>& texture, const Ref<Sprite2D>& subTex, float tilingFactor)
	{
		const glm::vec4 color = { 1.0f,1.0f,1.0f,1.0f };
		uint32_t texIndex = (texture ? retrieveTexIndex(texture) : 0);

#ifdef RENDERER2D_INSTANCED
		s_data->quadInstances[s_data->instanceCount].transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), angle, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size, 1.0 });

		s_data->quadInstances[s_data->instanceCount].color = color;
		s_data->quadInstances[s_data->instanceCount].texIndex = texIndex;
		s_data->quadInstances[s_data->instanceCount++].tilingFactor = tilingFactor;
#else
		if (s_data->quadIndexCount >= Renderer2DData::maxIndices)
			flush();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), angle, { 0.0f,0.0f,1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		constexpr glm::vec2 texCoords[4] = {
			{0.0f,0.0f},
			{1.0f,0.0f},
			{1.0f,1.0f},
			{0.0f,1.0f}
		};

		setVerticesData(transform, color,
			subTex ? subTex->getTexCoords() : texCoords,
			texIndex, tilingFactor);
#endif

#ifdef RENDERER_STATISTICS
		s_data->stats.quadCount++;
#endif
	}

	void Renderer2D::drawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float angle, const glm::vec4& color, const Ref<Texture2D>& texture, const Ref<Sprite2D>& subTex, float tilingFactor)
	{
		drawRotatedQuad({ position.x, position.y, 0.0f }, size, angle, color, texture, subTex, tilingFactor);
	}

	void Renderer2D::drawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float angle, const glm::vec4& color, const Ref<Texture2D>& texture, const Ref<Sprite2D>& subTex, float tilingFactor)
	{
		uint32_t texIndex = (texture ? retrieveTexIndex(texture) : 0);

#ifdef RENDERER2D_INSTANCED
		s_data->quadInstances[s_data->instanceCount].transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(angle), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), {size, 1.0 });

		s_data->quadInstances[s_data->instanceCount].color = color;
		s_data->quadInstances[s_data->instanceCount].texIndex = texIndex;
		s_data->quadInstances[s_data->instanceCount++].tilingFactor = tilingFactor;
#else
		if (s_data->quadIndexCount >= Renderer2DData::maxIndices)
			flush();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), angle, { 0.0f,0.0f,1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		constexpr glm::vec2 texCoords[4] = {
			{0.0f,0.0f},
			{1.0f,0.0f},
			{1.0f,1.0f},
			{0.0f,1.0f}
		};

		setVerticesData(transform, color, 
			subTex? subTex->getTexCoords() : texCoords, texIndex, tilingFactor);
#endif

#ifdef RENDERER_STATISTICS
		s_data->stats.quadCount++;
#endif
	}

	const Ref<GraphicsPipeline>& Renderer2D::getGraphicsPipeline()
	{
		return s_data->graphicsPipeline;
	}

	const Ref<Shader>& Renderer2D::getShader()
	{
		return s_data->shader;
	}

	const Ref<RenderPass>& Renderer2D::getRenderPass()
	{
		return s_data->renderPass;
	}

	const Ref<Texture2D>& Renderer2D::getColorOutput()
	{
#ifndef RENDERER2D_USE_RENDERGRAPH
		return s_rendererData->renderPass->getImage("color_out");
#else
		return s_data->colorOutput;
#endif
	}

	const Ref<Texture2D>& Renderer2D::getWhiteTexture()
	{
		return s_data->textureSlots[0];
	}

	uint32_t Renderer2D::addRenderGraph(const Ref<RenderGraph>& rendergraph)
	{
		s_data->rendergraphs.emplace_back(rendergraph);
		return static_cast<uint32_t>(s_data->rendergraphs.size() - 1);
	}

	void Renderer2D::setExeRenderGraph(uint32_t rendergraphId)
	{
		s_data->exeRendergraphId = rendergraphId;
	}

	void Renderer2D::setColorOutput(const Ref<Texture2D>& image)
	{
		s_data->colorOutput = image;
	}

	void Renderer2D::resetStats()
	{
		memset(&s_data->stats, 0, sizeof(Statistics));
	}

    const Renderer2D::Statistics& Renderer2D::getStats()
	{
		return s_data->stats;
	}
}