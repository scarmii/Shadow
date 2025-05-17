#include "shpch.hpp"
#include "Shadow/Core/LayerStack.hpp"

namespace Shadow
{
	LayerStack::LayerStack()
	{
	}

	LayerStack::~LayerStack()
	{
		for (Layer* layer : m_layers)
			delete layer;
	}

	void LayerStack::pushLayer(Layer* layer)
	{
		m_layers.emplace(m_layers.begin() + m_layerInsertIndex, layer);
		layer->onAttach();
		m_layerInsertIndex++;
	}

	void LayerStack::pushOverlay(Layer* overlay)
	{
		m_layers.emplace_back(overlay);
		overlay->onAttach();
	}

	void LayerStack::popLayer(Layer* layer)
	{
		auto it = std::find(m_layers.cbegin(), m_layers.cend(), layer);
		if (it != m_layers.cend())
		{
			m_layers.erase(it);
			m_layerInsertIndex--;
		}
	}

	void LayerStack::popOverlay(Layer* overlay)
	{
		auto it = std::find(m_layers.cbegin(), m_layers.cend(), overlay);
		if (it != m_layers.cend())
			m_layers.erase(it);
	}
}