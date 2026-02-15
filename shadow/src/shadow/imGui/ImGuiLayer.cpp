#include "shpch.h"
#include "shadow/core/shApp.h"
#include "shadow/core/core.h"

#include "shadow/imGui/imguiLayer.h"
#include "shadow/renderer/renderer.h"
#include "shadow/vulkan/context.h"

#include <imgui/backends/imgui_impl_glfw.h>

#define USE_IMGUI_CMD_BUFFER

namespace Shadow
{
	ImGuiLayer::ImGuiLayer()
	{
		SH_PROFILE_FUNCTION();

		EventDispatcher& dispatcher = EventDispatcher::get();
		dispatcher.addReciever(SH_ON_EVENT_FN(onWindowResized, WindowResizedEvent));
		dispatcher.addReciever(SH_ON_EVENT_FN(onMouseScrolledEvent, MouseScrolledEvent));
		dispatcher.addReciever(SH_ON_EVENT_FN(onMouseMovedEvent, MouseMovedEvent));

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; //Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		setupImGuiStyle();

		GLFWwindow* window = static_cast<GLFWwindow*>(ShApp::get().getWindow().getWindowHandle());
		Device* device = VulkanContext::getDevice();
		ImGui_ImplGlfw_InitForVulkan(window, true);

		m_imGuiFramebuffers.resize(device->getSwapchain()->getImageCount());

		m_imGuiRenderCompleteSemaphore = createScope<Semaphore>();
		m_imGuiCmdBuffer = createScope<CommandBuffer>(QueueType::Graphics);
		m_imGuiCmdBuffer->addWaitSemaphore(device->getRenderCompleteSem(), PipelineStages::ColorAttachmentOutput);
		m_imGuiCmdBuffer->addSignalSemaphore(m_imGuiRenderCompleteSemaphore, PipelineStages::ColorAttachmentOutput);

		createImGuiDescriptorPool();
		createImGuiRenderPass();
		createImGuiFramebuffers();

		std::string shadersPath = "C:/dev/shadow/shadow/assets/shaders/";
		m_imGuiShader = Shader::create("imgui_shader", shadersPath + "offscreen.vert.spv", shadersPath + "offscreen.frag.spv");

		VertexInput squareVertexDesc({ VertexAttribType::Vec2f, VertexAttribType::Vec2f });
		GraphicsPipeConfiguration pipelineConfig{};
		pipelineConfig.vertexInput = &squareVertexDesc;
		m_imGuiPipeline = GraphicsPipeline::create(pipelineConfig);

		m_imGuiPipeline->setName("imgui_pipeline");;
		m_imGuiPipeline->setSubpass(0, m_imGuiRenderPass);
		m_imGuiPipeline->build(m_imGuiShader.get());

		// init ImGui vulkan backend
		{
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
			initInfo.RenderPass = m_imGuiRenderPass;

			ImGui_ImplVulkan_Init(&initInfo);
			ImGui_ImplVulkan_CreateFontsTexture();
		}

		float vertices[] = {
			-1.0f,-1.0f,  0.0f,0.0f,
			-1.0f, 1.0f,  0.0f,1.0f,
			 1.0f, 1.0f,  1.0f,1.0f,
			 1.0f,-1.0f,  1.0f,0.0f
		};

		uint32_t indices[6] = { 0,1,2,2,3,0 };

		m_imGuiBuffers.vertexBuffer = VertexBuffer::create(vertices, sizeof(vertices), sizeof(float) * 4);
		m_imGuiBuffers.indexBuffer = IndexBuffer::create(indices, 6);
	}

	ImGuiLayer::~ImGuiLayer()
	{
		SH_PROFILE_FUNCTION();

		VkDevice vkDevice = VulkanContext::getDevice()->getVkDevice();
		vkDeviceWaitIdle(vkDevice);

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		for (size_t i = 0; i < m_imGuiFramebuffers.size(); i++)
			vkDestroyFramebuffer(vkDevice, m_imGuiFramebuffers[i], nullptr);

		vkDestroyDescriptorPool(vkDevice, m_imGuiDescriptorPool, nullptr);
		vkDestroyRenderPass(vkDevice, m_imGuiRenderPass, nullptr);
	}

	void ImGuiLayer::begin()
	{
		SH_PROFILE_FUNCTION();

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

#ifdef USE_IMGUI_CMD_BUFFER
		m_imGuiCmdBuffer->begin();
#endif
	}

