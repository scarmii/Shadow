#include "shpch.hpp"
#include "Shadow/Core/Core.hpp"

#include "Shadow/Renderer/Mesh.hpp"
#include "Shadow/Renderer/Renderer.hpp"

#include "Shadow/Vulkan/VulkanShader.hpp"
#include "Shadow/Vulkan/VulkanContext.hpp"
#include "Shadow/Vulkan/VulkanUniformBuffer.hpp"
#include "Shadow/Vulkan/VulkanCmdBuffer.hpp"

#include <spirv_cross.hpp>
#include <spirv_reflect.hpp>

namespace Shadow
{
	VulkanShader::VulkanShader(const std::string& name, const std::string vertPath, const std::string& fragPath)
		: m_name(name)
	{
		SH_PROFILE_FUNCTION();

		m_usedDescriptorSets.reserve(m_descriptorSets.size());
		m_vertexShaderCode.reserve(100);
		m_fragmentShaderCode.reserve(100);

		m_vertexShaderCode = readFile(vertPath);
		m_fragmentShaderCode = readFile(fragPath);

		m_vertexShaderModule = createShaderModule(m_vertexShaderCode);
		m_fragmentShaderModule = createShaderModule(m_fragmentShaderCode);

		if (m_vertexShaderModule != VK_NULL_HANDLE)
			m_stages |= ShaderStage::Vertex;
		if (m_fragmentShaderModule != VK_NULL_HANDLE)
			m_stages |= ShaderStage::Fragment;

		retrieveShaderResources();
		createDescriptorSetAllocator();
	}

	VulkanShader::VulkanShader(const std::string& name, const std::string computeSpv)
		: m_name(name)
	{
		SH_PROFILE_FUNCTION();

		m_computeShaderCode.reserve(100);

		m_computeShaderCode = readFile(computeSpv);
		m_computeShaderModule = createShaderModule(m_computeShaderCode);

		if (m_computeShaderModule != VK_NULL_HANDLE)
			m_stages |= ShaderStage::Compute;

		retrieveShaderResources();
		createDescriptorSetAllocator();
	}

	VulkanShader::VulkanShader(const std::string& name, const std::string vertPath, const std::string& fragPath, const std::string computeSpv)
		: m_name(name)
	{
		SH_PROFILE_FUNCTION();

		m_usedDescriptorSets.reserve(m_descriptorSets.size());
		m_vertexShaderCode.reserve(100);
		m_fragmentShaderCode.reserve(100);
		m_computeShaderCode.reserve(100);

		m_vertexShaderCode = readFile(vertPath);
		m_fragmentShaderCode = readFile(fragPath);
		m_computeShaderCode = readFile(computeSpv);

		m_vertexShaderModule = createShaderModule(m_vertexShaderCode);
		m_fragmentShaderModule = createShaderModule(m_fragmentShaderCode);
		m_computeShaderModule = createShaderModule(m_computeShaderCode);

		if (m_vertexShaderModule != VK_NULL_HANDLE)
			m_stages |= ShaderStage::Vertex;
		if (m_fragmentShaderModule != VK_NULL_HANDLE)
			m_stages |= ShaderStage::Fragment;
		if (m_computeShaderModule != VK_NULL_HANDLE)
			m_stages |= ShaderStage::Compute;

		retrieveShaderResources();
		createDescriptorSetAllocator();
	}

	VulkanShader::~VulkanShader()
	{
		VkDevice device = VulkanContext::getVulkanDevice()->getVkDevice();

		for (uint32_t i = 0; i < m_resources->descriptorSetLayouts.size(); i++)
			vkDestroyDescriptorSetLayout(device, m_resources->descriptorSetLayouts[i].layout, nullptr);

		if (m_vertexShaderModule != VK_NULL_HANDLE)
			vkDestroyShaderModule(device, m_vertexShaderModule, nullptr);
		if (m_fragmentShaderModule != VK_NULL_HANDLE)
			vkDestroyShaderModule(device, m_fragmentShaderModule, nullptr);
		if (m_computeShaderModule != VK_NULL_HANDLE)
			vkDestroyShaderModule(device, m_computeShaderModule, nullptr);
	}

