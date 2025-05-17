#include "shpch.hpp"
#include "Shadow/Renderer/Shader.hpp"
#include "Shadow/Renderer/Renderer.hpp"
#include "Shadow/Core/Core.hpp"
		 
#include "Shadow/Vulkan/VulkanShader.hpp"
#include "Shadow/Vulkan/VulkanContext.hpp"
		 
#include <fstream>

namespace Shadow
{
	SH_FLAG_DEF(ShaderStage, uint8_t);

	Ref<Shader> Shader::create(const std::string& name, const std::string& vertSpv, const std::string& fragSpv)
	{
		switch (Renderer::getRendererType())
		{
			case RendererType::None:   return nullptr;
			case RendererType::Vulkan: return createRef<VulkanShader>(name, vertSpv, fragSpv);
		}
		SH_TRACE("Shader::create(%s, %s, %s) returned nullptr: unknown renderer API :(", name.c_str(), vertSpv.c_str(), fragSpv.c_str());
		return nullptr;
	}

	Ref<Shader> Shader::create(const std::string& name, const std::string& computeSpv)
	{
		switch (Renderer::getRendererType())
		{
			case RendererType::None:   return nullptr;
			case RendererType::Vulkan: return createRef<VulkanShader>(name, computeSpv);
		}
		SH_TRACE("Shader::create(%s, %s) returned nullptr: unknown renderer API :(", name.c_str(), computeSpv.c_str());
		return nullptr;
	}

	Ref<Shader> Shader::create(const std::string& name, const std::string& vertSpv, const std::string& fragSpv, const std::string& computeSpv)
	{
		switch (Renderer::getRendererType())
		{
			case RendererType::None:   return nullptr;
			case RendererType::Vulkan: return createRef<VulkanShader>(name, vertSpv, fragSpv, computeSpv);
		}
		SH_TRACE("Shader::create(%s, %s, %s, %s) returned nullptr: unknown renderer API :(", name.c_str(), vertSpv.c_str(), fragSpv.c_str(), computeSpv.c_str());
		return nullptr;
	}

	std::vector<uint32_t> Shader::readFile(const std::string& filepath)
	{
		std::ifstream in(filepath, std::ios::ate | std::ios::binary);
		SH_ASSERT(in.is_open(), "failed to open %s :(", filepath.c_str());

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
		SH_ASSERT(!exists(name), "Shader '%s' already exists");
		m_shaders[name] = shader;
	}

	Ref<Shader> ShaderLibrary::load(const std::string& name, const std::string& vertSpv, const std::string& fragSpv)
	{
		Ref<Shader> shader = Shader::create(name, vertSpv, fragSpv);
		add(shader);
		return shader;
	}

	Ref<Shader> ShaderLibrary::load(const std::string& name, const std::string& computeSpv)
	{
		Ref<Shader> shader = Shader::create(name, computeSpv);
		add(shader);
		return shader;
	}

	Ref<Shader> ShaderLibrary::get(const std::string& name)
	{
		SH_ASSERT(exists(name), "Shader '%s' doesn't exist :<");
		return m_shaders[name];
	}
}