#include "shpch.hpp"

#include "Shadow/Vulkan/VulkanCmdBuffer.hpp"
#include "Shadow/Vulkan/VulkanContext.hpp"
#include "Shadow/Vulkan/VulkanPipeline.hpp"
#include "Shadow/Vulkan/VulkanRenderpass.hpp"
#include "Shadow/Vulkan/VulkanBuffer.hpp"

#include "Shadow/ImGui/VkImGuiLayer.hpp"
#include "Shadow/Renderer/Mesh.hpp"

namespace Shadow
{
	VulkanCmdBuffer::VulkanCmdBuffer()
	{
		createCmdBufferPools();
		createCmdBuffers();
		createSyncObjects();

		for (uint32_t i = 0; i < VulkanDevice::s_maxFramesInFlight; i++)
		{
			m_graphics.waitSemaphores[i].reserve(3);
			m_graphics.waitSemaphores[i].emplace_back(m_graphics.imageAvailableSemaphores[i]);

			m_graphics.signalSemaphores[i].reserve(3);
			m_graphics.signalSemaphores[i].emplace_back(m_graphics.renderCompleteSemaphores[i]);
		}

		m_graphics.waitStages.reserve(3);
		m_graphics.waitStages.emplace_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

		m_graphics.signalStages.reserve(3);
		m_graphics.signalStages.emplace_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	}

	VulkanCmdBuffer::~VulkanCmdBuffer()
	{
		VkDevice device = VulkanContext::getVulkanDevice()->getVkDevice();

		for (uint32_t i = 0; i < VulkanDevice::s_maxFramesInFlight; i++)
		{
			vkDestroySemaphore(device, m_graphics.imageAvailableSemaphores[i], nullptr);
			vkDestroySemaphore(device, m_graphics.renderCompleteSemaphores[i], nullptr);
			vkDestroyFence(device, m_graphics.inFlightFences[i], nullptr);

			vkDestroySemaphore(device, m_transfer.semaphores[i], nullptr);
			vkDestroyFence(device, m_transfer.inFlightFences[i], nullptr);

			vkDestroySemaphore(device, m_compute.readySemaphores[i], nullptr);
			vkDestroySemaphore(device, m_compute.completeSemaphores[i], nullptr);
			vkDestroyFence(device, m_compute.inFlightFences[i], nullptr);
		}

		vkDestroyCommandPool(device, m_graphics.cmdPool, nullptr);
		vkDestroyCommandPool(device, m_transfer.cmdPool, nullptr);
		vkDestroyCommandPool(device, m_compute.cmdPool, nullptr);
	}

