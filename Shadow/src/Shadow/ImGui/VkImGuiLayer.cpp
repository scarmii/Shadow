#include "shpch.hpp"
#include "Shadow/Core/ShEngine.hpp"
#include "Shadow/Core/Core.hpp"

#include "Shadow/ImGui/VkImGuiLayer.hpp"
#include "Shadow/Renderer/Renderer.hpp"
#include "Shadow/Vulkan/VulkanContext.hpp"
#include "Shadow/Vulkan/VulkanCmdBuffer.hpp"

#include <imgui/backends/imgui_impl_glfw.h>

namespace Shadow
{
	VulkanImGuiLayer::VulkanImGuiLayer()
	{
		SH_PROFILE_FUNCTION();

		EventDispatcher::get().addReciever(SH_CALLBACK(VulkanImGuiLayer::onWindowResized));

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; //Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		setupImGuiStyle();

		GLFWwindow* window = static_cast<GLFWwindow*>(ShEngine::get().getWindow().getWindowHandle());
		VulkanDevice* device = VulkanContext::getVulkanDevice();
		ImGui_ImplGlfw_InitForVulkan(window, true);

		m_imguiFramebuffers.resize(device->getSwapchain()->getImageCount());

		createImGuiDescriptorPool();
		createImGuiSyncObjects();
		createImGuiCmdBuffers();
		createImGuiRenderpass();
		createImGuiFramebuffers();

		ImGui_ImplVulkan_InitInfo initInfo{};
		initInfo.Instance = VulkanContext::getVkInstance();
		initInfo.PhysicalDevice = device->getPhysicalDevice();
		initInfo.Device = device->getVkDevice();
		initInfo.QueueFamily = device->getGraphicsQueueIndex();
		initInfo.Queue = device->getGraphicsQueue();
		initInfo.DescriptorPool = m_imGuiDescriptorPool;
		initInfo.MinImageCount = 2;
		initInfo.ImageCount = device->getSwapchain()->getImageCount();
		initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		initInfo.RenderPass = m_imguiRenderpass;

		ImGui_ImplVulkan_Init(&initInfo);
		ImGui_ImplVulkan_CreateFontsTexture();
	}

	VulkanImGuiLayer::~VulkanImGuiLayer()
	{
		SH_PROFILE_FUNCTION();

		VkDevice vkDevice = VulkanContext::getVulkanDevice()->getVkDevice();
		vkDeviceWaitIdle(vkDevice);

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		vkDestroyDescriptorPool(vkDevice, m_imGuiDescriptorPool, nullptr);
		vkDestroyCommandPool(vkDevice, m_imguiCmdPool, nullptr);

		for (size_t i = 0; i < VulkanDevice::s_maxFramesInFlight; i++)
		{
			vkDestroyFence(vkDevice, m_inFlightFences[i], nullptr);
			vkDestroySemaphore(vkDevice, m_uiRenderCompleteSemaphores[i], nullptr);
		}

		for (size_t i = 0; i < m_imguiFramebuffers.size(); i++)
			vkDestroyFramebuffer(vkDevice, m_imguiFramebuffers[i], nullptr);

		vkDestroyRenderPass(vkDevice, m_imguiRenderpass, nullptr);
	}

