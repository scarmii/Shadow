#include "shpch.hpp"
#include "Shadow/Core/Core.hpp"
#include "Shadow/Core/ShEngine.hpp"

#include "Shadow/Renderer/Renderer2D.hpp"
#include "Shadow/Renderer/Renderer.hpp"
#include "Shadow/Renderer/Pipeline.hpp"
#include "Shadow/ImGui/VkImGuiLayer.hpp"
#include "Shadow/Vulkan/VulkanBuffer.hpp"

#include <glm/ext.hpp>

#include <unordered_map>

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
		static const uint32_t maxQuads = 1000;
		static const uint32_t maxVertices = maxQuads * 4;
		static const uint32_t maxIndices = maxQuads * 6;
	    static const uint32_t maxTextureSlots = 32; // TODO: RenderCapabilities

		Ref<GraphicsPipeline> graphicsPipeline;
		Ref<Shader> shader;
		Ref<VertexBuffer> quadVertexBuffer;
		Ref<IndexBuffer> quadIndexBuffer;
		Ref<VertexBuffer> instanceBuffer;
		Ref<Renderpass> renderpass;
		Ref<Texture2D> whiteTexture;

		uint32_t quadIndexCount = 0;
		uint32_t quadVertexOffset = 0;
		uint32_t quadIndexOffset = 0;

		QuadVertex* quadVertexBufferBase = nullptr;
		QuadVertex* quadVertexBufferPtr = nullptr;

		std::array<Ref<Texture2D>, maxTextureSlots> textureSlots;
		uint32_t textureSlotIndex = 1; // 0 = white texture

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
	static Renderer2DData* s_rendererData; 

	void Renderer2D::init()                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
	{
		s_rendererData = new Renderer2DData();

		s_rendererData->whiteTexture = Texture2D::create(1, 1);
		uint32_t whiteTextureData = 0xffffffff;
		s_rendererData->whiteTexture->setData(&whiteTextureData);

		std::string assetsPath = "C:/dev/Shadow/Shadow/assets/";

		FramebufferInfo framebufferInfo{};
		ShEngine::get().getWindow().getFramebufferSize(framebufferInfo.width, framebufferInfo.height);
		framebufferInfo.samples = 1;
		framebufferInfo.layers = 1;

		SubpassAttachment depthAttachment{};
		depthAttachment.ref = 0;
		depthAttachment.format = ImageFormat::Depth32f;
		depthAttachment.usage = AttachmentUsage::DepthAttachment;

		Subpass subpass{};
		subpass.pDepthAttachment = &depthAttachment;
		subpass.inputAttachmentCount = 0;

		RenderpassConfig renderpassConfig{};
		renderpassConfig.subpassCount = 1;
		renderpassConfig.pSubpasses = &subpass;
		renderpassConfig.framebufferInfo = framebufferInfo;
		renderpassConfig.firstRenderpass = true;
		renderpassConfig.swapchainTarget = true;
		s_rendererData->renderpass = Renderpass::create(renderpassConfig);

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
		s_rendererData->shader = Shader::create("texture", assetsPath + "shaders/texture.vert.spv", assetsPath + "shaders/texture.frag.spv");

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
		pipeConfig.shader = s_rendererData->shader;
		pipeConfig.renderpass = s_rendererData->renderpass;
		pipeConfig.subpass = 0;
		s_rendererData->graphicsPipeline = GraphicsPipeline::create(pipeConfig);

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
		s_rendererData->quadVertexBuffer = VertexBuffer::create(Renderer2DData::maxVertices * sizeof(QuadVertex), quadVertInput);
		s_rendererData->quadVertexBufferBase = new QuadVertex[Renderer2DData::maxIndices];

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

		s_rendererData->quadIndexBuffer = IndexBuffer::create(quadIndices, Renderer2DData::maxIndices);
		delete[] quadIndices;
#endif

		s_rendererData->textureSlots[0] = s_rendererData->whiteTexture;
		s_rendererData->textureSlotIndex = 1;

		s_rendererData->quadIndexCount = 0;
		s_rendererData->quadVertexBufferPtr = s_rendererData->quadVertexBufferBase;
		
		s_rendererData->quadVertexPositions[0] = { -0.5f,-0.5f,0.0f,1.0f };
		s_rendererData->quadVertexPositions[1] = {  0.5f,-0.5f,0.0f,1.0f };
		s_rendererData->quadVertexPositions[2] = {  0.5f, 0.5f,0.0f,1.0f };
		s_rendererData->quadVertexPositions[3] = { -0.5f, 0.5f,0.0f,1.0f };
	}

	void Renderer2D::shutdown()
	{
#ifndef RENDERER2D_INSTANCED
		delete[] s_rendererData->quadVertexBufferBase;
#endif 
		delete s_rendererData;
	}

	void Renderer2D::beginScene(const OrthoCamera& camera)
	{
#ifdef RENDERER2D_INSTANCED
		s_data->instanceCount = 0;
#endif
		s_rendererData->quadIndexOffset = 0;
		s_rendererData->quadVertexOffset = 0;
		s_rendererData->quadVertexBufferPtr = s_rendererData->quadVertexBufferBase;

		const Window& window = Shadow::ShEngine::get().getWindow();

		Renderer::setViewport(0, 0, static_cast<float>(window.getWidth()), static_cast<float>(window.getHeight()));
		Renderer::beginRenderPass(s_rendererData->graphicsPipeline, &camera.getVPMatrix());
	}

	void Renderer2D::endScene()
	{
		flush();
		Renderer::endRenderPass();
	}

	void Renderer2D::flush()
	{
		//SH_PROFILE_RENDERER_FUNCTION();

		s_rendererData->shader->writeDescriptorSet("u_samplers", s_rendererData->textureSlotIndex, s_rendererData->textureSlots.data(), 0);

#ifdef RENDERER2D_INSTANCED
		s_data->instanceBuffer->setData(s_data->quadInstances.data(), 0);
		Renderer::drawInstanced(s_data->quadVertexBuffer, s_data->quadIndexBuffer, s_data->instanceBuffer, s_data->instanceCount);
#else
		uint32_t dataSize = static_cast<uint32_t>((uint8_t*)s_rendererData->quadVertexBufferPtr - ((uint8_t*)s_rendererData->quadVertexBufferBase+s_rendererData->quadVertexOffset));

		Renderer::beginTransfer();
		s_rendererData->quadVertexBuffer->setData(s_rendererData->quadVertexBufferBase, dataSize);
		Renderer::submitTransfer(PipelineStages::VertexInput);

		Renderer::drawIndexed(s_rendererData->quadVertexBuffer, s_rendererData->quadIndexBuffer);

		s_rendererData->quadIndexCount = 0;
		s_rendererData->textureSlotIndex = 1;
#endif

#ifdef RENDERER_STATISTICS
		s_rendererData->stats.drawCalls++;
#endif
	}

	void Renderer2D::drawQuad(const QuadProperties& properties)
	{
		uint32_t texIndex = properties.texture ? retrieveTexIndex(properties.texture) : 0;

#ifndef RENDERER2D_INSTANCED
		if (s_rendererData->quadIndexCount >= Renderer2DData::maxIndices)
			flush();

		setVerticesData(properties.position, properties.size, properties.color, texIndex, properties.tilingFactor);
#else
		s_data->quadInstances[s_data->instanceCount].transform = 
			glm::translate(glm::mat4(1.0f), properties.position) * glm::scale(glm::mat4(1.0f), { properties.size, 1.0 });

		s_data->quadInstances[s_data->instanceCount].color = properties.color;
		s_data->quadInstances[s_data->instanceCount].texIndex = texIndex;
		s_data->quadInstances[s_data->instanceCount++].tilingFactor = properties.tilingFactor;
#endif

#ifdef RENDERER_STATISTICS
		s_rendererData->stats.quadCount++;
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
		if (s_rendererData->quadIndexCount >= Renderer2DData::maxIndices)
			flush();

		setVerticesData(position, size, color, 0, 1.0f);
#else
		s_data->quadInstances[s_data->instanceCount].transform = 
			glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), { size, 1.0 });
		s_data->quadInstances[s_data->instanceCount++].color = color;
