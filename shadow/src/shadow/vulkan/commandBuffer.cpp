#include "shpch.h"

#include "shadow/renderer/mesh.h"
#include "shadow/vulkan/commandBuffer.h"
#include "shadow/vulkan/context.h"
#include "shadow/vulkan/renderPass.h"

namespace Shadow
{
	Semaphore::Semaphore(uint64_t initialValue)
		: m_timeline{initialValue, initialValue+1}
	{
		Device* device = VulkanContext::getDevice();

		VkSemaphoreTypeCreateInfo timelineCI{};
		timelineCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
		timelineCI.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		timelineCI.initialValue = initialValue;

		VkSemaphoreCreateInfo semaphoreCI{};
		semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreCI.flags = 0;
		semaphoreCI.pNext = &timelineCI;

		for (uint32_t i = 0; i < Device::s_maxFramesInFlight; i++)
			VK_CHECK_RESULT(vkCreateSemaphore(device->getVkDevice(), &semaphoreCI, nullptr, &m_semaphores[i]));
	}

	Semaphore::Semaphore(bool createSignaled)
		: m_signaled(createSignaled)
	{
		Device* device = VulkanContext::getDevice();

		VkSemaphoreCreateInfo semaphoreCI{};
		semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreCI.flags = 0;

		for (uint32_t i = 0; i < Device::s_maxFramesInFlight; i++)
			VK_CHECK_RESULT(vkCreateSemaphore(device->getVkDevice(), &semaphoreCI, nullptr, &m_semaphores[i]));

		if (createSignaled)
		{
			VkCommandBuffer vkCmdBuffer = device->beginSingleTimeCmdBuffer(QueueType::Graphics);
			VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

			VkSubmitInfo signalSemaphores{};
			signalSemaphores.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			signalSemaphores.commandBufferCount = 1;
			signalSemaphores.pCommandBuffers = &vkCmdBuffer;
			signalSemaphores.signalSemaphoreCount = static_cast<uint32_t>(m_semaphores.size());
			signalSemaphores.pSignalSemaphores = m_semaphores.data();
			signalSemaphores.waitSemaphoreCount = 0;
			signalSemaphores.pWaitDstStageMask = &waitStage;

			vkEndCommandBuffer(vkCmdBuffer);
			vkQueueSubmit(device->getGraphicsQueue(), 1, &signalSemaphores, VK_NULL_HANDLE);

			vkQueueWaitIdle(device->getGraphicsQueue());
			vkFreeCommandBuffers(device->getVkDevice(), device->getGraphicsCmdPool(), 1, &vkCmdBuffer);
		}
	}

	Semaphore::~Semaphore()
	{
		for (uint32_t i = 0; i < Device::s_maxFramesInFlight; i++)
			vkDestroySemaphore(VulkanContext::getDevice()->getVkDevice(), m_semaphores[i], nullptr);
	}

	CommandBuffer::CommandBuffer(QueueType type)
		: m_type(type), m_waitImageAvailable(false)
	{
		if (type == QueueType::None)
			SH_WARN("a command buffer with QueueType::None queue isn't supposed to be submitted at all");

		createCommandBuffers(type);
		createFences();

		for (uint32_t i = 0; i < Device::s_maxFramesInFlight; i++)
		{
			m_waits[i].reserve(5);
			m_signals[i].reserve(5);
		}
	}

	CommandBuffer::~CommandBuffer()
	{
		VkDevice vkDevice = VulkanContext::getDevice()->getVkDevice();
		vkDeviceWaitIdle(vkDevice);

		for (uint32_t i = 0; i < Device::s_maxFramesInFlight; i++)
			vkDestroyFence(vkDevice, m_inFlightFences[i], nullptr);
	}

	void CommandBuffer::begin()
	{
		SH_PROFILE_RENDERER_FUNCTION();
		Device* device = VulkanContext::getDevice();

		vkWaitForFences(device->getVkDevice(), 1, &m_inFlightFences[device->currentFrame()], VK_TRUE, UINT64_MAX);

		if (m_waitImageAvailable)
			device->acquireSwapchainImage();

		vkResetFences(device->getVkDevice(), 1, &m_inFlightFences[device->currentFrame()]);
		vkResetCommandBuffer(m_cmdBuffers[device->currentFrame()], 0);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		vkBeginCommandBuffer(m_cmdBuffers[device->currentFrame()], &beginInfo);
	}

