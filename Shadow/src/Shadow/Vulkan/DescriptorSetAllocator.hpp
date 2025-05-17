#pragma once

#include <vulkan/vulkan.h>
#include <unordered_map>
#include <array>

namespace Shadow
{
	class UniformBuffer;
	struct DescriptorSetLayout;
	struct Resource;

	class DescriptorSetAllocator
	{
	public:
		DescriptorSetAllocator(const std::array<DescriptorSetLayout, 4>& layouts);
		~DescriptorSetAllocator();

		void allocateDescriptorSets(std::array<VkDescriptorSet, 4>& sets, const std::array<VkDescriptorSetLayout, 4>& layouts);

		inline const VkDescriptorPool const getDescriptorPool() const { return m_descriptorPool; }
	private:
		VkDescriptorPool m_descriptorPool;
	};
}