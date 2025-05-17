#pragma once

#include "Shadow.hpp"
#include "Player.hpp"

class Level
{
public:
	void init();
	
	void onRender();
private:
	Shadow::Scope<Player> m_ghosty, m_demony;

	Shadow::Ref<Shadow::Texture2D> m_background;
};