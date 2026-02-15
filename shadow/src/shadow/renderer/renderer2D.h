#pragma once

#include "shadow/renderer/camera.h"
#include "shadow/renderer/rendergraph.h"

#include "shadow/vulkan/texture.h"
#include "shadow/vulkan/pipeline.h"
#include "shadow/vulkan/commandBuffer.h"

namespace Shadow
{
	struct QuadProperties
	{
		glm::vec3 position{0.0f};
		glm::vec2 size{1.0f};
		glm::vec4 color{ 1.0f };
		Ref<Sprite2D> sprite = nullptr;
		Ref<Texture2D> texture = nullptr;
		float tilingFactor = 1.0f;
	};

	class Renderer2D
	{
	public:
		static void init();
		static void shutdown();

		static void beginScene();
		static void endScene(const Camera& camera, const glm::mat4& transform);
		static void endScene(const OrthoCamera& camera); // TODO: remove
		static void flush();
		static void resizeFramebuffers(uint32_t width, uint32_t height);

		// primitives
		static void drawQuad(const QuadProperties& properties);
		static void drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
		static void drawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);
		static void drawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, const Ref<Sprite2D>& subTex = nullptr, float tilingFactor = 1.0f);
		static void drawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, const Ref<Sprite2D>& subTex = nullptr, float tilingFactor = 1.0f);
		static void drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color, const Ref<Texture2D>& texture, const Ref<Sprite2D>& subTex = nullptr, float tilingFactor = 1.0f);
		static void drawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, const Ref<Texture2D>& texture, const Ref<Sprite2D>& subTex = nullptr, float tilingFactor = 1.0f);

		static void drawQuad(const glm::mat4& transform, const glm::vec4& color);
		static void drawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, const Ref<Sprite2D>& subTex = nullptr, float tilingFactor = 1.0f);
		static void drawQuad(const glm::mat4& transform, const glm::vec4& color, const Ref<Texture2D>& texture, const Ref<Sprite2D>& subTex = nullptr, float tilingFactor = 1.0f);

		// rotation is supposed to be in radians
		static void drawRotatedQuad(const QuadProperties& properties, float angle);
		static void drawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float angle, const glm::vec4& color);
		static void drawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float angle, const glm::vec4& color);
		static void drawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float angle, const Ref<Texture2D>& texture, const Ref<Sprite2D>& subTex = nullptr, float tilingFactor = 1.0f);
		static void drawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float angle, const Ref<Texture2D>& texture, const Ref<Sprite2D>& subTex = nullptr, float tilingFactor = 1.0f);
		static void drawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float angle, const glm::vec4& color, const Ref<Texture2D>& texture, const Ref<Sprite2D>& subTex = nullptr, float tilingFactor = 1.0f);
		static void drawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float angle, const glm::vec4& color, const Ref<Texture2D>& texture, const Ref<Sprite2D>& subTex = nullptr, float tilingFactor = 1.0f);

		static const Ref<GraphicsPipeline>& getGraphicsPipeline();
		static const Ref<Shader>& getShader();
		static const Ref<RenderPass>& getRenderPass();
		static const Ref<Texture2D>& getColorOutput();
		static const Ref<Texture2D>& getWhiteTexture();

		static uint32_t addRenderGraph(const Ref<RenderGraph>& rendergraph);
		static void setExeRenderGraph(uint32_t rendergraphId);
		static void setColorOutput(const Ref<Texture2D>& image);

		// stats
		struct Statistics
		{
			uint32_t drawCall = 0;
			uint32_t quadCount = 0;

			inline uint32_t getTotalVertexCount() const { return quadCount * 4; }
			inline uint32_t getTotalIndexCount() const { return quadCount * 6; }
		};
		static void resetStats();
		static const Statistics& getStats();
	};
}