	void ImGuiLayer::end()
	{
		SH_PROFILE_FUNCTION();

		ImGui::Render();

		Device* device = VulkanContext::getDevice();
		Swapchain* swapchain = device->getSwapchain();
		uint32_t currentFrame = device->currentFrame();

#ifdef USE_IMGUI_CMD_BUFFER
		VkCommandBuffer cmdBuffer = m_imGuiCmdBuffer->getVkCommandBuffer();
#else
		VkCommandBuffer cmdBuffer = device->getCmdBuffer()->getVkCommandBuffer();
#endif

		VkClearValue clearColor{ 0.025f,0.025f,0.025f,1.0f };
		VkExtent2D swapchainExtent = swapchain->getExtent();
		VkRenderPassBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		beginInfo.clearValueCount = 1;
		beginInfo.pClearValues = &clearColor;
		beginInfo.renderPass = m_imGuiRenderPass;
		beginInfo.framebuffer = m_imGuiFramebuffers[swapchain->getCurrentImageIndex()];
		beginInfo.renderArea.offset = { 0, 0 };
		beginInfo.renderArea.extent = swapchainExtent;
		vkCmdBeginRenderPass(cmdBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
		vkCmdEndRenderPass(cmdBuffer);

#ifdef USE_IMGUI_CMD_BUFFER
		m_imGuiCmdBuffer->end();
		m_imGuiCmdBuffer->submit();
#endif
	}

	void ImGuiLayer::updateWindows()
	{
		SH_PROFILE_FUNCTION();

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}

	ImTextureID ImGuiLayer::addTexture(const Ref<Texture2D>& texture)
	{
		return (ImTextureID)ImGui_ImplVulkan_AddTexture(texture->getSampler(), texture->getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	void ImGuiLayer::removeTexture(ImTextureID id)
	{
		ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)id);
	}

	bool ImGuiLayer::onWindowResized(const WindowResizedEvent& e)
	{
		VulkanContext::getDevice()->waitIdle();

		for (size_t i = 0; i < m_imGuiFramebuffers.size(); i++)
			vkDestroyFramebuffer(VulkanContext::getDevice()->getVkDevice(), m_imGuiFramebuffers[i], nullptr);

		createImGuiFramebuffers();
		return false;
	}

	bool ImGuiLayer::onMouseScrolledEvent(const MouseScrolledEvent& e)
	{
		if (m_blockEvents)
			return true;
		return false;
	}

	bool ImGuiLayer::onMouseMovedEvent(const MouseMovedEvent& e)
	{
		if (m_blockEvents)
			return true;
		return false;
	}

	void ImGuiLayer::createImGuiFramebuffers()
	{
		Device* device = VulkanContext::getDevice();

		uint32_t width = 0, height = 0;
		ShApp::get().getWindow().getFramebufferSize(width, height);

		for (size_t i = 0; i < m_imGuiFramebuffers.size(); i++)
		{
			VkFramebufferCreateInfo framebufferCI{};
			framebufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferCI.attachmentCount = 1;
			framebufferCI.pAttachments = &device->getSwapchain()->getImageViews()[i];
			framebufferCI.renderPass = m_imGuiRenderPass;
			framebufferCI.width = width;
			framebufferCI.height = height;
			framebufferCI.layers = 1;
			VK_CHECK_RESULT(vkCreateFramebuffer(device->getVkDevice(), &framebufferCI, nullptr, &m_imGuiFramebuffers[i]));
		}
	}

	void ImGuiLayer::createImGuiRenderPass(PipelineStages srcStageMask, AccessFlags srcAccess, VkRenderPass* renderPass)
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.flags = VK_DEPENDENCY_BY_REGION_BIT;
		colorAttachment.format = VulkanContext::getDevice()->getSwapchain()->getImageFormat();
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
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
		dependencies[0].srcStageMask = static_cast<VkPipelineStageFlags>(srcStageMask);
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = static_cast<VkAccessFlagBits>(srcAccess);
		dependencies[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = &colorAttachment;
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subpass;
		createInfo.dependencyCount = 2;
		createInfo.pDependencies = &dependencies[0];
		VK_CHECK_RESULT(vkCreateRenderPass(VulkanContext::getDevice()->getVkDevice(), &createInfo, nullptr, renderPass));
	}

	void ImGuiLayer::createImGuiRenderPass()
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.flags = VK_DEPENDENCY_BY_REGION_BIT;
		colorAttachment.format = VulkanContext::getDevice()->getSwapchain()->getImageFormat();
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
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
		VK_CHECK_RESULT(vkCreateRenderPass(VulkanContext::getDevice()->getVkDevice(), &createInfo, nullptr, &m_imGuiRenderPass));
	}

	void ImGuiLayer::createImGuiDescriptorPool()
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

		VkDescriptorPoolCreateInfo descriptorPoolCI{};
		descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolCI.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		descriptorPoolCI.maxSets = 4;
		descriptorPoolCI.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
		descriptorPoolCI.pPoolSizes = poolSizes;
		VK_CHECK_RESULT(vkCreateDescriptorPool(VulkanContext::getDevice()->getVkDevice(), &descriptorPoolCI, nullptr, &m_imGuiDescriptorPool));
	}


	void ImGuiLayer::setupImGuiStyle()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		//ImGui::StyleColorsDark(&style);

		style.Colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
		style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		style.Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
		style.Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
		style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
		style.Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style.Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		style.Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style.Colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		style.Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
		style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
		style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		style.Colors[ImGuiCol_TabHovered] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		style.Colors[ImGuiCol_Tab] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		style.Colors[ImGuiCol_TabSelected] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
		style.Colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		style.Colors[ImGuiCol_TabDimmed] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		style.Colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		style.Colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);


		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			//style.Colors[ImGuiCol_WindowBg] = ImVec4(0.025f, 0.025f, 0.025f,1.0f);
			//style.Colors[ImGuiCol_WindowBg] = ImVec4(0.025f, 0.025f, 0.025f,1.0f);
		}
	}
};