	void VulkanShader::writeDescriptorSet(const std::string& shaderName, const Ref<UniformBuffer>& buffer)
	{
		VkDevice device = VulkanContext::getVulkanDevice()->getVkDevice();
		auto vkBuffer = as<VulkanUniformBuffer>(buffer);
		const Resource& resource = m_resources->resources[shaderName];

		VkWriteDescriptorSet writers[VulkanDevice::s_maxFramesInFlight]{};
		VkDescriptorBufferInfo bufferInfos[VulkanDevice::s_maxFramesInFlight]{};

		for (uint32_t i = 0; i < VulkanDevice::s_maxFramesInFlight; i++)
		{
			bufferInfos[i].buffer = vkBuffer->getVkBuffer(i);
			bufferInfos[i].offset = 0;
			bufferInfos[i].range = vkBuffer->getSize();

			writers[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writers[i].dstSet = m_descriptorSets[resource.set];
			writers[i].dstBinding = resource.binding;
			writers[i].dstArrayElement = 0;
			writers[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writers[i].descriptorCount = 1;
			writers[i].pBufferInfo = &bufferInfos[i];
		}
		vkUpdateDescriptorSets(device, VulkanDevice::s_maxFramesInFlight, writers, 0, nullptr);
	}

	void VulkanShader::writeDescriptorSet(const std::string& shaderName, const Ref<StorageBuffer>& buffer, bool acquireFromGraphicsQueue)
	{
		VulkanDevice* vulkanDevice = VulkanContext::getVulkanDevice();
		const Resource& resource = m_resources->resources[shaderName];
		auto vkBuffer = as<VulkanStorageBuffer>(buffer); 

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = vkBuffer->getVkBuffer();
		bufferInfo.offset = 0;
		bufferInfo.range = vkBuffer->getSize();

		VkWriteDescriptorSet writer{};
		writer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writer.dstSet = m_descriptorSets[resource.set];
		writer.dstBinding = resource.binding;
		writer.dstArrayElement = 0;
		writer.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writer.descriptorCount = 1;
		writer.pBufferInfo = &bufferInfo;
		vkUpdateDescriptorSets(vulkanDevice->getVkDevice(), 1, &writer, 0, nullptr);

		// TEMP (acquire the buffer from the graphics queue)
		if (acquireFromGraphicsQueue && vulkanDevice->hasDedicatedComputeQueue() && m_stages & ShaderStage::Compute)
		{
			Ref<VulkanCmdBuffer> renderCmdBuffer = as<VulkanCmdBuffer>(Renderer::getCmdBuffer());
			VkCommandBuffer vkCmdBuffer;

			VkCommandBufferAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.commandPool = renderCmdBuffer->getComputeCmdPool();
			allocInfo.commandBufferCount = 1;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			vkAllocateCommandBuffers(vulkanDevice->getVkDevice(), &allocInfo, &vkCmdBuffer);

			VkCommandBufferBeginInfo beginSingleSubmitInfo{};
			beginSingleSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginSingleSubmitInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			vkBeginCommandBuffer(vkCmdBuffer, &beginSingleSubmitInfo);

			VkBufferMemoryBarrier acquireBarrier{};
			acquireBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			acquireBarrier.buffer = vkBuffer->getVkBuffer();
			acquireBarrier.size = vkBuffer->getSize();
			acquireBarrier.offset = 0;
			acquireBarrier.srcAccessMask = 0;
			acquireBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			acquireBarrier.srcQueueFamilyIndex = vulkanDevice->getGraphicsQueueIndex();
			acquireBarrier.dstQueueFamilyIndex = vulkanDevice->getComputeQueueIndex();
			vkCmdPipelineBarrier(vkCmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1, &acquireBarrier, 0, nullptr);
			vkEndCommandBuffer(vkCmdBuffer);

			VkSubmitInfo submit{};
			submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submit.commandBufferCount = 1;
			submit.pCommandBuffers = &vkCmdBuffer;
			submit.waitSemaphoreCount = 0;
			submit.signalSemaphoreCount = 0;
			vkQueueSubmit(vulkanDevice->getComputeQueue(), 1, &submit, VK_NULL_HANDLE);
			vkQueueWaitIdle(vulkanDevice->getComputeQueue());

			vkFreeCommandBuffers(vulkanDevice->getVkDevice(), renderCmdBuffer->getComputeCmdPool(), 1, &vkCmdBuffer);
		}
	}

	void VulkanShader::writeDescriptorSet(const std::string& name, const Ref<Texture2D>& texture)
	{
		SH_PROFILE_FUNCTION();

		auto vkTexture = as<VulkanTexture2D>(texture);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = vkTexture->getImage().imageView;
		imageInfo.sampler = vkTexture->getSampler();

		auto& samplerRes = m_resources->resources[name];
		VkWriteDescriptorSet descriptorWriter{};
		descriptorWriter.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWriter.dstSet = m_descriptorSets[samplerRes.set];
		descriptorWriter.dstBinding = samplerRes.binding;
		descriptorWriter.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWriter.descriptorCount = 1;
		descriptorWriter.pImageInfo = &imageInfo;
		vkUpdateDescriptorSets(VulkanContext::getVulkanDevice()->getVkDevice(), 1, &descriptorWriter, 0, nullptr);
	}

	void VulkanShader::writeDescriptorSet(const std::string& name, const Texture2D& texture)
	{
		SH_PROFILE_FUNCTION();

		const VulkanTexture2D& vkTexture = (const VulkanTexture2D&)texture;

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = vkTexture.getImage().imageView;
		imageInfo.sampler = vkTexture.getSampler();
		
		auto& samplerRes = m_resources->resources[name];
		VkWriteDescriptorSet descriptorWriter{};
		descriptorWriter.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWriter.dstSet = m_descriptorSets[samplerRes.set];
		descriptorWriter.dstBinding = samplerRes.binding;
		descriptorWriter.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWriter.descriptorCount = 1;
		descriptorWriter.pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(VulkanContext::getVulkanDevice()->getVkDevice(), 1, &descriptorWriter, 0, nullptr);
		vkDeviceWaitIdle(VulkanContext::getVulkanDevice()->getVkDevice());
	}

	void VulkanShader::writeDescriptorSet(const std::string& name, uint32_t count, const Ref<Texture2D>* pTextures, uint32_t dstArrIndex)
	{
		SH_PROFILE_FUNCTION();

		auto& samplerRes = m_resources->resources[name];
		VkDescriptorImageInfo imageInfos[32]{};

		for (uint32_t i = 0; i < count; i++)
		{
			const Ref<VulkanTexture2D>& vkTexture = (const Ref<VulkanTexture2D>&)pTextures[i];
			imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfos[i].imageView = vkTexture->getImage().imageView;
			imageInfos[i].sampler = vkTexture->getSampler();
		}

		VkWriteDescriptorSet descriptorWriter{};
		descriptorWriter.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWriter.dstSet = m_descriptorSets[samplerRes.set];
		descriptorWriter.dstBinding = samplerRes.binding;
		descriptorWriter.dstArrayElement = dstArrIndex;
		descriptorWriter.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWriter.descriptorCount = count;
		descriptorWriter.pImageInfo = imageInfos;

		{
			SH_PROFILE_SCOPE("vkUpdateDescriptorSets - VulkanShader::setInput(const std::string& name, uint32_t count, const Ref<Texture2D>* pTextures, uint32_t dstArrIndex)");
			vkUpdateDescriptorSets(VulkanContext::getVulkanDevice()->getVkDevice(), 1, &descriptorWriter, 0, nullptr);
		}
	}

	void VulkanShader::writeDescriptorSet(const std::string& name, const Mesh& mesh)
	{
		auto& textures = mesh.getTextures();
		writeDescriptorSet(name, textures.size(), textures.data());
	}

	void VulkanShader::retrieveShaderResources()
	{
		SH_PROFILE_FUNCTION();

		m_resources = createScope<VulkanShaderResources>();
		spirv_cross::ShaderResources* allocation = new spirv_cross::ShaderResources[3];

		if (m_stages & ShaderStage::Vertex)
		{
			SH_TRACE("shader name: %s (vertex)", m_name.c_str());

			spirv_cross::Compiler vertexShaderCompiler(m_vertexShaderCode);
			spirv_cross::ShaderResources* vertReflRes = new (allocation) spirv_cross::ShaderResources(vertexShaderCompiler.get_shader_resources());
			reflect(vertexShaderCompiler, *vertReflRes, VK_SHADER_STAGE_VERTEX_BIT);
		}

		if (m_stages & ShaderStage::Fragment)
		{
			SH_TRACE("shader name: %s (fragment)", m_name.c_str());

			spirv_cross::Compiler fragShaderCompiler(m_fragmentShaderCode);
			spirv_cross::ShaderResources* fragReflResources = new (allocation + 1) spirv_cross::ShaderResources(fragShaderCompiler.get_shader_resources());
			reflect(fragShaderCompiler, *fragReflResources, VK_SHADER_STAGE_FRAGMENT_BIT);
		}

		if (m_stages & ShaderStage::Compute)
		{
			SH_TRACE("shader name: %s (compute)", m_name.c_str());

			spirv_cross::Compiler computeShaderCompiler(m_computeShaderCode);
			spirv_cross::ShaderResources* computeReflRes = new (allocation + 2) spirv_cross::ShaderResources(computeShaderCompiler.get_shader_resources());
			reflect(computeShaderCompiler, *computeReflRes, VK_SHADER_STAGE_COMPUTE_BIT);
		}

		delete[] allocation;

		for(size_t i_layout = 0; i_layout < m_resources->descriptorSetLayouts.size(); i_layout++)
		{
			const auto& setBinding = m_resources->descriptorSetLayouts[i_layout].bindings;
			VkDescriptorBindingFlags* bindingFlags = new VkDescriptorBindingFlags[setBinding.size()];

			for (size_t i_binding = 0; i_binding < setBinding.size(); i_binding++)
			{
				bindingFlags[i_binding] = setBinding[i_binding].descriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT ?
					0 : VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
			}

			VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCI{};
			bindingFlagsCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
			bindingFlagsCI.bindingCount = static_cast<uint32_t>(setBinding.size());
			bindingFlagsCI.pBindingFlags = bindingFlags;

			VkDescriptorSetLayoutCreateInfo layoutInfo{};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = static_cast<uint32_t>(setBinding.size());
			layoutInfo.pBindings = setBinding.data();
			layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
			layoutInfo.pNext = &bindingFlagsCI;

			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(VulkanContext::getVulkanDevice()->getVkDevice(), 
				&layoutInfo, nullptr, &m_resources->descriptorSetLayouts[i_layout].layout));

			delete[] bindingFlags;
		}
	}

	VkShaderModule VulkanShader::createShaderModule(const std::vector<uint32_t>& shaderCode)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = shaderCode.size() * sizeof(uint32_t);
		createInfo.pCode = shaderCode.data();

		VkShaderModule shaderModule;
		VK_CHECK_RESULT(vkCreateShaderModule(VulkanContext::getVulkanDevice()->getVkDevice(), &createInfo, nullptr, &shaderModule));
		return shaderModule;
	}

