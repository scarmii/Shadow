#pragma once

#include "shadow/core/types.h"
#include "shadow/vulkan/texture.h"
#include "shadow/vulkan/pipeline.h"
#include "shadow/vulkan/renderPass.h"
#include "shadow/vulkan/commandBuffer.h"

#include <vulkan/vulkan.h>

namespace Shadow
{
	class RenderGraph;

	//enum class ResourceType
	//{
	//	None   = 0,
	//	Image  = 1,
	//	Buffer = 2
	//};

	enum class ResourceAccess
	{
		None         = 0,
		ReadOnly     = 1,
		WriteOnly    = 2,
		ReadAndWrite = 3
	};

	enum class DrawPassFlags
	{
		None =                 0,
		FirstSubpass =    1 << 0,
		LastSubpass =     1 << 1,
		HasDepthStencil = 1 << 2
	};
	SH_FLAG(DrawPassFlags);

	struct AttachmentInfo
	{
		ImageFormat format = ImageFormat::None;
		AttachmentLoadOp loadOp = AttachmentLoadOp::Clear;
		uint32_t samples = 1;
		uint32_t levels = 1;
		uint32_t layers = 1;
	};

	struct BufferInfo
	{
		VkDeviceSize size = 0;
		VkBufferUsageFlags usage = 0;
	};

	struct Barrier
	{
		VkPipelineStageFlags srcStage;
		VkPipelineStageFlags dstStage;
		VkImageMemoryBarrier imageBarrier;
	};

	class ImageResource
	{
	public:
		inline void setAttachmentInfo(const AttachmentInfo& info) { m_attachmentInfo = info; }
		inline void addImageUsage(ImageUsage usage) { m_imageUsage |= usage; }
		inline void setTexture(const Ref<Texture2D>& texture) { m_texture = texture; }
		inline void setName(const std::string& name) { m_name = name; }

		void readInPass(const std::string& name) { m_readInPasses.emplace(name); }
		void writtenInPass(const std::string& name) { m_writtenInPasses.emplace(name); }

		bool isReadInPass(const std::string& name) const { return m_readInPasses.count(name) > 0; }
		bool isWrittenInPass(const std::string& name) const { return m_writtenInPasses.count(name) > 0; }

		inline const std::string& getName() const { return m_name; }
		inline ImageUsage getImageUsage() const { return m_imageUsage; }
		inline const AttachmentInfo& getAttachmentInfo() const { return m_attachmentInfo; }
		inline const Ref<Texture2D>& getTexture() const { return m_texture; }
		inline const std::unordered_set<std::string>& getReadInPasses() const { return m_readInPasses; }
		inline const std::unordered_set<std::string>& getWrittenInPasses() const { return m_writtenInPasses; }
	private:
		std::string m_name;
		AttachmentInfo m_attachmentInfo;
		ImageUsage m_imageUsage;
		Ref<Texture2D> m_texture;

		std::unordered_set<std::string> m_readInPasses;
		std::unordered_set<std::string> m_writtenInPasses;
	};

	class RenderGraphPass
	{
	public:
		RenderGraphPass(const std::string& name, QueueType type);
		virtual ~RenderGraphPass();

		inline void setCallback(const std::function<void()>& callback) { m_callback = callback; }

		void addBarrier(const Barrier& barrier);
		void execute(const Ref<CommandBuffer>& cmdBuffer);

		inline const std::string& getName() const { return m_name; }
		inline QueueType getType() const { return m_type; }
		inline const std::vector<Barrier>& getBarriers() const { return m_barriers; }
	protected:
		virtual void begin(const Ref<CommandBuffer>& cmdBuffer) = 0;
		virtual void end(const Ref<CommandBuffer>& cmdBuffer) = 0;
	private:
		std::string m_name;
		QueueType m_type;
		std::function<void()> m_callback;

		std::vector<Barrier> m_barriers;
	};

	class DrawPass : public RenderGraphPass
	{
	public:
		DrawPass(RenderGraph& renderGraph, const std::string& name, const Ref<GraphicsPipeline>& pipe, const Ref<Shader>& shader);
		virtual ~DrawPass();

		inline void setFirstSubpass(bool state) { m_flags |= DrawPassFlags::FirstSubpass; }
		inline void setLastSubpass(bool state) { m_flags |= DrawPassFlags::LastSubpass; }
		void setCompatibleRenderPass(const Ref<RenderPass>& renderPass);

		void addBarrier(const Barrier& barrier);
	    void execute(const Ref<CommandBuffer>& cmdBuffer);

		ImageResource& addColorOutput(const std::string& name, const AttachmentInfo& info);
		ImageResource& setDepthStencilOutput(const std::string& name, const AttachmentInfo& info);
		void addInputAttachment(const std::string& name);
		void addTextureInput(const std::string& name);