	void VulkanCmdBuffer::begin()
	{
		VulkanDevice* device = VulkanContext::getVulkanDevice();

		vkWaitForFences(device->getVkDevice(), 1, &m_graphics.inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
		vkResetFences(device->getVkDevice(), 1, &m_graphics.inFlightFences[m_currentFrame]);

		device->getSwapchain()->acquireNextImage(m_graphics.imageAvailableSemaphores[m_currentFrame]);
		vkResetCommandBuffer(m_graphics.cmdBuffers[m_currentFrame], 0);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		vkBeginCommandBuffer(m_graphics.cmdBuffers[m_currentFrame], &beginInfo);
	}

	void VulkanCmdBuffer::end()
	{
		vkEndCommandBuffer(m_graphics.cmdBuffers[m_currentFrame]);
	}

	void VulkanCmdBuffer::submit()
	{
		VulkanDevice* device = VulkanContext::getVulkanDevice();
		VkSubmitInfo2 submitInfos[2]{};

		// rendering submission
		VkCommandBufferSubmitInfo cmdSubmit{};
		cmdSubmit.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		cmdSubmit.commandBuffer = m_graphics.cmdBuffers[m_currentFrame];

		submitInfos[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		submitInfos[0].commandBufferInfoCount = 1;
		submitInfos[0].pCommandBufferInfos = &cmdSubmit;
		submitInfos[0].waitSemaphoreInfoCount = static_cast<uint32_t>(m_graphics.waitStages.size());
		submitInfos[0].signalSemaphoreInfoCount = static_cast<uint32_t>(m_graphics.signalSemaphores[m_currentFrame].size());

		VkSemaphoreSubmitInfo semaphoreSubmits[10] = {}; // 0-4 -> waitSemaphores; 5-9 -> signalSemaphores

		for (uint32_t i = 0; i < submitInfos[0].waitSemaphoreInfoCount; i++) // wait semaphore submit infos
		{
			semaphoreSubmits[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
			semaphoreSubmits[i].semaphore = m_graphics.waitSemaphores[m_currentFrame][i];
			semaphoreSubmits[i].stageMask = m_graphics.waitStages[i];
		}

		for (uint32_t i = 0; i < submitInfos[0].signalSemaphoreInfoCount; i++) // signal semaphore submit infos
		{
			uint32_t index = i + 5;
			semaphoreSubmits[index].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
			semaphoreSubmits[index].semaphore = m_graphics.signalSemaphores[m_currentFrame][i];
			semaphoreSubmits[index].stageMask = m_graphics.signalStages[i];
		}

		submitInfos[0].pWaitSemaphoreInfos = semaphoreSubmits;
		submitInfos[0].pSignalSemaphoreInfos = &semaphoreSubmits[5];


		// imgui rendering submission
		Ref<VulkanImGuiLayer> imguiLayer = as<VulkanImGuiLayer>(ShEngine::get().getImGuiLayer());

		VkCommandBufferSubmitInfo imguiCmdSubmit{};
		imguiCmdSubmit.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		imguiCmdSubmit.commandBuffer = imguiLayer->getCmdBuffer(m_currentFrame);

		VkSemaphoreSubmitInfo waitSemaphoreInfo{};
		waitSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		waitSemaphoreInfo.semaphore = m_graphics.renderCompleteSemaphores[m_currentFrame];
		waitSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSemaphoreSubmitInfo signalSemaphoreInfo{};
		signalSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		signalSemaphoreInfo.semaphore = imguiLayer->getRenderCompleteSemaphore(m_currentFrame);
		signalSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		submitInfos[1].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		submitInfos[1].commandBufferInfoCount = 1;
		submitInfos[1].pCommandBufferInfos = &imguiCmdSubmit;
		submitInfos[1].waitSemaphoreInfoCount = 1;
		submitInfos[1].pWaitSemaphoreInfos = &waitSemaphoreInfo;
		submitInfos[1].signalSemaphoreInfoCount = 1;
		submitInfos[1].pSignalSemaphoreInfos = &signalSemaphoreInfo;

		vkQueueSubmit2(device->getGraphicsQueue(), 2, submitInfos, m_graphics.inFlightFences[m_currentFrame]);
	}

	void VulkanCmdBuffer::setViewport(float x, float y, float width, float height)
	{
		VulkanDevice* device = VulkanContext::getVulkanDevice();

		VkViewport viewport{};
		viewport.x = x;
		viewport.y = y;
		viewport.width = width;
		viewport.height = height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(m_graphics.cmdBuffers[m_currentFrame], 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = device->getSwapchain()->getExtent();
		vkCmdSetScissor(m_graphics.cmdBuffers[m_currentFrame], 0, 1, &scissor);
	}

	void VulkanCmdBuffer::beginRenderPass(const Ref<GraphicsPipeline>& pipe, const void* pPushConstants)
	{
		VkCommandBuffer cmdBuffer = m_graphics.cmdBuffers[m_currentFrame];
		auto vkPipe = as<VulkanGraphicsPipeline>(pipe);

		auto& descriptorSets = vkPipe->getDescriptorSets();
		auto& pushConstants = vkPipe->getPushConstantRanges();
		auto& pipeConfig = vkPipe->getConfiguration();

		VkRenderPassBeginInfo beginInfo{};
		vkPipe->getVkRenderpass()->initBeginInfo(beginInfo);

		vkCmdBeginRenderPass(cmdBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipe->getVkPipeline());

		if (descriptorSets.size)
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipe->getLayout(), 0, 1, descriptorSets.data(), 0, nullptr);

		char* pRange = (char*)pPushConstants;
		for (size_t i = 0; i < pushConstants.size(); i++)
		{
			SH_ASSERT(pRange, "pPushConstnats must be an array of valid pointers");
			vkCmdPushConstants(cmdBuffer, vkPipe->getLayout(), pushConstants[i].stageFlags, pushConstants[i].offset, pushConstants[i].size, pRange);

			pRange += pushConstants[i].size;
		}
	}

	void VulkanCmdBuffer::endRenderPass()
	{
		vkCmdEndRenderPass(m_graphics.cmdBuffers[m_currentFrame]);
	}

	void VulkanCmdBuffer::nextSubpass(const Ref<GraphicsPipeline>& pipe, const void* pPushConstants)
	{
		VkCommandBuffer cmdBuffer = m_graphics.cmdBuffers[m_currentFrame];

		auto vkPipe = as<VulkanGraphicsPipeline>(pipe);
		auto& descriptorSets = vkPipe->getDescriptorSets();
		auto& pushConstants = vkPipe->getPushConstantRanges();

		vkCmdNextSubpass(cmdBuffer, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipe->getVkPipeline());

		if (descriptorSets.size)
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipe->getLayout(), 0, 1, descriptorSets.data(), 0, nullptr);

		char* pRange = (char*)pPushConstants;
		for (size_t i = 0; i < pushConstants.size(); i++)
		{
			SH_ASSERT(pRange, "pPushConstnats must be an array of valid pointers");
			vkCmdPushConstants(cmdBuffer, vkPipe->getLayout(), pushConstants[i].stageFlags, pushConstants[i].offset, pushConstants[i].size, pRange);

			pRange += pushConstants[i].size;
		}
	}

	void VulkanCmdBuffer::drawMesh(const Mesh& mesh)
	{
		VkCommandBuffer cmdBuffer = m_graphics.cmdBuffers[m_currentFrame];
		auto& meshIndexBuffer = mesh.getIndexBuffer();

		VkBuffer vb = as<VulkanVertexBuffer>(mesh.getVertexBuffer())->getVkBuffer();
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vb, &offset);

		VkBuffer ib = as<VulkanIndexBuffer>(mesh.getIndexBuffer())->getVkBuffer();
		vkCmdBindIndexBuffer(cmdBuffer, ib, offset, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(cmdBuffer, meshIndexBuffer->getCount(), 1, 0, 0, 0);
	}

	void VulkanCmdBuffer::draw(uint32_t verticesCount, uint32_t firstVertex)
	{
		vkCmdDraw(m_graphics.cmdBuffers[m_currentFrame], verticesCount, 1, firstVertex, 0);
	}

	void VulkanCmdBuffer::draw(const Ref<VertexBuffer>& vertexBuffer)
	{
		VkCommandBuffer cmdBuffer = m_graphics.cmdBuffers[m_currentFrame];

		VkBuffer buffer = as<VulkanVertexBuffer>(vertexBuffer)->getVkBuffer();
		VkDeviceSize offset = 0;

		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &buffer, &offset);
		vkCmdDraw(cmdBuffer, vertexBuffer->getVertexCount(), 1, 0, 0);
	}

	void VulkanCmdBuffer::draw(const Ref<StorageBuffer>& vertexBuffer)
	{
		VkCommandBuffer cmdBuffer = m_graphics.cmdBuffers[m_currentFrame];
		VkBuffer buffer = as<VulkanStorageBuffer>(vertexBuffer)->getVkBuffer();
		VkDeviceSize offset = 0;

		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &buffer, &offset);
		vkCmdDraw(cmdBuffer, vertexBuffer->getElementCount(), 1, 0, 0);
	}

