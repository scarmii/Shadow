#pragma once

#include"Shadow/Core/Layer.hpp"

#include<vector>

namespace Shadow
{
	class LayerStack
	{
	public:
		LayerStack();
		~LayerStack();

		void pushLayer(Layer* layer);
		void pushOverlay(Layer* overlay);
		void popLayer(Layer* layer);
		void popOverlay(Layer* overlay);

		inline std::vector<Layer*>::iterator begin() { return m_layers.begin(); }
		inline std::vector<Layer*>::iterator end() { return m_layers.end(); }
	private:
		std::vector<Layer*> m_layers;
		uint32_t m_layerInsertIndex = 0;
	};
}