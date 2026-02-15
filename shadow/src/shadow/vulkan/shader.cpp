#include "shpch.h"
#include "shadow/core/core.h"

#include "shadow/renderer/mesh.h"
#include "shadow/renderer/renderer.h"

#include "shadow/vulkan/shader.h"
#include "shadow/vulkan/context.h"
#include "shadow/vulkan/buffer.h"
#include "shadow/vulkan/texture.h"
#include "shadow/vulkan/renderPass.h"

#include <spirv_cross.hpp>
#include <spirv_reflect.hpp>

namespace Shadow
{
	SH_FLAG_DEF(ShaderStage, uint32_t);

	Shader::Shader(const std::string& name, const std::string vertPath, const std::string& fragPath)
		: m_name(name), m_resources{}
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

	Shader::Shader(const std::string& name, const std::string computeSpv)
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

	Shader::Shader(const std::string& name, const std::string vertPath, const std::string& fragPath, const std::string computeSpv)
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

	Shader::~Shader()
	{
		VkDevice vulkanDevice = VulkanContext::getDevice()->getVkDevice();

		for (uint32_t i = 0; i < m_resources.descriptorSetLayouts.size(); i++)
			vkDestroyDescriptorSetLayout(vulkanDevice, m_resources.descriptorSetLayouts[i].layout, nullptr);

		if (m_vertexShaderModule != VK_NULL_HANDLE)
			vkDestroyShaderModule(vulkanDevice, m_vertexShaderModule, nullptr);
		if (m_fragmentShaderModule != VK_NULL_HANDLE)
			vkDestroyShaderModule(vulkanDevice, m_fragmentShaderModule, nullptr);
		if (m_computeShaderModule != VK_NULL_HANDLE)
			vkDestroyShaderModule(vulkanDevice, m_computeShaderModule, nullptr);

		SH_TRACE("destroying shader: {name = %s}", m_name.c_str());
	}