	void VulkanCmdBuffer::drawIndexed(const Ref<VertexBuffer>& vertexBuffer, const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount)
	{
		uint32_t count = indexCount ? indexCount : indexBuffer->getCount();
		VkCommandBuffer cmdBuffer = m_graphics.cmdBuffers[m_currentFrame];

		VkBuffer vkVertexBuffer = as<VulkanVertexBuffer>(vertexBuffer)->getVkBuffer();
		VkBuffer vkIndexBuffer = as<VulkanIndexBuffer>(indexBuffer)->getVkBuffer();

		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vkVertexBuffer, &offset);
		vkCmdBindIndexBuffer(cmdBuffer, vkIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(cmdBuffer, count, 1, 0, 0, 0);
	}

	void VulkanCmdBuffer::drawInstanced(const Ref<VertexBuffer>& vertexBuffer, const Ref<VertexBuffer>& instanceBuffer, uint32_t instanceCount)
	{
		uint32_t count = instanceCount ? instanceCount : instanceBuffer->getVertexCount();
		VkCommandBuffer cmdBuffer = m_graphics.cmdBuffers[m_currentFrame];

		VkBuffer vertexBuffers[2] = {
			as<VulkanVertexBuffer>(vertexBuffer)->getVkBuffer(),
			as<VulkanVertexBuffer>(instanceBuffer)->getVkBuffer()
		};

		VkDeviceSize offsets[2] = { 0,0 };
		vkCmdBindVertexBuffers(cmdBuffer, 0, 2, vertexBuffers, offsets);
		vkCmdDraw(cmdBuffer, vertexBuffer->getVertexCount(), count, 0, 0);
	}

