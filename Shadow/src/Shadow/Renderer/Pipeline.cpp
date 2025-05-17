#include "shpch.hpp"
#include "Shadow/Renderer/Pipeline.hpp"
#include "Shadow/Renderer/Renderer.hpp"
#include "Shadow/Core/Core.hpp"
         
#include "Shadow/Vulkan/VulkanPipeline.hpp"
#include "Shadow/Vulkan/VulkanContext.hpp"

namespace Shadow
{
    SH_FLAG_DEF(PipelineStages, uint32_t);
    SH_FLAG_DEF(AccessFlags, uint32_t);

    Ref<GraphicsPipeline> GraphicsPipeline::create(const GraphicsPipeConfiguration& config)
    {
        switch (Renderer::getRendererType())
        {
            case RendererType::None:   return nullptr;
            case RendererType::Vulkan: return createRef<VulkanGraphicsPipeline>(config);
        }
        SH_TRACE("failed to create a pipeline: it seems you're using unknown renderer API :(");
        return nullptr;
    }

    Ref<ComputePipeline> ComputePipeline::create(const Ref<Shader>& computeShader)
    {
        switch (Renderer::getRendererType())
        {
            case RendererType::None:   return nullptr;
            case RendererType::Vulkan: return createRef<VulkanComputePipeline>(computeShader);
        }
        SH_TRACE("failed to create a compute pipeline: it seems you're using unknown renderer API :(");
        return nullptr;
    }
}