	void Shader::setInput(const std::string& name, const Ref<UniformBuffer>& buffer)
	{
		SH_ASSERT((m_resources.resources.find(name) != m_resources.resources.end()),
			"failed to find the resource '%s' in the shader: {name = %s}", name.c_str(), m_name.c_str());

		const TextureResource& resource = m_resources.resources[name];
		VkDevice device = VulkanContext::getDevice()->getVkDevice();

		VkWriteDescriptorSet writers[Device::s_maxFramesInFlight]{};
		VkDescriptorBufferInfo bufferInfos[Device::s_maxFramesInFlight]{};

		for (uint32_t i = 0; i < Device::s_maxFramesInFlight; i++)
		{
			bufferInfos[i].buffer = buffer->getVkBuffer(i);
			bufferInfos[i].offset = 0;
			bufferInfos[i].range = buffer->getSize();

			writers[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writers[i].dstSet = m_descriptorSets[resource.set];
			writers[i].dstBinding = resource.binding;
			writers[i].dstArrayElement = 0;
			writers[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writers[i].descriptorCount = 1;
			writers[i].pBufferInfo = &bufferInfos[i];
		}
		vkUpdateDescriptorSets(device, Device::s_maxFramesInFlight, writers, 0, nullptr);
	}

	void Shader::setInput(const std::string& name, const Ref<VertexBuffer>& buffer)
	{
		SH_ASSERT(buffer->isStorageBuffer(), "you should have called VertexBuffer::create(..., storageBuffer = true), if the vertex buffer is supposed to be used as a shader storage buffer");

		Device* vulkanDevice = VulkanContext::getDevice();
		const TextureResource& resource = m_resources.resources[name];

		VkDescriptorBufferInfo bufferInfos[Device::s_maxFramesInFlight]{};
		VkWriteDescriptorSet writers[Device::s_maxFramesInFlight]{};

		for (uint32_t i = 0; i < Device::s_maxFramesInFlight; i++)
		{
			bufferInfos[i].buffer = buffer->getVkBuffer(i);
			bufferInfos[i].offset = 0;
			bufferInfos[i].range = buffer->getSize();

			writers[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writers[i].dstSet = m_descriptorSets[resource.set];
			writers[i].dstBinding = resource.binding;
			writers[i].dstArrayElement = 0;
			writers[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writers[i].descriptorCount = 1;
			writers[i].pBufferInfo = &bufferInfos[i];
		}
		vkUpdateDescriptorSets(vulkanDevice->getVkDevice(), Device::s_maxFramesInFlight, writers, 0, nullptr);
	}

	void Shader::setInput(const std::string& name, const Ref<StorageBuffer>& buffer)
	{
		SH_ASSERT((m_resources.resources.find(name) != m_resources.resources.end()),
			"failed to find the resource %s in the shader %s", name.c_str(), m_name.c_str());

		Device* vulkanDevice = VulkanContext::getDevice();
		const TextureResource& resource = m_resources.resources[name];

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = buffer->getVkBuffer();
		bufferInfo.offset = 0;
		bufferInfo.range = buffer->getSize();

		VkWriteDescriptorSet writer{};
		writer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writer.dstSet = m_descriptorSets[resource.set];
		writer.dstBinding = resource.binding;
		writer.dstArrayElement = 0;
		writer.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writer.descriptorCount = 1;
		writer.pBufferInfo = &bufferInfo;
		vkUpdateDescriptorSets(vulkanDevice->getVkDevice(), 1, &writer, 0, nullptr);
	}

	void Shader::setInput(const std::string& name, const std::string& inputAttachment)
	{
		SH_PROFILE_FUNCTION();

		uint32_t index = m_pRenderPass->getAttachmentIndex(inputAttachment);
		m_resources.inputAttachments.setAt(InputAttachment{ name, inputAttachment}, index);
		auto& subpassInputRes = m_resources.resources[name];
		
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_pRenderPass->getImageView(index);
		imageInfo.sampler = VK_NULL_HANDLE;
		
		VkWriteDescriptorSet descriptorWriter{};
		descriptorWriter.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWriter.dstSet = m_descriptorSets[subpassInputRes.set];
		descriptorWriter.dstBinding = subpassInputRes.binding;
		descriptorWriter.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		descriptorWriter.descriptorCount = 1;
		descriptorWriter.pImageInfo = &imageInfo;
		vkUpdateDescriptorSets(VulkanContext::getDevice()->getVkDevice(), 1, &descriptorWriter, 0, nullptr);
	}

	void Shader::setInput(const std::string& name, const Ref<Texture2D>& texture)
	{
		SH_PROFILE_FUNCTION();
		SH_ASSERT((m_resources.resources.find(name) != m_resources.resources.end()),
			"failed to find the resource '%s' in the shader '%s'", name.c_str(), m_name.c_str());

		if (texture->isRenderpassInput())
			m_resources.renderpassInputs[name] = texture;

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageView = texture->getImageView();
		imageInfo.sampler = texture->getSampler();

		auto& imageRes = m_resources.resources[name];
		VkWriteDescriptorSet descriptorWriter{};
		descriptorWriter.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWriter.dstSet = m_descriptorSets[imageRes.set];
		descriptorWriter.dstBinding = imageRes.binding;
		descriptorWriter.descriptorCount = 1;
		descriptorWriter.pImageInfo = &imageInfo;

		switch (imageRes.type)
		{
			case ResourceType::SampledImage:
			{
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				descriptorWriter.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				break;
			}
			case ResourceType::StorageImage:
			{
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
				descriptorWriter.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				break;
			}
			default:
				SH_ERROR("unknown image descriptor type, it should be either ResourceType::SampledImage or ResourceType::StorageImage");
		}
		vkUpdateDescriptorSets(VulkanContext::getDevice()->getVkDevice(), 1, &descriptorWriter, 0, nullptr);
	}

	void Shader::setInput(const std::string& name, uint32_t count, const Ref<Texture2D>* pTextures, uint32_t dstArrIndex)
	{
		SH_PROFILE_FUNCTION();
		SH_ASSERT((m_resources.resources.find(name) != m_resources.resources.end()),
			"failed to find the resource %s in the shader %s", name.c_str(), m_name.c_str());

		auto& samplerRes = m_resources.resources[name];
		VkDescriptorImageInfo imageInfos[32]{};

		for (uint32_t i = 0; i < count; i++)
		{
			imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfos[i].imageView = pTextures[i]->getImageView();
			imageInfos[i].sampler = pTextures[i]->getSampler();
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
			vkUpdateDescriptorSets(VulkanContext::getDevice()->getVkDevice(), 1, &descriptorWriter, 0, nullptr);
		}
	}

	void Shader::setInput(const std::string& name, const Mesh& mesh)
	{
		auto& textures = mesh.getTextures();
		setInput(name, textures.size(), textures.data());
	}

	void Shader::retrieveShaderResources()
	{
		SH_PROFILE_FUNCTION();
		spirv_cross::ShaderResources* sharedResources = new spirv_cross::ShaderResources[3];

		if (m_stages & ShaderStage::Vertex)
		{
			SH_TRACE("shader: {name = %s; type = vertex}", m_name.c_str());

			spirv_cross::Compiler vertexShaderCompiler(m_vertexShaderCode);
			sharedResources[0] = vertexShaderCompiler.get_shader_resources();
			reflect(vertexShaderCompiler, sharedResources[0], VK_SHADER_STAGE_VERTEX_BIT);
		}

		if (m_stages & ShaderStage::Fragment)
		{
			SH_TRACE("shader: {name = %s; type = fragment}", m_name.c_str());

			spirv_cross::Compiler fragShaderCompiler(m_fragmentShaderCode);
			sharedResources[1] = fragShaderCompiler.get_shader_resources();
			reflect(fragShaderCompiler, sharedResources[1], VK_SHADER_STAGE_FRAGMENT_BIT);
		}

		if (m_stages & ShaderStage::Compute)
		{
			SH_TRACE("shader: {name = %s; type = compute}", m_name.c_str());

			spirv_cross::Compiler computeShaderCompiler(m_computeShaderCode);
			sharedResources[2] = computeShaderCompiler.get_shader_resources();
			reflect(computeShaderCompiler, sharedResources[2], VK_SHADER_STAGE_COMPUTE_BIT);
		}

		delete[] sharedResources;

		for(size_t i_layout = 0; i_layout < m_resources.descriptorSetLayouts.size(); i_layout++)
		{
			const auto& setBinding = m_resources.descriptorSetLayouts[i_layout].bindings;
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

			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(VulkanContext::getDevice()->getVkDevice(), 
				&layoutInfo, nullptr, &m_resources.descriptorSetLayouts[i_layout].layout));

			delete[] bindingFlags;
		}
	}

	void Shader::updateDescriptorSets()
	{
		for (uint32_t i = 0; i < m_resources.inputAttachments.size; i++)
		{
			auto& subpassInput = m_resources.inputAttachments.array[i];
			setInput(subpassInput.shaderName, subpassInput.attachmentName);
			m_resources.inputAttachments.size--;
		}

		for (auto& inImage : m_resources.renderpassInputs)
			setInput(inImage.first, inImage.second);
	}

	VkShaderModule Shader::createShaderModule(const std::vector<uint32_t>& shaderCode)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = shaderCode.size() * sizeof(uint32_t);
		createInfo.pCode = shaderCode.data();

		VkShaderModule shaderModule;
		VK_CHECK_RESULT(vkCreateShaderModule(VulkanContext::getDevice()->getVkDevice(), &createInfo, nullptr, &shaderModule));
		return shaderModule;
	}

	void Shader::createDescriptorSetAllocator()
	{	
		for (uint32_t i = 0; i < m_resources.descriptorSetLayouts.size(); i++)
		{
			m_setLayouts[i] = m_resources.descriptorSetLayouts[i].layout;

			if (!m_resources.descriptorSetLayouts[i].bindings.empty())
				m_usedDescriptorSets.emplace_back(i);
		}

		m_descriptorSetAllocator = createScope<DescriptorSetAllocator>(m_resources.descriptorSetLayouts);
		m_descriptorSetAllocator->allocateDescriptorSets(m_descriptorSets, m_setLayouts);
	}

	void Shader::reflect(const spirv_cross::Compiler& compiler, spirv_cross::ShaderResources& reflResources,
		VkShaderStageFlagBits shaderType)
	{
		SH_PROFILE_FUNCTION();

		// uniform buffers /////////////////////////////////////////////
		for (size_t i = 0; i < reflResources.uniform_buffers.size(); i++)
		{
			TextureResource uboRes{};
			uboRes.type = ResourceType::UniformBuffer;
			uboRes.shaderType = static_cast<ShaderStage>(shaderType);

			const auto& ubo = reflResources.uniform_buffers[i];
			const auto& baseType = compiler.get_type(ubo.base_type_id);

			const auto& type = compiler.get_type(ubo.type_id);
			uboRes.set = compiler.get_decoration(ubo.id, spv::DecorationDescriptorSet);
			uboRes.binding = compiler.get_decoration(ubo.id, spv::DecorationBinding);
			uboRes.size = compiler.get_declared_struct_size(baseType);
			uboRes.arraySize = type.array.empty() ? 1 : type.array[0];

			const std::string& uboName = compiler.get_name(ubo.id);
			m_resources.resources[uboName] = uboRes;

			VkDescriptorSetLayoutBinding uboBinding{};
			uboBinding.binding = uboRes.binding;
			uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uboBinding.descriptorCount = uboRes.arraySize;
			uboBinding.stageFlags |= shaderType;
			uboBinding.pImmutableSamplers = nullptr;
			m_resources.descriptorSetLayouts[uboRes.set].bindings.emplace_back(uboBinding);

			SH_TRACE("resource: {type = uniform buffer; name = %s; set = %u; binding = %u; size = %u}", uboName.c_str(), uboRes.set, uboRes.binding, uboRes.size);
		}

		// sampled images //////////////////////////////////////////////
		for (size_t i = 0; i < reflResources.sampled_images.size(); i++)
		{
			const auto& sampler = reflResources.sampled_images[i];
			const auto& type = compiler.get_type(sampler.type_id);

			TextureResource samplerRes{};
			samplerRes.type = ResourceType::SampledImage;
			samplerRes.shaderType = static_cast<ShaderStage>(shaderType);
			samplerRes.set = compiler.get_decoration(sampler.id, spv::DecorationDescriptorSet);
			samplerRes.binding = compiler.get_decoration(sampler.id, spv::DecorationBinding);
			samplerRes.arraySize = type.array.empty()? 1 : type.array[0];

			const std::string& samplerName = compiler.get_name(sampler.id);
			m_resources.resources[samplerName] = samplerRes;

			VkDescriptorSetLayoutBinding samplerBinding{};
			samplerBinding.binding = compiler.get_decoration(sampler.id, spv::DecorationBinding);
			samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			samplerBinding.descriptorCount = samplerRes.arraySize;
			samplerBinding.stageFlags |= shaderType;
			samplerBinding.pImmutableSamplers = nullptr;
			m_resources.descriptorSetLayouts[samplerRes.set].bindings.emplace_back(samplerBinding);

			SH_TRACE("resource: {type = image sampler; name = %s; set = %u; binding = %u; array size = %u}", 
				samplerName.c_str(), samplerRes.set, samplerRes.binding, samplerRes.arraySize);
		}

		// subpass inputs //////////////////////////////////////////////
		for (size_t i = 0; i < reflResources.subpass_inputs.size(); i++)
		{
			const auto& subpassInput = reflResources.subpass_inputs[i];
			TextureResource subpassInputRes{
				ResourceType::SubpassInput,
				static_cast<ShaderStage>(shaderType),
				compiler.get_decoration(subpassInput.id, spv::DecorationDescriptorSet),
				compiler.get_decoration(subpassInput.id, spv::DecorationBinding)
			};
			const std::string& subpassInputName = compiler.get_name(subpassInput.id);
			m_resources.resources[subpassInputName] = subpassInputRes;

			VkDescriptorSetLayoutBinding subpassInputBinding{};
			subpassInputBinding.binding = compiler.get_decoration(subpassInput.id, spv::DecorationBinding);
			subpassInputBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			subpassInputBinding.descriptorCount = 1;
			subpassInputBinding.stageFlags |= shaderType;
			subpassInputBinding.pImmutableSamplers = nullptr;
			m_resources.descriptorSetLayouts[subpassInputRes.set].bindings.emplace_back(subpassInputBinding);

			SH_TRACE("resource: {type = subpass input; name = %s; set = %u; binding = %u}", subpassInputName.c_str(), subpassInputRes.set, subpassInputRes.binding);
		}

		// storage buffers /////////////////////////////////////////////
		for(size_t i = 0; i < reflResources.storage_buffers.size(); i++)
		{
			TextureResource ssboRes{};
			ssboRes.type = ResourceType::StorageBuffer;
			ssboRes.shaderType = static_cast<ShaderStage>(shaderType);

			const auto& ssbo = reflResources.storage_buffers[i];
			const auto& baseType = compiler.get_type(ssbo.base_type_id);
			const auto& type = compiler.get_type(ssbo.type_id);

			ssboRes.set = compiler.get_decoration(ssbo.id, spv::DecorationDescriptorSet);
			ssboRes.binding = compiler.get_decoration(ssbo.id, spv::DecorationBinding);
			ssboRes.arraySize = type.array.empty() ? 1 : type.array[0];

			m_resources.resources[ssbo.name] = ssboRes;

			VkDescriptorSetLayoutBinding ssboBinding{};
			ssboBinding.binding = ssboRes.binding;
			ssboBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			ssboBinding.descriptorCount = ssboRes.arraySize;
			ssboBinding.stageFlags |= shaderType;
			ssboBinding.pImmutableSamplers = nullptr;
			m_resources.descriptorSetLayouts[ssboRes.set].bindings.emplace_back(ssboBinding);

			SH_TRACE("resource: {type = storage buffer; name = %s; set = %u; binding = %u}", ssbo.name.c_str(), ssboRes.set, ssboRes.binding);
		}

		// storage images ///////////////////////////////////////////////
		for (size_t i = 0; i < reflResources.storage_images.size(); i++)
		{
			TextureResource storageImageRes{};
			storageImageRes.type = ResourceType::StorageImage;
			storageImageRes.shaderType = static_cast<ShaderStage>(shaderType);

			const auto& storageImage = reflResources.storage_images[i];
			const auto& baseType = compiler.get_type(storageImage.base_type_id);
			const auto& type = compiler.get_type(storageImage.type_id);

			storageImageRes.set = compiler.get_decoration(storageImage.id, spv::DecorationDescriptorSet);
			storageImageRes.binding = compiler.get_decoration(storageImage.id, spv::DecorationBinding);
			storageImageRes.arraySize = type.array.empty() ? 1 : type.array[0];

			m_resources.resources[storageImage.name] = storageImageRes;

			VkDescriptorSetLayoutBinding ssboBinding{};
			ssboBinding.binding = storageImageRes.binding;
			ssboBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			ssboBinding.descriptorCount = storageImageRes.arraySize;
			ssboBinding.stageFlags |= shaderType;
			ssboBinding.pImmutableSamplers = nullptr;
			m_resources.descriptorSetLayouts[storageImageRes.set].bindings.emplace_back(ssboBinding);

			SH_TRACE("resource: {type = storage buffer; name = %s; set = %u; binding = %u}", storageImage.name.c_str(), storageImageRes.set, storageImageRes.binding);
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

	std::vector<uint32_t> Shader::readFile(const std::string& filepath)
	{
		std::ifstream in(filepath, std::ios::ate | std::ios::binary);
		SH_ASSERT(in.is_open(), "failed to open %s", filepath.c_str());

		size_t fileSize = (size_t)in.tellg();
		std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
		in.seekg(0);
		in.read(reinterpret_cast<char*>(buffer.data()), fileSize);
		in.close();
		return buffer;
	}

	void ShaderLibrary::add(const Ref<Shader>& shader)
	{
		const std::string& name = shader->getName();
		SH_ASSERT(!exists(name), "shader with name = '%s' already exists");
		m_shaders[name] = shader;
	}

	Ref<Shader> ShaderLibrary::load(const std::string& name, const std::string& vertSpv, const std::string& fragSpv)
	{
		Ref<Shader> shader = createRef<Shader>(name, vertSpv, fragSpv);
		add(shader);
		return shader;
	}

	Ref<Shader> ShaderLibrary::load(const std::string& name, const std::string& computeSpv)
	{
		Ref<Shader> shader = createRef<Shader>(name, computeSpv);
		add(shader);
		return shader;
	}

	Ref<Shader> ShaderLibrary::get(const std::string& name)
	{
		SH_ASSERT(exists(name), "shader with name = '%s' doesn't exist");
		return m_shaders[name];
	}
}