#endif

#ifdef RENDERER_STATISTICS
		s_rendererData->stats.quadCount++;
#endif
	}

	void Renderer2D::drawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor)
	{
		drawQuad({ position.x, position.y, 0.0f }, size, texture, tilingFactor);
	}

	void Renderer2D::drawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor)
	{
		constexpr glm::vec4 color = { 1.0f,1.0f,1.0f,1.0f };
		uint32_t texIndex = retrieveTexIndex(texture);

#ifndef RENDERER2D_INSTANCED
		if (s_rendererData->quadIndexCount >= Renderer2DData::maxIndices)
			flush();

		setVerticesData(position, size, color, texIndex, tilingFactor);
#else
		s_data->quadInstances[s_data->instanceCount].transform = 
			glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), { size, 1.0 });

		s_data->quadInstances[s_data->instanceCount].color = { 1.0f,1.0f,1.0f,1.0f };
		s_data->quadInstances[s_data->instanceCount].texIndex = texIndex;
		s_data->quadInstances[s_data->instanceCount++].tilingFactor = tilingFactor;
#endif

#ifdef RENDERER_STATISTICS
		s_rendererData->stats.quadCount++;
#endif
	}

	void Renderer2D::drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color, const Ref<Texture2D>& texture, float tilingFactor)
	{
		drawQuad(glm::vec3{ position, 0.0f }, size, color, texture);
	}

	void Renderer2D::drawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, const Ref<Texture2D>& texture, float tilingFactor)
	{
		uint32_t texIndex = retrieveTexIndex(texture);

#ifndef RENDERER2D_INSTANCED
		if (s_rendererData->quadIndexCount >= Renderer2DData::maxIndices)
			flush();

		setVerticesData(position, size, color, texIndex, tilingFactor);
#else
		s_data->quadInstances[s_data->instanceCount].transform =
			glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), { size, 1.0 });

		s_data->quadInstances[s_data->instanceCount].color = color;
		s_data->quadInstances[s_data->instanceCount].texIndex = texIndex;
		s_data->quadInstances[s_data->instanceCount++].tilingFactor = tilingFactor;