		inline bool hasDepthAttachment() const { return m_flags & DrawPassFlags::HasDepthStencil; }
		inline const Ref<GraphicsPipeline>& getGraphicsPipeline() const { return m_graphicsPipeline; }
		inline const Ref<Shader>& getShader() const { return m_shader; }
		inline const Ref<RenderPass>& getCompatibleRenderPass() const { return m_compatibleRenderPass; }
		inline std::vector<ImageResource*>& getColorOutputs() { return m_colorOutputs; }
		inline std::vector<ImageResource*>& getTextureInputs() { return m_textureInputs; }
		inline ImageResource* getDepthOutput() const { return m_flags & DrawPassFlags::HasDepthStencil ? m_depthStencilOutput : nullptr; }
		inline std::vector<std::string>& getInputAttachments() { return m_inputAttachments; }
	private:
		virtual void begin(const Ref<CommandBuffer>& cmdBuffer) override;
		virtual void end(const Ref<CommandBuffer>& cmdBuffer) override;
	private:
		RenderGraph& m_renderGraph;
		Ref<Shader> m_shader;	
		Ref<GraphicsPipeline> m_graphicsPipeline = nullptr;
		Ref<RenderPass> m_compatibleRenderPass;
		DrawPassFlags m_flags;

		std::vector<ImageResource*> m_colorOutputs;
		std::vector<ImageResource*> m_textureInputs;
		ImageResource* m_depthStencilOutput = nullptr;
		std::vector<std::string> m_inputAttachments;
	};

	class ComputePass : public RenderGraphPass
	{
	public:
		ComputePass(RenderGraph& renderGraph, const std::string& name, const Ref<ComputePipeline>& pipe);
		virtual ~ComputePass();

		void addStorageOutput(const std::string& name);
		void addStorageInput(const std::string& name);

		inline const Ref<ComputePipeline>& getComputePipeline() const { return m_computePipeline; }
		inline std::vector<ImageResource*>& getStorageOutputs() { return m_storageOutputs; }
		inline std::vector<ImageResource*>& getStorageInputs() { return m_storageInputs; }
	private:
		virtual void begin(const Ref<CommandBuffer>& cmdBuffer) override;
		virtual void end(const Ref<CommandBuffer>& cmdBuffer) override;
	private:
		RenderGraph& m_renderGraph;
		std::function<void()> m_callback;
		Ref<ComputePipeline> m_computePipeline;

		std::vector<ImageResource*> m_storageOutputs;
		std::vector<ImageResource*> m_storageInputs;
	};

	class RenderGraph
	{
	public:
		~RenderGraph();

		Ref<DrawPass> addDrawPass(const std::string& name, const Ref<GraphicsPipeline>& pipe, const Ref<Shader>& shader);
		Ref<ComputePass> addComputePass(const std::string& name, const Ref<ComputePipeline>& pipe);

		void setup(const glm::vec4 clearColor = {0.025f,0.025f,0.025f,1.0f});
		void setup(const FramebufferInfo& info, const glm::vec4 clearColor = { 0.025f,0.025f,0.025f,1.0f });
		void execute(const Ref<CommandBuffer>& cmdBuffer);
		void resizeFramebuffers(uint32_t newWidth, uint32_t newHeight);

	    ImageResource& getImageResource(const std::string& name);

		inline const std::vector<Ref<RenderGraphPass>>& getPasses() const { return m_passes; }
		inline const std::vector<Ref<RenderPass>>& getRenderPasses() const { return m_renderPasses; }
		inline const Ref<RenderGraphPass>& getPass(const std::string& name) const { return m_passes[m_passIndices[name]]; }
		inline const Ref<RenderGraphPass>& getPass(uint32_t index) const { return m_passes[index]; }
	private:
		void setupPasses(std::vector<SubpassAttachment>& attachments);
		void distributePasses();
		void mergePasses(const std::vector<std::string>& passNames);
		void createRenderPasses(const FramebufferInfo& info, const glm::vec4 clearColor);
		void setTextures();
		void transitionImageLayouts();
		void transitionImageLayout(ImageResource* imageRes, VkImageLayout initialLayout);
		void setupBarriers();
		void initImageBarrier(ImageResource* pRes, VkImageMemoryBarrier* pBarrier);
	private:
		uint8_t m_activePasses;
		std::vector<Ref<RenderGraphPass>> m_passes;
		std::vector<Ref<RenderPass>> m_renderPasses;

		mutable std::unordered_map<std::string, uint32_t> m_passIndices;
		std::unordered_map<std::string, ImageResource> m_imageResources;

		struct MergedPass
		{
			std::vector<std::string> passNames;
			std::vector<Subpass> subpasses;
			RenderPassConfig config;
		};
		std::vector<MergedPass> m_mergedPasses;
	};
}