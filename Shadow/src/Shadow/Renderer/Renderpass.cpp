#include "shpch.hpp"
#include "Shadow/Renderer/Renderpass.hpp"
#include "Shadow/Renderer/Renderer.hpp"
#include "Shadow/Events/EventDispatcher.hpp"

#include "Shadow/Vulkan/VulkanRenderpass.hpp"
#include "Shadow/Vulkan/VulkanContext.hpp"

namespace Shadow
{
	Ref<Renderpass> Renderpass::create(const RenderpassConfig& config)
	{
		switch (Renderer::getRendererType())
		{
			case RendererType::None: return nullptr;
			case RendererType::Vulkan: return createRef<VulkanRenderpass>(config);
		}
		SH_ASSERT(false, "Failed to create a framebuffer :<");
		return nullptr;
	}
}