	void VulkanCmdBuffer::drawInstanced(const Ref<VertexBuffer>& vertexBuffer, const Ref<VertexBuffer>& instanceBuffer, const Ref<IndexBuffer>& indexBuffer, uint32_t instanceCount)
	{
		uint32_t count = instanceCount ? instanceCount : instanceBuffer->getVertexCount();
		VkCommandBuffer cmdBuffer = m_graphics.cmdBuffers[m_currentFrame];

		VkBuffer vertexBuffers[2] = {
			as<VulkanVertexBuffer>(vertexBuffer)->getVkBuffer(),
			as<VulkanVertexBuffer>(instanceBuffer)->getVkBuffer()
		};

		VkDeviceSize offsets[2] = { 0,0 };
		vkCmdBindVertexBuffers(cmdBuffer, 0, 2, vertexBuffers, offsets);

		VkBuffer ib = as<VulkanIndexBuffer>(indexBuffer)->getVkBuffer();
		vkCmdBindIndexBuffer(cmdBuffer, ib, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(cmdBuffer, indexBuffer->getCount(), count, 0, 0, 0);
	}

	void VulkanCmdBuffer::beginTransfer()
	{

	}

	void VulkanCmdBuffer::submitTransfer(PipelineStages graphicsWaitStage)
	{
		VulkanDevice* device = VulkanContext::getVulkanDevice();
		syncWithTransferQueue(graphicsWaitStage, PipelineStages::None);
	}

	void VulkanCmdBuffer::beginCompute(const Ref<ComputePipeline>& pipe, uint32_t descriptorSet, const void* pPushConstants)
	{
		VkCommandBuffer cmdBuffer = m_compute.cmdBuffers[m_currentFrame];

		VkCommandBufferBeginInfo cmdBegin{};
		cmdBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		vkBeginCommandBuffer(cmdBuffer, &cmdBegin);

		auto computePipe = as<VulkanComputePipeline>(pipe);
		auto& descriptorSets = computePipe->getDescriptorSets();
		auto& pushConstants = computePipe->getPushConstantRanges();

		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipe->getVkPipeline());

		if (descriptorSets.size)
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipe->getLayout(), descriptorSet, 1, descriptorSets.data() + descriptorSet, 0, nullptr);

