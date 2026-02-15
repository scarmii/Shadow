#pragma once

#include "shadow/renderer/mesh.h"
#include "shadow/renderer/camera.h"
#include "shadow/vulkan/pipeline.h"
#include "shadow/vulkan/buffer.h"
#include "shadow/vulkan/shader.h"

struct GLFWwindow;

namespace Shadow
{
	class RenderGraph;

	enum class RendererType
	{
		None   = 0,
		Vulkan = 1
	};

	class Renderer
	{
	public:
		static void init();
		static void shutdown();

		static void begin();
		static void end(const Camera& camera, const glm::mat4& transform);
		static void flush();
		static void resizeFramebuffers(uint32_t width, uint32_t height);
		static void synchronizeRendering();

		static void drawMesh(const Ref<Mesh>& mesh, glm::mat4& trasnform);

		static uint32_t addRenderGraph(const Ref<RenderGraph>& rendergraph);
		static void setExeRenderGraph(uint32_t rendergraphId);
		static void setColorOutput(const Ref<Texture2D>& image);

		static ShaderLibrary& getShaderLibrary();
		static RendererType getRendererType();
		static const Ref<Texture2D>& getImageOut();
		static const Ref<CommandBuffer>& getTransferCmdBuffer();
	};
}