	void CommandBuffer::end()
	{
		vkEndCommandBuffer(m_cmdBuffers[VulkanContext::getDevice()->currentFrame()]);
	}

	void CommandBuffer::submit()
	{
		SH_PROFILE_RENDERER_FUNCTION();
		Device* device = VulkanContext::getDevice();

		VkCommandBufferSubmitInfo cmdSubmit{};
		cmdSubmit.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		cmdSubmit.commandBuffer = m_cmdBuffers[device->currentFrame()];

		VkSubmitInfo2 submit{};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		submit.commandBufferInfoCount = 1;
		submit.pCommandBufferInfos = &cmdSubmit;
		submit.waitSemaphoreInfoCount = static_cast<uint32_t>(m_waits[device->currentFrame()].size());
		submit.pWaitSemaphoreInfos = m_waits[device->currentFrame()].data();
		submit.signalSemaphoreInfoCount = static_cast<uint32_t>(m_signals[device->currentFrame()].size());
		submit.pSignalSemaphoreInfos = m_signals[device->currentFrame()].data();
		vkQueueSubmit2(device->getQueue(m_type), 1, &submit, m_inFlightFences[device->currentFrame()]);
	}

	void CommandBuffer::reset()
	{
		vkResetCommandBuffer(m_cmdBuffers[VulkanContext::getDevice()->currentFrame()], 0);
	}

	void CommandBuffer::addWaitSemaphore(const Ref<Semaphore>& semaphore, PipelineStages waitStages)
	{
		VkSemaphoreSubmitInfo waitSemaphore{};
		waitSemaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		waitSemaphore.stageMask = static_cast<VkPipelineStageFlags>(waitStages);

		for (uint32_t i = 0; i < Device::s_maxFramesInFlight; i++)
		{
			waitSemaphore.semaphore = semaphore->getVkSemaphore(i);
			m_waits[i].emplace_back(waitSemaphore);
		}
	}

	void CommandBuffer::addSignalSemaphore(const Ref<Semaphore>& semaphore, PipelineStages signalStages)
	{
		VkSemaphoreSubmitInfo signalSemaphore{};
		signalSemaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		signalSemaphore.stageMask = static_cast<VkPipelineStageFlags>(signalStages);

		for (uint32_t i = 0; i < Device::s_maxFramesInFlight; i++)
		{
			signalSemaphore.semaphore = semaphore->getVkSemaphore(i);
			m_signals[i].emplace_back(signalSemaphore);
		}
	}

	void CommandBuffer::addWaitSemaphore(const Scope<Semaphore>& semaphore, PipelineStages waitStages)
	{
		if (semaphore == VulkanContext::getDevice()->getImageAvailableSem())
			m_waitImageAvailable = true;

		VkSemaphoreSubmitInfo waitSemaphore{};
		waitSemaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		waitSemaphore.stageMask = static_cast<VkPipelineStageFlags>(waitStages);

		for (uint32_t i = 0; i < Device::s_maxFramesInFlight; i++)
		{
			waitSemaphore.semaphore = semaphore->getVkSemaphore(i);
			m_waits[i].emplace_back(waitSemaphore);
		}
	}

	void CommandBuffer::addSignalSemaphore(const Scope<Semaphore>& semaphore, PipelineStages signalStages)
	{
		VkSemaphoreSubmitInfo signalSemaphore{};
		signalSemaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		signalSemaphore.stageMask = static_cast<VkPipelineStageFlags>(signalStages);

		for (uint32_t i = 0; i < Device::s_maxFramesInFlight; i++)
		{
			signalSemaphore.semaphore = semaphore->getVkSemaphore(i);
			m_signals[i].emplace_back(signalSemaphore);
		}
	}

