#pragma once

#include "Shadow/Renderer/Camera.hpp"
#include "Shadow/Renderer/Texture.hpp"

namespace Shadow
{
	struct QuadProperties
	{
		glm::vec3 position{0.0f};
		glm::vec2 size{1.0f};
		glm::vec4 color{ 1.0f };
		Ref<Texture2D> texture = nullptr;
		float tilingFactor = 1.0f;
	};

	class Renderer2D
	{
	public:
		static void init();
		static void shutdown();

		static void beginScene(const OrthoCamera& camera);
		static void endScene();
		static void flush();

		// primitives
		static void drawQuad(const QuadProperties& properties);
		static void drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);    
		static void drawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);
		static void drawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor = 1.0f);
		static void drawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor = 1.0f);
		static void drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color, const Ref<Texture2D>& texture, float tilingFactor = 1.0f);
		static void drawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, const Ref<Texture2D>& texture, float tilingFactor = 1.0f);

		static void drawRotatedQuad(const QuadProperties& properties, float angle);
		static void drawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float angle, const glm::vec4& color);
		static void drawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float angle, const glm::vec4& color);
		static void drawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float angle, const Ref<Texture2D>& texture, float tilingFactor = 1.0f);
		static void drawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float angle, const Ref<Texture2D>& texture, float tilingFactor = 1.0f);
		static void drawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float angle, const glm::vec4& color, const Ref<Texture2D>& texture, float tilingFactor = 1.0f);
		static void drawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float angle, const glm::vec4& color, const Ref<Texture2D>& texture, float tilingFactor = 1.0f);

		// stats
		struct Statistics
		{
			uint32_t drawCalls = 0;
			uint32_t quadCount = 0;

			inline uint32_t getTotalVertexCount() const { return quadCount * 4; }
			inline uint32_t getTotalIndexCount() const { return quadCount * 6; }
		};
		static void resetStats();
		static const Statistics& getStats();
	private:
		static uint32_t retrieveTexIndex(const Ref<Texture2D>& texture);
		static void setVerticesData(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, uint32_t texIndex, float tilingFactor);
		static void setVerticesData(const glm::mat4& transform, const glm::vec4& color, uint32_t texIndex, float tilingFactor);
	};
}