	void VulkanShader::createDescriptorSetAllocator()
	{	
		for (uint32_t i = 0; i < m_resources->descriptorSetLayouts.size(); i++)
		{
			m_setLayouts[i] = m_resources->descriptorSetLayouts[i].layout;

			if (!m_resources->descriptorSetLayouts[i].bindings.empty())
				m_usedDescriptorSets.emplace_back(i);
		}

		m_descriptorSetAllocator = createScope<DescriptorSetAllocator>(m_resources->descriptorSetLayouts);
		m_descriptorSetAllocator->allocateDescriptorSets(m_descriptorSets, m_setLayouts);
	}

	void VulkanShader::reflect(const spirv_cross::Compiler& compiler, spirv_cross::ShaderResources& reflResources,
		VkShaderStageFlagBits shaderType)
	{
		SH_PROFILE_FUNCTION();

		// uniform buffers /////////////////////////////////////////////
		for (size_t i = 0; i < reflResources.uniform_buffers.size(); i++)
		{
			Resource uboRes{};
			uboRes.type = ResourceType::UniformBuffer;

			const auto& ubo = reflResources.uniform_buffers[i];
			const auto& baseType = compiler.get_type(ubo.base_type_id);

			const auto& type = compiler.get_type(ubo.type_id);
			uboRes.set = compiler.get_decoration(ubo.id, spv::DecorationDescriptorSet);
			uboRes.binding = compiler.get_decoration(ubo.id, spv::DecorationBinding);
			uboRes.size = compiler.get_declared_struct_size(baseType);
			uboRes.arraySize = type.array.empty() ? 1 : type.array[0];

			const std::string& uboName = compiler.get_name(ubo.id);
			m_resources->resources[uboName] = uboRes;

			VkDescriptorSetLayoutBinding uboBinding{};
			uboBinding.binding = uboRes.binding;
			uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uboBinding.descriptorCount = uboRes.arraySize;
			uboBinding.stageFlags |= shaderType;
			uboBinding.pImmutableSamplers = nullptr;
			m_resources->descriptorSetLayouts[uboRes.set].bindings.emplace_back(uboBinding);

			SH_TRACE("type: uniform buffer\n\t  name: %s\n\t  set: %u\n\t  binding: %u\n\t  size: %u", uboName.c_str(), uboRes.set, uboRes.binding, uboRes.size);
		}

		// sampled images //////////////////////////////////////////////
		for (size_t i = 0; i < reflResources.sampled_images.size(); i++)
		{
			const auto& sampler = reflResources.sampled_images[i];
			const auto& type = compiler.get_type(sampler.type_id);

			Resource samplerRes{};
			samplerRes.type = ResourceType::SampledImage;
			samplerRes.set = compiler.get_decoration(sampler.id, spv::DecorationDescriptorSet);
			samplerRes.binding = compiler.get_decoration(sampler.id, spv::DecorationBinding);
			samplerRes.arraySize = type.array.empty()? 1 : type.array[0];

			const std::string& samplerName = compiler.get_name(sampler.id);
			m_resources->resources[samplerName] = samplerRes;

			VkDescriptorSetLayoutBinding samplerBinding{};
			samplerBinding.binding = compiler.get_decoration(sampler.id, spv::DecorationBinding);
			samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			samplerBinding.descriptorCount = samplerRes.arraySize;
			samplerBinding.stageFlags |= shaderType;
			samplerBinding.pImmutableSamplers = nullptr;
			m_resources->descriptorSetLayouts[samplerRes.set].bindings.emplace_back(samplerBinding);

			SH_TRACE("type: image sampler\n\t  name: %s\n\t  set: %u\n\t  binding: %u\n\t  array size: %u", 
				samplerName.c_str(), samplerRes.set, samplerRes.binding, samplerRes.arraySize);
		}

		// subpass inputs //////////////////////////////////////////////
		for (size_t i = 0; i < reflResources.subpass_inputs.size(); i++)
		{
			const auto& subpassInput = reflResources.subpass_inputs[i];
			Resource subpassInputRes{
				ResourceType::SubpassInput,
				compiler.get_decoration(subpassInput.id, spv::DecorationDescriptorSet),
				compiler.get_decoration(subpassInput.id, spv::DecorationBinding)
			};
			const std::string& subpassInputName = compiler.get_name(subpassInput.id);
			m_resources->resources[subpassInputName] = subpassInputRes;

			VkDescriptorSetLayoutBinding subpassInputBinding{};
			subpassInputBinding.binding = compiler.get_decoration(subpassInput.id, spv::DecorationBinding);
			subpassInputBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			subpassInputBinding.descriptorCount = 1;
			subpassInputBinding.stageFlags |= shaderType;
			subpassInputBinding.pImmutableSamplers = nullptr;
			m_resources->descriptorSetLayouts[subpassInputRes.set].bindings.emplace_back(subpassInputBinding);

			SH_TRACE("type: subpass input\n\t  name: %s\n\t  set: %u\n\t  binding: %u", subpassInputName.c_str(), subpassInputRes.set, subpassInputRes.binding);
		}

		// storage buffers /////////////////////////////////////////////
		for(size_t i = 0; i < reflResources.storage_buffers.size(); i++)
		{
			Resource ssboRes{};
			ssboRes.type = ResourceType::StorageBuffer;

			const auto& ssbo = reflResources.storage_buffers[i];
			const auto& baseType = compiler.get_type(ssbo.base_type_id);
			const auto& type = compiler.get_type(ssbo.type_id);

			ssboRes.set = compiler.get_decoration(ssbo.id, spv::DecorationDescriptorSet);
			ssboRes.binding = compiler.get_decoration(ssbo.id, spv::DecorationBinding);
			ssboRes.arraySize = type.array.empty() ? 1 : type.array[0];

			m_resources->resources[ssbo.name] = ssboRes;

			VkDescriptorSetLayoutBinding ssboBinding{};
			ssboBinding.binding = ssboRes.binding;
			ssboBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			ssboBinding.descriptorCount = ssboRes.arraySize;
			ssboBinding.stageFlags |= shaderType;
			ssboBinding.pImmutableSamplers = nullptr;
			m_resources->descriptorSetLayouts[ssboRes.set].bindings.emplace_back(ssboBinding);

			SH_TRACE("type: storage buffer\n\t  name: %s\n\t  set: %u\n\t  binding: %u", ssbo.name.c_str(), ssboRes.set, ssboRes.binding);
		}

		// push constants //////////////////////////////////////////////
		if (!reflResources.push_constant_buffers.empty())
		{
			auto& type = compiler.get_type(reflResources.push_constant_buffers[0].base_type_id);
			auto ranges = compiler.get_active_buffer_ranges(reflResources.push_constant_buffers[0].id);

			VkPushConstantRange pushConstant{};
			pushConstant.stageFlags |= shaderType;
			pushConstant.offset = ranges[0].offset;
			pushConstant.size = compiler.get_declared_struct_size(type);
			m_pushConstantRanges.emplace_back(pushConstant);
		}
	}
}