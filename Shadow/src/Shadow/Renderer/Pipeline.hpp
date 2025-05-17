#pragma once

#include "Shadow/Renderer/PipelineStatesConfig.hpp"
#include "Shadow/Renderer/UniformBuffer.hpp"
#include "Shadow/Renderer/Renderpass.hpp"
#include "Shadow/Renderer/VertexDescription.hpp"
#include "Shadow/Renderer/Buffer.hpp"
#include "Shadow/Renderer/Texture.hpp"
#include "Shadow/Renderer/Shader.hpp"

namespace Shadow
{

    // identical to VkPipelineStageFlagBits (bitfield)
	enum class PipelineStages 
	{
        None                  = 0,
        TopOfPipe             = 0x00000001,
        DrawIndirect          = 0x00000002,
        VertexInput           = 0x00000004,
        VertexShader          = 0x00000008,
        FragmentShader        = 0x00000080,
        EarlyFragmentTests    = 0x00000100,
        LateFragmentTests     = 0x00000200,
        ColorAttachmentOutput = 0x00000400,
        ComputeShader         = 0x00000800,
        Transfer              = 0x00001000,
        BottomOfPipe          = 0x00002000,
	};
    SH_FLAG(PipelineStages);

    // identical to VkAccessFlagBits (bitfield)
    enum class AccessFlags
    {
        None                        = 0,
        IndirectCommandRead         = 0x00000001,
        IndexRead                   = 0x00000002,
        VertexAttributeRead         = 0x00000004,
        UniformRead                 = 0x00000008,
        InputAttachmentRead         = 0x00000010,
        ShaderRead                  = 0x00000020,
        ShaderWrite                 = 0x00000040,
        ColorAttachmentRead         = 0x00000080,
        ColorAttachmentWrite        = 0x00000100,
        DepthStencilAttachmentRead  = 0x00000200,
        DepthStencilAttachmentWrite = 0x00000400,
        TransferRead                = 0x00000800,
        TransferWrite               = 0x00001000,
        HostRead                    = 0x00002000,
        HostWrite                   = 0x00004000,
        MemoryRead                  = 0x00008000,
        MemoryWrite                 = 0x00010000,
    };
    SH_FLAG(AccessFlags)

	struct GraphicsPipeConfiguration
	{
		uint32_t subpass = 0;
		Ref<Renderpass> renderpass = nullptr;
		Ref<Shader> shader = nullptr;
		const VertexInput* vertexInput = nullptr;
		const VertexInput* instanceInput = nullptr;
		GraphicsPipeStates states = GraphicsPipeStates();
		bool useDynamicRendering = false;
	};

	class GraphicsPipeline
	{
	public:
		virtual ~GraphicsPipeline() = default;

        virtual void setSubpassInput(const std::string& shaderName, uint32_t inputAttachmentRef) = 0;
        virtual void setRenderpassInput(const std::string& shaderName, uint32_t imageIndex, const Ref<Renderpass>& src) = 0;

        virtual const GraphicsPipeConfiguration& getConfiguration() const = 0;

		static Ref<GraphicsPipeline> create(const GraphicsPipeConfiguration& config);
	};

    class ComputePipeline
    {
    public:
        virtual ~ComputePipeline() = default;

        static Ref<ComputePipeline> create(const Ref<Shader>& computeShader); 
    };
}