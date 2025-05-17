#include "shpch.hpp"
#include "Shadow/Core/Core.hpp"

#include "Shadow/Vulkan/DescriptorSetAllocator.hpp"
#include "Shadow/Vulkan/VulkanContext.hpp"
#include "Shadow/Vulkan/VulkanUniformBuffer.hpp"
#include "Shadow/Vulkan/VulkanShader.hpp"

namespace Shadow
{
	DescriptorSetAllocator::DescriptorSetAllocator(const std::array<DescriptorSetLayout, 4>& layouts)
	{
		//std::unordered_map<VkDescriptorType, uint32_t> descriptorTypeCounts;

		//for (uint32_t i = 0; i < layouts.size(); i++)
		//{
		//	for(const VkDescriptorSetLayoutBinding& binding: layouts[i].bindings)
		//		descriptorTypeCounts[binding.descriptorType]++;
		//}

		//std::vector<VkDescriptorPoolSize> poolSizes;
		//for (auto& descriptorType : descriptorTypeCounts)
		//{
		//	VkDescriptorPoolSize poolSize{};
		//	poolSize.type = descriptorType.first;
		//	poolSize.descriptorCount = descriptorType.second * VulkanDevice::s_maxFramesInFlight;
		//	poolSizes.emplace_back(poolSize);
		//}

		VkDescriptorPoolSize poolSizes[] = {
			{VK_DESCRIPTOR_TYPE_SAMPLER, 32},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 32},
			{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 32},
			{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10},
			{VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 10},
			{VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 10},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10},
			{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10},
			{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 10},
			{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 10},
		};

		VkDescriptorPoolCreateInfo poolinfo{};
		poolinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolinfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
		poolinfo.pPoolSizes = poolSizes;
		poolinfo.maxSets = static_cast<uint32_t>(layouts.size());
		poolinfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
		VK_CHECK_RESULT(vkCreateDescriptorPool(VulkanContext::getVulkanDevice()->getVkDevice(), &poolinfo, nullptr, &m_descriptorPool));
	}

	DescriptorSetAllocator::~DescriptorSetAllocator()
	{
		vkDeviceWaitIdle(VulkanContext::getVulkanDevice()->getVkDevice());
		vkDestroyDescriptorPool(VulkanContext::getVulkanDevice()->getVkDevice(), m_descriptorPool, nullptr);
	}

	void DescriptorSetAllocator::allocateDescriptorSets(std::array<VkDescriptorSet, 4>& sets, const std::array<VkDescriptorSetLayout, 4>& layouts)
	{
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorSetCount = (uint32_t)layouts.size();
		allocInfo.pSetLayouts = layouts.data();
		allocInfo.descriptorPool = m_descriptorPool;
		VK_CHECK_RESULT(vkAllocateDescriptorSets(VulkanContext::getVulkanDevice()->getVkDevice(), &allocInfo, sets.data()));
	}
}