	void VulkanImGuiLayer::begin()
	{
		SH_PROFILE_FUNCTION();

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void VulkanImGuiLayer::submit()
	{
		SH_PROFILE_FUNCTION();
		{
			SH_PROFILE_SCOPE("ImGui::Render - VkImGuiLayer::submit");
			ImGui::Render();
		}

		VulkanDevice* vulkanDevice = VulkanContext::getVulkanDevice();
		Ref<VulkanCmdBuffer> renderCmdBuffer = as<VulkanCmdBuffer>(Renderer::getCmdBuffer());
		VkFence fence = renderCmdBuffer->getInFlightFence();
		uint32_t currentFrame = renderCmdBuffer->currentFrame();

		vkResetCommandBuffer(m_imguiCmdBuffers[currentFrame], 0);

		VkCommandBufferBeginInfo cmdBeginInfo{};
		cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		vkBeginCommandBuffer(m_imguiCmdBuffers[currentFrame], &cmdBeginInfo);

		{
			SH_PROFILE_SCOPE("ImGui_ImplVulkan_RenderDrawData - VkImGuiLayer::submit");

			VkRenderPassBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			beginInfo.clearValueCount = 0;
			beginInfo.renderPass = m_imguiRenderpass;
			beginInfo.framebuffer = m_imguiFramebuffers[vulkanDevice->getSwapchain()->getCurrentImageIndex()];
			beginInfo.renderArea.offset = { 0, 0 };
			beginInfo.renderArea.extent = vulkanDevice->getSwapchain()->getExtent();
			vkCmdBeginRenderPass(m_imguiCmdBuffers[currentFrame], &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_imguiCmdBuffers[currentFrame]);
		}
	
		vkCmdEndRenderPass(m_imguiCmdBuffers[currentFrame]);
		vkEndCommandBuffer(m_imguiCmdBuffers[currentFrame]);
	}

	void VulkanImGuiLayer::updateWindows()
	{
		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}

	bool VulkanImGuiLayer::onWindowResized(const WindowResizedEvent& e)
	{
		for (size_t i = 0; i < m_imguiFramebuffers.size(); i++)
			vkDestroyFramebuffer(VulkanContext::getVulkanDevice()->getVkDevice(), m_imguiFramebuffers[i], nullptr);

		createImGuiFramebuffers();

		return false;
	}

	void VulkanImGuiLayer::createImGuiRenderpass()
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.flags = VK_DEPENDENCY_BY_REGION_BIT;
		colorAttachment.format = VulkanContext::getVulkanDevice()->getSwapchain()->getImageFormat();
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

		VkAttachmentReference colorRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.flags = 0;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorRef;
		subpass.pDepthStencilAttachment = nullptr;
		subpass.inputAttachmentCount = 0;

		VkSubpassDependency dependencies[2]{};
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = 0;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = &colorAttachment;
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subpass;
		createInfo.dependencyCount = 2;
		createInfo.pDependencies = &dependencies[0];
		VK_CHECK_RESULT(vkCreateRenderPass(VulkanContext::getVulkanDevice()->getVkDevice(), &createInfo, nullptr, &m_imguiRenderpass));
	}

	void VulkanImGuiLayer::createImGuiFramebuffers()
	{
		VulkanDevice* device = VulkanContext::getVulkanDevice();

		uint32_t width = 0, height = 0;
		ShEngine::get().getWindow().getFramebufferSize(width, height);

		for (size_t i = 0; i < m_imguiFramebuffers.size(); i++)
		{
			VkFramebufferCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			createInfo.attachmentCount = 1;
			createInfo.pAttachments = &device->getSwapchain()->getImageViews()[i];
			createInfo.renderPass = m_imguiRenderpass;
			createInfo.width = width;
			createInfo.height = height;
			createInfo.layers = 1;
			VK_CHECK_RESULT(vkCreateFramebuffer(device->getVkDevice(), &createInfo, nullptr, &m_imguiFramebuffers[i]));
		}
	}

	void VulkanImGuiLayer::createImGuiDescriptorPool()
	{
		VkDescriptorPoolSize poolSizes[] = {
			{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
			{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
			{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
			{VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
			{VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
			{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
		    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
			{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
			{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000},
		};

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.maxSets = 4;
		poolInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
		poolInfo.pPoolSizes = poolSizes;
		VK_CHECK_RESULT(vkCreateDescriptorPool(VulkanContext::getVulkanDevice()->getVkDevice(),
			&poolInfo, nullptr, &m_imGuiDescriptorPool));
	}

	void VulkanImGuiLayer::createImGuiCmdBuffers()
	{
		VulkanDevice* device = VulkanContext::getVulkanDevice();

		VkCommandPoolCreateInfo cmdPoolInfo{};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = device->getGraphicsQueueIndex();
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VK_CHECK_RESULT(vkCreateCommandPool(device->getVkDevice(), &cmdPoolInfo, nullptr, &m_imguiCmdPool));

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_imguiCmdPool;
		allocInfo.commandBufferCount = m_imguiCmdBuffers.size();
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device->getVkDevice(), &allocInfo, m_imguiCmdBuffers.data()));
	}

	void VulkanImGuiLayer::createImGuiSyncObjects()
	{
		VkDevice vkDevice = VulkanContext::getVulkanDevice()->getVkDevice();

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < m_inFlightFences.size(); i++)
		{
			VK_CHECK_RESULT(vkCreateFence(vkDevice, &fenceInfo, nullptr, &m_inFlightFences[i]));
			VK_CHECK_RESULT(vkCreateSemaphore(vkDevice, &semaphoreInfo, nullptr, &m_uiRenderCompleteSemaphores[i]));
		}
	}
};