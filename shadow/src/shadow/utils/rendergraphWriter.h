#pragma once

#include "shadow/renderer/rendergraph.h"

namespace Shadow
{
	class RenderGraphWriter
	{
	public:
		void begin(const std::string& filepath = "results.json");
		void writeData(const RenderGraph& rendergraph);
		void end();
	private:
		void writePassInfo(const Ref<RenderGraphPass>& pass, bool lastPass);
		void writeResInfo(const std::string& resourceType, const std::vector<ImageResource*> resources, std::stringstream& ss);
	private:
		std::ofstream m_outputStream;
		uint32_t m_writeCount;
	};
}