	void CommandBuffer::setViewport(float x, float y, float width, float height)
	{
		SH_PROFILE_RENDERER_FUNCTION();
		Device* device = VulkanContext::getDevice();

		VkViewport viewport{};
		viewport.x = x;
		viewport.y = y;
		viewport.width = width;
		viewport.height = height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(m_cmdBuffers[device->currentFrame()], 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = device->getSwapchain()->getExtent();
		vkCmdSetScissor(m_cmdBuffers[device->currentFrame()], 0, 1, &scissor);
	}

	void CommandBuffer::beginRenderPass(Ref<RenderPass>& renderpass)
	{
		SH_PROFILE_RENDERER_FUNCTION();

		m_drawData.renderpass = renderpass;
		VkCommandBuffer cmdBuffer = m_cmdBuffers[VulkanContext::getDevice()->currentFrame()];

		auto& vkPipe = m_drawData.renderpass->getGraphicsPipeline(0);
		auto& descriptorSets = vkPipe->getDescriptorSets();
		auto& pipeConfig = vkPipe->getConfiguration();

		VkRenderPassBeginInfo beginInfo{};
		m_drawData.renderpass->initBeginInfo(beginInfo);

		vkCmdBeginRenderPass(cmdBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipe->getVkPipeline());

		if (descriptorSets.size)
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipe->getLayout(), 0, 1, descriptorSets.data(), 0, nullptr);
	}

	void CommandBuffer::endRenderPass()
	{
		SH_PROFILE_RENDERER_FUNCTION();

		m_drawData.subpass = 0;
		vkCmdEndRenderPass(m_cmdBuffers[VulkanContext::getDevice()->currentFrame()]);
	}