#endif

#ifdef RENDERER_STATISTICS
		s_rendererData->stats.quadCount++;
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
		if (s_rendererData->quadIndexCount >= Renderer2DData::maxIndices)
			flush();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), properties.position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(angle), { 0.0f,0.0f,1.0f })
			* glm::scale(glm::mat4(1.0f), { properties.size.x, properties.size.y, 1.0f });

		setVerticesData(transform, properties.color, texIndex, properties.tilingFactor);
#endif

#ifdef RENDERER_STATISTICS
		s_rendererData->stats.quadCount++;
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
			* glm::rotate(glm::mat4(1.0f), glm::radians(angle), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size, 1.0 });

		s_data->quadInstances[s_data->instanceCount++].color = color;
#else
		if (s_rendererData->quadIndexCount >= Renderer2DData::maxIndices)
			flush();

		const uint32_t texIndex = 0;
		const float tilingFactor = 1.0f;

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(angle), { 0.0f,0.0f,1.0f })
			* glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f });

		setVerticesData(transform, color, texIndex, tilingFactor);
#endif

#ifdef RENDERER_STATISTICS
		s_rendererData->stats.quadCount++;
#endif
	}

	void Renderer2D::drawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float angle, const Ref<Texture2D>& texture, float tilingFactor)
	{
		drawRotatedQuad({ position.x, position.y, 0.0f }, size, angle, texture, tilingFactor);
	}

	void Renderer2D::drawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float angle, const Ref<Texture2D>& texture, float tilingFactor)
	{
		const glm::vec4 color = { 1.0f,1.0f,1.0f,1.0f };
		uint32_t texIndex = retrieveTexIndex(texture);

#ifdef RENDERER2D_INSTANCED
		s_data->quadInstances[s_data->instanceCount].transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(angle), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size, 1.0 });

		s_data->quadInstances[s_data->instanceCount].color = color;
		s_data->quadInstances[s_data->instanceCount].texIndex = texIndex;
		s_data->quadInstances[s_data->instanceCount++].tilingFactor = tilingFactor;
#else
		if (s_rendererData->quadIndexCount >= Renderer2DData::maxIndices)
			flush();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(angle), { 0.0f,0.0f,1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		setVerticesData(transform, color, texIndex, tilingFactor);
#endif

#ifdef RENDERER_STATISTICS
		s_rendererData->stats.quadCount++;
#endif
	}

	void Renderer2D::drawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float angle, const glm::vec4& color, const Ref<Texture2D>& texture, float tilingFactor)
	{
		drawRotatedQuad({ position.x, position.y, 0.0f }, size, angle, color, texture, tilingFactor);
	}

	void Renderer2D::drawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float angle, const glm::vec4& color, const Ref<Texture2D>& texture, float tilingFactor)
	{
		uint32_t texIndex = retrieveTexIndex(texture);

#ifdef RENDERER2D_INSTANCED
		s_data->quadInstances[s_data->instanceCount].transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(angle), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), {size, 1.0 });

		s_data->quadInstances[s_data->instanceCount].color = color;
		s_data->quadInstances[s_data->instanceCount].texIndex = texIndex;
		s_data->quadInstances[s_data->instanceCount++].tilingFactor = tilingFactor;
