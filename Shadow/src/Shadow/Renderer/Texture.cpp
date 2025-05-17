#include"shpch.hpp"
#include"Shadow/Renderer/Texture.hpp"
#include"Shadow/Renderer/Renderer.hpp"
#include"Shadow/Vulkan/VkTexture.hpp"

namespace Shadow
{
	SH_FLAG_DEF(AttachmentUsage, uint8_t);

	Ref<Texture2D> Texture2D::create(uint32_t width, uint32_t height, const Sampler& sampler)
	{
		switch (Renderer::getRendererType())
		{
			case RendererType::Vulkan: return createRef<VulkanTexture2D>(width, height, sampler);
		}

		SH_ASSERT(false, "unknown renderer API :(");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::create(const std::string& imagePath, const Sampler& sampler)
	{
		switch (Renderer::getRendererType())
		{
			case RendererType::Vulkan: return createRef<VulkanTexture2D>(imagePath, sampler);
		}

		SH_ASSERT(false, "unknown renderer API :(");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::create(uint8_t* pixels, uint32_t length, const Sampler& sampler)
	{
		switch (Renderer::getRendererType())
		{
			case RendererType::Vulkan: return createRef<VulkanTexture2D>(pixels, length, sampler);
		}

		SH_ASSERT(false, "unknown renderer API :(");
		return nullptr;
	}
}