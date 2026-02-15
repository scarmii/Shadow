#include "shpch.h"
#include "shadow/utils/rendergraphWriter.h"

namespace Shadow
{
	void RenderGraphWriter::begin(const std::string& filepath)
	{
		m_outputStream.open(filepath);
		m_outputStream << "{\n";
		m_outputStream << "\t\"rendergrahData\": [\n";
		m_outputStream << "\t{\n";
		m_outputStream.flush();
	}

	void RenderGraphWriter::writeData(const RenderGraph& rendergraph)
	{
		auto& passes = rendergraph.getPasses();

		if (m_writeCount > 0)
			m_outputStream << "\t},\n\t{\n\t\t\"ptr\": \"" << &rendergraph << "\",\n\t\t\"renderPasses\": \n\t\t{\n";
		else
			m_outputStream << "\t\t\"ptr\": \"" << &rendergraph << "\",\n\t\t\"renderPasses\": \n\t\t{\n";

		for (size_t i = 0; i < passes.size()-1; i++)
			writePassInfo(passes[i], false);

		writePassInfo(passes.back(), true);

		m_outputStream << "\n\t\t}\n";
		m_outputStream.flush();
		m_writeCount++;
	}

	void RenderGraphWriter::end()
	{
		m_outputStream << "\t}]\n}";
		m_outputStream.flush();
		m_outputStream.close();
	}

	void RenderGraphWriter::writePassInfo(const Ref<RenderGraphPass>& pass, bool lastPass)
	{

		std::stringstream ss[5];
		m_outputStream << "\t\t\t\"" << pass->getName() << "\":\n\t\t\t{\n";
		{
			if (pass->getType() == QueueType::Compute)
			{
				m_outputStream << "\t\t\t\t\"type\": \"compute\",\n";

				Ref<ComputePass> computePass = as<ComputePass>(pass);
				auto& storageOutputs = computePass->getStorageOutputs();
				auto& storageInputs = computePass->getStorageOutputs();

				writeResInfo("storageInputs", storageInputs, ss[2]);
				writeResInfo("storageOutputs", storageOutputs, ss[3]);
			}
			else
			{
				m_outputStream << "\t\t\t\t\"type\": \"graphics\",\n";

				Ref<DrawPass> drawPass = as<DrawPass>(pass);
				auto& colorOutputs = drawPass->getColorOutputs();
				auto& inputAttachments = drawPass->getInputAttachments();
				auto& textureInputs = drawPass->getTextureInputs();
				ImageResource* pDepthRes = drawPass->getDepthOutput();

				writeResInfo("colorOutputs", colorOutputs, ss[0]);
				writeResInfo("textureInputs", textureInputs, ss[1]);

				m_outputStream << "\t\t\t\t\"" << "inputAttachments" << "\": ";
				if (!inputAttachments.empty())
				{
					ss[4] << "[";
					for (size_t i = 0; i < inputAttachments.size() - 1; i++)
						ss[4] << "\"" << inputAttachments[i] << "\", ";

					ss[4] << "\"" << inputAttachments.back() << "\"";
					ss[4] << "],\n";
					m_outputStream << ss[4].str();
				}
				else
					m_outputStream << "\"none\",\n";

				for (uint8_t i = 0; i < sizeof(ss) / sizeof(std::stringstream); i++)
					ss[i].clear();

				if (pDepthRes)
					m_outputStream << "\t\t\t\t\"depthOutput\": \"" << pDepthRes->getName() << "\"";
				else
					m_outputStream << "\t\t\t\t\"depthOutput\": \"none\"";
			}
		}
		
		if (lastPass)
			m_outputStream << "\n\t\t\t}";
		else
			m_outputStream << "\n\t\t\t},\n";
	}

	void RenderGraphWriter::writeResInfo(const std::string& resourceType, const std::vector<ImageResource*> resources, std::stringstream& ss)
	{
		m_outputStream << "\t\t\t\t\"" << resourceType << "\": ";
		if (!resources.empty())
		{
			ss << "[";
			for (size_t i = 0; i < resources.size() - 1; i++)
				ss << "\"" << resources[i]->getName() << "\", ";

			ss << "\"" << resources[resources.size() - 1]->getName() << "\"";
			ss << "]";
			m_outputStream << ss.str();
		}
		else
			m_outputStream << "\"none\"";

		if (resourceType != "storageOutputs")
			m_outputStream << ",\n";
	}
}