#else
		if (s_rendererData->quadIndexCount >= Renderer2DData::maxIndices)
			flush();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(angle), { 0.0f,0.0f,1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		setVerticesData(transform, color, texIndex, tilingFactor);
#endif

#ifdef RENDERER_STATISTICS
		s_rendererData->stats.quadCount++;
#endif
	}

	void Renderer2D::resetStats()
	{
		memset(&s_rendererData->stats, 0, sizeof(Statistics));
	}

    const Renderer2D::Statistics& Renderer2D::getStats()
	{
		return s_rendererData->stats;
	}

	void Renderer2D::setVerticesData(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, uint32_t texIndex, float tilingFactor)
	{
		s_rendererData->quadVertexBufferPtr->position = position;
		s_rendererData->quadVertexBufferPtr->color = color;
		s_rendererData->quadVertexBufferPtr->texCoords = { 0.0f, 0.0f };
		s_rendererData->quadVertexBufferPtr->texIndex = texIndex;
		s_rendererData->quadVertexBufferPtr->tilingFactor = tilingFactor;
		s_rendererData->quadVertexBufferPtr++;

		s_rendererData->quadVertexBufferPtr->position = { position.x + size.x, position.y, position.z };
		s_rendererData->quadVertexBufferPtr->color = color;
		s_rendererData->quadVertexBufferPtr->texCoords = { 1.0f, 0.0f };
		s_rendererData->quadVertexBufferPtr->texIndex = texIndex;
		s_rendererData->quadVertexBufferPtr->tilingFactor = tilingFactor;
		s_rendererData->quadVertexBufferPtr++;

		s_rendererData->quadVertexBufferPtr->position = { position.x + size.x, position.y + size.y, position.z };
		s_rendererData->quadVertexBufferPtr->color = color;
		s_rendererData->quadVertexBufferPtr->texCoords = { 1.0f, 1.0f };
		s_rendererData->quadVertexBufferPtr->texIndex = texIndex;
		s_rendererData->quadVertexBufferPtr->tilingFactor = tilingFactor;
		s_rendererData->quadVertexBufferPtr++;

		s_rendererData->quadVertexBufferPtr->position = { position.x, position.y + size.y, position.z };
		s_rendererData->quadVertexBufferPtr->color = color;
		s_rendererData->quadVertexBufferPtr->texCoords = { 0.0f, 1.0f };
		s_rendererData->quadVertexBufferPtr->texIndex = texIndex;
		s_rendererData->quadVertexBufferPtr->tilingFactor = tilingFactor;
		s_rendererData->quadVertexBufferPtr++;

		s_rendererData->quadIndexCount += 6;
	}

	void Renderer2D::setVerticesData(const glm::mat4& transform, const glm::vec4& color, uint32_t texIndex, float tilingFactor)
	{
		s_rendererData->quadVertexBufferPtr->position = transform * s_rendererData->quadVertexPositions[0];
		s_rendererData->quadVertexBufferPtr->color = color;
		s_rendererData->quadVertexBufferPtr->texCoords = { 0.0f, 0.0f };
		s_rendererData->quadVertexBufferPtr->texIndex = texIndex;
		s_rendererData->quadVertexBufferPtr->tilingFactor = tilingFactor;
		s_rendererData->quadVertexBufferPtr++;

		s_rendererData->quadVertexBufferPtr->position = transform * s_rendererData->quadVertexPositions[1];
		s_rendererData->quadVertexBufferPtr->color = color;
		s_rendererData->quadVertexBufferPtr->texCoords = { 1.0f, 0.0f };
		s_rendererData->quadVertexBufferPtr->texIndex = texIndex;
		s_rendererData->quadVertexBufferPtr->tilingFactor = tilingFactor;
		s_rendererData->quadVertexBufferPtr++;

		s_rendererData->quadVertexBufferPtr->position = transform * s_rendererData->quadVertexPositions[2];
		s_rendererData->quadVertexBufferPtr->color = color;
		s_rendererData->quadVertexBufferPtr->texCoords = { 1.0f, 1.0f };
		s_rendererData->quadVertexBufferPtr->texIndex = texIndex;
		s_rendererData->quadVertexBufferPtr->tilingFactor = tilingFactor;
		s_rendererData->quadVertexBufferPtr++;

		s_rendererData->quadVertexBufferPtr->position = transform * s_rendererData->quadVertexPositions[3];
		s_rendererData->quadVertexBufferPtr->color = color;
		s_rendererData->quadVertexBufferPtr->texCoords = { 0.0f, 1.0f };
		s_rendererData->quadVertexBufferPtr->texIndex = texIndex;
		s_rendererData->quadVertexBufferPtr->tilingFactor = tilingFactor;
		s_rendererData->quadVertexBufferPtr++;

		s_rendererData->quadIndexCount += 6;
	}

	uint32_t Renderer2D::retrieveTexIndex(const Ref<Texture2D>& texture)
	{
		uint32_t textureIndex = 0;

		for (uint32_t i = 1; i < s_rendererData->textureSlotIndex; i++)
		{
			if (*s_rendererData->textureSlots[i].get() == *texture.get())
			{
				textureIndex = i;
				break;
			}
		}

		if (textureIndex == 0)
		{
			textureIndex = s_rendererData->textureSlotIndex;
			s_rendererData->textureSlots[textureIndex] = texture;
			s_rendererData->textureSlotIndex++;
		}

		return textureIndex;
	}
}