	void CommandBuffer::nextSubpass()
	{
		SH_PROFILE_RENDERER_FUNCTION();

		m_drawData.subpass++;
		VkCommandBuffer cmdBuffer = m_cmdBuffers[VulkanContext::getDevice()->currentFrame()];

		auto& vkPipe = m_drawData.renderpass->getGraphicsPipeline(m_drawData.subpass);
		auto& descriptorSets = vkPipe->getDescriptorSets();

		vkCmdNextSubpass(cmdBuffer, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipe->getVkPipeline());

		if (descriptorSets.size)
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipe->getLayout(), 0, 1, descriptorSets.data(), 0, nullptr);
	}

	void CommandBuffer::setPushConstants(const void* data, uint32_t size, ShaderStage stageMask)
	{
		SH_PROFILE_RENDERER_FUNCTION();

		auto& vkPipe = m_drawData.renderpass->getGraphicsPipeline(m_drawData.subpass);

		vkCmdPushConstants(m_cmdBuffers[VulkanContext::getDevice()->currentFrame()], vkPipe->getLayout(),
			static_cast<VkShaderStageFlags>(stageMask), 0, size, data);
	}

	void CommandBuffer::setPushConstants(const Ref<ComputePipeline>& pipe, const void* data, uint32_t size, ShaderStage stageMask)
	{
		SH_PROFILE_RENDERER_FUNCTION();

		vkCmdPushConstants(m_cmdBuffers[VulkanContext::getDevice()->currentFrame()], pipe->getLayout(),
			static_cast<VkShaderStageFlags>(stageMask), 0, size, data);
	}

	void CommandBuffer::drawMesh(const Mesh& mesh)
	{
		SH_PROFILE_RENDERER_FUNCTION();

		uint32_t currentFrame = VulkanContext::getDevice()->currentFrame();
		VkCommandBuffer cmdBuffer = m_cmdBuffers[currentFrame];

		VkBuffer vb = mesh.getVertexBuffer()->getVkBuffer(currentFrame);
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vb, &offset);

		auto& indexBuffer = mesh.getIndexBuffer();
		vkCmdBindIndexBuffer(cmdBuffer, indexBuffer->getVkBuffer(), offset, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(cmdBuffer, indexBuffer->getCount(), 1, 0, 0, 0);
	}

	void CommandBuffer::drawMesh(Mesh& mesh, glm::mat4& trasnform)
	{
		mesh.setTransform(trasnform);
		mesh.updateVertexBuffer(*this);
		drawMesh(mesh);
	}

	void CommandBuffer::draw(uint32_t verticesCount, uint32_t firstVertex)
	{
		SH_PROFILE_RENDERER_FUNCTION();
		vkCmdDraw(m_cmdBuffers[VulkanContext::getDevice()->currentFrame()], verticesCount, 1, firstVertex, 0);
	}

	void CommandBuffer::draw(const Ref<VertexBuffer>& vertexBuffer, uint32_t vertexCount)
	{
		SH_PROFILE_RENDERER_FUNCTION();

		uint32_t currentFrame = VulkanContext::getDevice()->currentFrame();
		uint32_t vertCount = vertexCount ? vertexCount : vertexBuffer->getVertexCount();

		VkCommandBuffer cmdBuffer = m_cmdBuffers[currentFrame];
		VkBuffer buffer = vertexBuffer->getVkBuffer(currentFrame);
		VkDeviceSize offset = 0;

		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &buffer, &offset);
		vkCmdDraw(cmdBuffer, vertCount, 1, 0, 0);
	}

	void CommandBuffer::drawIndexed(const Ref<VertexBuffer>& vertexBuffer, const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount)
	{
		SH_PROFILE_RENDERER_FUNCTION();

		uint32_t count = indexCount ? indexCount : indexBuffer->getCount();
		uint32_t currentFrame = VulkanContext::getDevice()->currentFrame();

		VkCommandBuffer cmdBuffer = m_cmdBuffers[currentFrame];
		VkBuffer vkVertexBuffer = vertexBuffer->getVkBuffer(currentFrame);
		VkBuffer vkIndexBuffer = indexBuffer->getVkBuffer();

		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vkVertexBuffer, &offset);
		vkCmdBindIndexBuffer(cmdBuffer, vkIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(cmdBuffer, count, 1, 0, 0, 0);
	}

	void CommandBuffer::drawIndexed(const Ref<VertexBuffer>& vertexBuffer, const Ref<IndexBuffer>& indexBuffer, const DrawArgs& args)
	{
		SH_PROFILE_RENDERER_FUNCTION();
		uint32_t currentFrame = VulkanContext::getDevice()->currentFrame();

		VkCommandBuffer cmdBuffer = m_cmdBuffers[currentFrame];
		VkBuffer vkVertexBuffer = vertexBuffer->getVkBuffer(currentFrame);
		VkBuffer vkIndexBuffer = indexBuffer->getVkBuffer();

		VkDeviceSize offset = args.vertexBufferOffset;
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vkVertexBuffer, &offset);
		vkCmdBindIndexBuffer(cmdBuffer, vkIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(cmdBuffer, args.indexCount, args.instanceCount, args.firstIndex, args.firstVertex, args.firstInstance);
	}

	void CommandBuffer::drawInstanced(const Ref<VertexBuffer>& vertexBuffer, const Ref<VertexBuffer>& instanceBuffer, uint32_t instanceCount)
	{
		SH_PROFILE_RENDERER_FUNCTION();

		uint32_t count = instanceCount ? instanceCount : instanceBuffer->getVertexCount();
		uint32_t currentFrame = VulkanContext::getDevice()->currentFrame();
		VkCommandBuffer cmdBuffer = m_cmdBuffers[currentFrame];

		VkBuffer vertexBuffers[2] = {
			vertexBuffer->getVkBuffer(currentFrame),
			instanceBuffer->getVkBuffer(currentFrame)
		};

		VkDeviceSize offsets[2] = { 0,0 };
		vkCmdBindVertexBuffers(cmdBuffer, 0, 2, vertexBuffers, offsets);
		vkCmdDraw(cmdBuffer, vertexBuffer->getVertexCount(), count, 0, 0);
	}

	void CommandBuffer::drawInstanced(const Ref<VertexBuffer>& vertexBuffer, const Ref<VertexBuffer>& instanceBuffer, const Ref<IndexBuffer>& indexBuffer, uint32_t instanceCount)
	{
		SH_PROFILE_RENDERER_FUNCTION();

		uint32_t count = instanceCount ? instanceCount : instanceBuffer->getVertexCount();
		uint32_t currentFrame = VulkanContext::getDevice()->currentFrame();
		VkCommandBuffer cmdBuffer = m_cmdBuffers[currentFrame];

		VkBuffer vertexBuffers[2] = {
			vertexBuffer->getVkBuffer(currentFrame),
			instanceBuffer->getVkBuffer(currentFrame)
		};

		VkDeviceSize offsets[2] = { 0,0 };
		vkCmdBindVertexBuffers(cmdBuffer, 0, 2, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(cmdBuffer, indexBuffer->getVkBuffer(), 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(cmdBuffer, indexBuffer->getCount(), count, 0, 0, 0);
	}

	void CommandBuffer::clearColor(const Ref<Texture2D>& image, ImageLayout layout, PipelineStages srcStage, AccessFlags srcAcces, const glm::vec4 clearValue)
	{
		transitionImageLayout(image, layout, ImageLayout::TransferDstOptimal,
			srcStage, PipelineStages::Transfer, srcAcces, AccessFlags::TransferWrite);

		VkImageSubresourceRange range{};
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.baseMipLevel = 0;
		range.baseArrayLayer = 0;
		range.levelCount = image->getMipLevelCount();
		range.layerCount = 1;

		VkClearColorValue clearColor{clearValue.r, clearValue.g, clearValue.b, clearValue.a};
		vkCmdClearColorImage(m_cmdBuffers[VulkanContext::getDevice()->currentFrame()], image->getVkImage(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range);

		transitionImageLayout(image, ImageLayout::TransferDstOptimal, layout,
			PipelineStages::Transfer, srcStage, AccessFlags::TransferWrite, srcAcces);
	}

	void CommandBuffer::transitionImageLayout(const Ref<Texture2D>& image, ImageLayout oldLayout, ImageLayout newLayout,
										   PipelineStages srcStageMask, PipelineStages dstStageMask,
										   AccessFlags srcAccess, AccessFlags dstAccess)
	{
		SH_PROFILE_RENDERER_FUNCTION();

		VkImageMemoryBarrier imageBarrier{};
		imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageBarrier.image = image->getVkImage();
		imageBarrier.oldLayout = static_cast<VkImageLayout>(oldLayout);
		imageBarrier.newLayout = static_cast<VkImageLayout>(newLayout);
		imageBarrier.srcAccessMask = static_cast<VkAccessFlags>(srcAccess);
		imageBarrier.dstAccessMask = static_cast<VkAccessFlags>(dstAccess);
		imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.subresourceRange.aspectMask = image->getImageAspect();
		imageBarrier.subresourceRange.layerCount = 1;
		imageBarrier.subresourceRange.baseArrayLayer = 0;
		imageBarrier.subresourceRange.levelCount = image->getMipLevelCount();
		imageBarrier.subresourceRange.baseMipLevel = 0;

		vkCmdPipelineBarrier(m_cmdBuffers[VulkanContext::getDevice()->currentFrame()],
			static_cast<VkPipelineStageFlags>(srcStageMask), static_cast<VkPipelineStageFlags>(dstStageMask), 0,
			0, nullptr, 0, nullptr, 1, &imageBarrier);
	}

	void CommandBuffer::beginComputePass(const Ref<ComputePipeline>& pipe)
	{
		SH_PROFILE_RENDERER_FUNCTION();

		VkDevice vkDevice = VulkanContext::getDevice()->getVkDevice();
		VkCommandBuffer cmdBuffer = m_cmdBuffers[VulkanContext::getDevice()->currentFrame()];

		auto& descriptorSets = pipe->getDescriptorSets();
		auto& pushConstants = pipe->getPushConstantRanges();

		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipe->getVkPipeline());

		if (descriptorSets.size)
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipe->getLayout(), 0, 1, descriptorSets.data(), 0, nullptr);
	}

	void CommandBuffer::endComputePass()
	{
	}

	void CommandBuffer::dispatch(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
	{
		SH_PROFILE_RENDERER_FUNCTION();
		vkCmdDispatch(m_cmdBuffers[VulkanContext::getDevice()->currentFrame()], groupX, groupY, groupZ);
	}

	void CommandBuffer::createCommandBuffers(QueueType type)
	{
		Device* device = VulkanContext::getDevice();
	    VkCommandPool cmdPool;

		switch (type)
		{
			case QueueType::Compute:   cmdPool = device->getComputeCmdPool(); break;
			case QueueType::Transfer:  cmdPool = device->getTransferCmdPool(); break;
			case QueueType::Graphics:  cmdPool = device->getGraphicsCmdPool(); break;
			default:                   cmdPool = device->getGraphicsCmdPool(); break;
		}

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = cmdPool;
		allocInfo.commandBufferCount = Device::s_maxFramesInFlight;
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device->getVkDevice(), &allocInfo, m_cmdBuffers.data()));
	}

	void CommandBuffer::createFences()
	{
		VkFenceCreateInfo fenceCI{};
		fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (uint32_t i = 0; i < Device::s_maxFramesInFlight; i++)
			VK_CHECK_RESULT(vkCreateFence(VulkanContext::getDevice()->getVkDevice(), &fenceCI, nullptr, &m_inFlightFences[i]));
	}
}