		char* pRange = (char*)pPushConstants;
		for (size_t i = 0; i < pushConstants.size(); i++)
		{
			SH_ASSERT(pRange, "pPushConstnats must be an array of valid pointers");
			vkCmdPushConstants(cmdBuffer, computePipe->getLayout(), pushConstants[i].stageFlags, pushConstants[i].offset, pushConstants[i].size, pRange);

			pRange += pushConstants[i].size;
		}
	}

	void VulkanCmdBuffer::submitCompute(PipelineStages graphicsWaitStage)
	{
		VulkanDevice* device = VulkanContext::getVulkanDevice();
	    syncWithComputeQueue(graphicsWaitStage, PipelineStages::None); // TEMP

		VkCommandBuffer cmdBuffer = m_compute.cmdBuffers[m_currentFrame];
		vkEndCommandBuffer(cmdBuffer);

		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		VkSemaphore computeWaitSemaphore = m_compute.readySemaphores[m_currentFrame];
		VkSemaphore computeSignalSemaphore = m_compute.completeSemaphores[m_currentFrame];

		VkSubmitInfo computeSubmit{};
		computeSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		computeSubmit.commandBufferCount = 1;
		computeSubmit.pCommandBuffers = &cmdBuffer;
		computeSubmit.waitSemaphoreCount = 1;
		computeSubmit.pWaitSemaphores = &computeWaitSemaphore;
		computeSubmit.pWaitDstStageMask = &waitStage;
		computeSubmit.signalSemaphoreCount = 1;
		computeSubmit.pSignalSemaphores = &computeSignalSemaphore;
		vkQueueSubmit(device->getComputeQueue(), 1, &computeSubmit, VK_NULL_HANDLE);
	}

	void VulkanCmdBuffer::dispatch(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
	{
		vkCmdDispatch(m_compute.cmdBuffers[m_currentFrame], groupX, groupY, groupZ);
	}

	void VulkanCmdBuffer::acquireFromGraphicsQueue(const Ref<StorageBuffer>& buffer, PipelineStages dstStage, AccessFlags dstAccess)
	{
		VulkanDevice* device = VulkanContext::getVulkanDevice();
		VkCommandBuffer computeCmdBuffer = m_compute.cmdBuffers[m_currentFrame];

		if (device->hasDedicatedComputeQueue())
		{
			device->bufferMemoryBarrier(computeCmdBuffer, as<VulkanStorageBuffer>(buffer)->getVkBuffer(), buffer->getSize(),
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, static_cast<VkPipelineStageFlags>(dstStage),
				0, static_cast<VkAccessFlags>(dstAccess),
				device->getGraphicsQueueIndex(), device->getComputeQueueIndex());
		}
	}

	void VulkanCmdBuffer::releaseToGraphicsQueue(const Ref<StorageBuffer>& buffer, PipelineStages srcStage, AccessFlags srcAccess)
	{
		VulkanDevice* device = VulkanContext::getVulkanDevice();
		VkCommandBuffer computeCmdBuffer = m_compute.cmdBuffers[m_currentFrame];
		auto vkBuffer = as<VulkanStorageBuffer>(buffer);

		if (device->hasDedicatedComputeQueue())
		{
			device->bufferMemoryBarrier(computeCmdBuffer, vkBuffer->getVkBuffer(), vkBuffer->getSize(),
				static_cast<VkPipelineStageFlags>(srcStage), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				static_cast<VkAccessFlags>(srcAccess), 0,
				device->getComputeQueueIndex(), device->getGraphicsQueueIndex());
		}
	}

	void VulkanCmdBuffer::acquireFromComputeQueue(const Ref<StorageBuffer>& buffer, PipelineStages dstStage, AccessFlags dstAccess)
	{
		VulkanDevice* device = VulkanContext::getVulkanDevice();
		VkCommandBuffer graphicsCmdBuffer = m_graphics.cmdBuffers[m_currentFrame];

		if (device->hasDedicatedComputeQueue())
		{
			device->bufferMemoryBarrier(graphicsCmdBuffer, as<VulkanStorageBuffer>(buffer)->getVkBuffer(), buffer->getSize(),
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, static_cast<VkPipelineStageFlags>(dstStage),
				0, static_cast<VkAccessFlags>(dstAccess),
				device->getComputeQueueIndex(), device->getGraphicsQueueIndex());
		}
	}

	void VulkanCmdBuffer::releaseToComputeQueue(const Ref<StorageBuffer>& buffer, PipelineStages srcStage, AccessFlags srcAccess)
	{
		VulkanDevice* device = VulkanContext::getVulkanDevice();
		VkCommandBuffer graphicsCmdBuffer = m_graphics.cmdBuffers[m_currentFrame];

		if (device->hasDedicatedComputeQueue())
		{
			device->bufferMemoryBarrier(graphicsCmdBuffer, as<VulkanStorageBuffer>(buffer)->getVkBuffer(), buffer->getSize(),
				static_cast<VkPipelineStageFlags>(srcStage), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				static_cast<VkAccessFlags>(srcAccess), 0,
				device->getGraphicsQueueIndex(), device->getComputeQueueIndex());
		}
	}

	void VulkanCmdBuffer::queuePresent()
	{
		SH_PROFILE_FUNCTION();

		VulkanDevice* device = VulkanContext::getVulkanDevice();
		const VkSwapchainKHR vkSwapchain = device->getSwapchain()->getVkSwapchain();
		uint32_t imageIndex = device->getSwapchain()->getCurrentImageIndex();
		VkSemaphore waitSemaphore = as<VulkanImGuiLayer>(ShEngine::get().getImGuiLayer())->getRenderCompleteSemaphore(m_currentFrame);

		VkPresentInfoKHR present{};
		present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present.swapchainCount = 1;
		present.pSwapchains = &vkSwapchain;
		present.pImageIndices = &imageIndex;
		present.waitSemaphoreCount = 1;
		present.pWaitSemaphores = &waitSemaphore;
		vkQueuePresentKHR(device->getPresentQueue(), &present);

		m_currentFrame = (m_currentFrame + 1) % VulkanDevice::s_maxFramesInFlight;
	}

	void VulkanCmdBuffer::syncWithTransferQueue(PipelineStages waitStage, PipelineStages signalStage)
	{
		if (!m_graphics.syncWithTransferQueue)
		{
			m_graphics.syncWithTransferQueue = true;
			m_graphics.waitStages.emplace_back(static_cast<VkPipelineStageFlags>(waitStage));
			m_graphics.signalStages.emplace_back(static_cast<VkPipelineStageFlags>(signalStage));

			for (uint32_t i = 0; i < VulkanDevice::s_maxFramesInFlight; i++)
				m_graphics.waitSemaphores[i].emplace_back(m_transfer.semaphores[i]);
		}
	}

	void VulkanCmdBuffer::syncWithComputeQueue(PipelineStages waitStage, PipelineStages signalStage)
	{
		if (!m_graphics.syncWithComputeQueue)
		{
			m_graphics.syncWithComputeQueue = true;
			m_graphics.waitStages.emplace_back(static_cast<VkPipelineStageFlags>(waitStage));
			m_graphics.signalStages.emplace_back(static_cast<VkPipelineStageFlags>(signalStage));

			for (uint32_t i = 0; i < VulkanDevice::s_maxFramesInFlight; i++)
			{
				m_graphics.waitSemaphores[i].emplace_back(m_compute.completeSemaphores[i]);
				m_graphics.signalSemaphores[i].emplace_back(m_compute.readySemaphores[i]);
			}
		}
	}

	void VulkanCmdBuffer::addGraphicsQueueSubmitInfo(const VkSubmitInfo2& info)
	{
	}

	VkCommandBuffer VulkanCmdBuffer::beginSingleTimeCmdBuffer(uint32_t submitQueueIndex)
	{
		VulkanDevice* device = VulkanContext::getVulkanDevice();

		VkCommandBuffer singleSubmitCmdBuffer;
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandBufferCount = 1;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		if (submitQueueIndex == device->getTransferQueueIndex())
			allocInfo.commandPool = m_transfer.cmdPool;
		else if (submitQueueIndex == device->getComputeQueueIndex())
			allocInfo.commandPool = m_compute.cmdPool;
		else
			allocInfo.commandPool = m_graphics.cmdPool;

		vkAllocateCommandBuffers(device->getVkDevice(), &allocInfo, &singleSubmitCmdBuffer);

		VkCommandBufferBeginInfo beginSingleSubmitInfo{};
		beginSingleSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginSingleSubmitInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(singleSubmitCmdBuffer, &beginSingleSubmitInfo);

		return singleSubmitCmdBuffer;
	}

	void VulkanCmdBuffer::submitSingleTimeCmdBuffer(VkCommandBuffer cmdBuffer, uint32_t submitQueueIndex)
	{
		vkEndCommandBuffer(cmdBuffer);

		VulkanDevice* device = VulkanContext::getVulkanDevice();
		VkQueue submitQueue;
		VkCommandPool cmdPool;

		if (submitQueueIndex == device->getTransferQueueIndex())
		{
			submitQueue = device->getTransferQueue();
			cmdPool = m_transfer.cmdPool;
		}
		else if (submitQueueIndex == device->getComputeQueueIndex())
		{
			submitQueue = device->getComputeQueue();
			cmdPool = m_compute.cmdPool;
		}
		else
		{
			submitQueue = device->getGraphicsQueue();
			cmdPool = m_graphics.cmdPool;
		}

		VkSubmitInfo submit{};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &cmdBuffer;
		submit.waitSemaphoreCount = 0;
		submit.signalSemaphoreCount = 0;
		vkQueueSubmit(submitQueue, 1, &submit, VK_NULL_HANDLE);
		vkQueueWaitIdle(submitQueue);

		vkFreeCommandBuffers(device->getVkDevice(), cmdPool, 1, &cmdBuffer);
	}


	void VulkanCmdBuffer::createCmdBuffers()
	{
		VkDevice device = VulkanContext::getVulkanDevice()->getVkDevice();

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_graphics.cmdPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = VulkanDevice::s_maxFramesInFlight;

		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &allocInfo, m_graphics.cmdBuffers.data()));

		allocInfo.commandPool = m_transfer.cmdPool;
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &allocInfo, m_transfer.cmdBuffers.data()));

		allocInfo.commandPool = m_compute.cmdPool;
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &allocInfo, m_compute.cmdBuffers.data()))
	}

	void VulkanCmdBuffer::createCmdBufferPools()
	{
		VulkanDevice* device = VulkanContext::getVulkanDevice();

		VkCommandPoolCreateInfo cmdPoolInfo{};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		cmdPoolInfo.queueFamilyIndex = device->getGraphicsQueueIndex();
		VK_CHECK_RESULT(vkCreateCommandPool(device->getVkDevice(), &cmdPoolInfo, nullptr, &m_graphics.cmdPool));

		cmdPoolInfo.queueFamilyIndex = device->getTransferQueueIndex();
		VK_CHECK_RESULT(vkCreateCommandPool(device->getVkDevice(), &cmdPoolInfo, nullptr, &m_transfer.cmdPool));

		cmdPoolInfo.queueFamilyIndex = device->getComputeQueueIndex();
		VK_CHECK_RESULT(vkCreateCommandPool(device->getVkDevice(), &cmdPoolInfo, nullptr, &m_compute.cmdPool));
	}

	void VulkanCmdBuffer::createSyncObjects()
	{
		VulkanDevice* device = VulkanContext::getVulkanDevice();

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (uint32_t i = 0; i < VulkanDevice::s_maxFramesInFlight; i++)
		{
			VK_CHECK_RESULT(vkCreateSemaphore(device->getVkDevice(), &semaphoreInfo, nullptr, &m_graphics.imageAvailableSemaphores[i]));
			VK_CHECK_RESULT(vkCreateSemaphore(device->getVkDevice(), &semaphoreInfo, nullptr, &m_graphics.renderCompleteSemaphores[i]));
			VK_CHECK_RESULT(vkCreateFence(device->getVkDevice(), &fenceInfo, nullptr, &m_graphics.inFlightFences[i]));

			VK_CHECK_RESULT(vkCreateSemaphore(device->getVkDevice(), &semaphoreInfo, nullptr, &m_transfer.semaphores[i]));
			VK_CHECK_RESULT(vkCreateFence(device->getVkDevice(), &fenceInfo, nullptr, &m_transfer.inFlightFences[i]));

			VK_CHECK_RESULT(vkCreateSemaphore(device->getVkDevice(), &semaphoreInfo, nullptr, &m_compute.readySemaphores[i]))
			VK_CHECK_RESULT(vkCreateSemaphore(device->getVkDevice(), &semaphoreInfo, nullptr, &m_compute.completeSemaphores[i]));
			VK_CHECK_RESULT(vkCreateFence(device->getVkDevice(), &fenceInfo, nullptr, &m_compute.inFlightFences[i]));
		}

		// signal compute ready semaphore
		VkSubmitInfo submits[VulkanDevice::s_maxFramesInFlight]{};
		for (uint32_t i = 0; i < VulkanDevice::s_maxFramesInFlight; i++)
		{
			VkCommandBufferBeginInfo begin{};
			begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			vkBeginCommandBuffer(m_graphics.cmdBuffers[i], &begin);
			vkEndCommandBuffer(m_graphics.cmdBuffers[i]);

			submits[i].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submits[i].signalSemaphoreCount = 1;
			submits[i].pSignalSemaphores = &m_compute.readySemaphores[i];
			submits[i].commandBufferCount = 1;
			submits[i].pCommandBuffers = &m_graphics.cmdBuffers[i];
		}
		vkQueueSubmit(device->getGraphicsQueue(), VulkanDevice::s_maxFramesInFlight, submits, VK_NULL_HANDLE);
	}
}