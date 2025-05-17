#include "shpch.hpp"
#include "Shadow/Core/Core.hpp"

#include "Shadow/Renderer/Renderer.hpp"
#include "Shadow/Renderer/GraphicsContext.hpp"
#include "Shadow/Vulkan/VulkanContext.hpp"

namespace Shadow
{
    void GraphicsContext::create(GLFWwindow* windowHandle)
    {
        switch (Renderer::getRendererType())
        {
            case RendererType::Vulkan: s_ctx = new VulkanContext(windowHandle); break;
        }

        if (!s_ctx)
            SH_ASSERT(false, "Failed to create graphics context");
    }

    void GraphicsContext::destroy()
    {
        delete s_ctx;
    }
}