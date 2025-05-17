#include "Player.hpp"

using namespace Shadow;

GhostyCat::GhostyCat()
{
	m_ghosty = Texture2D::create("C:/dev/Shadow/Sandbox/assets/textures/ghosty.png");
}

void GhostyCat::onRender(float aspectRatio)
{
	Renderer2D::drawQuad({ aspectRatio * -5.0f,-5.0f, -0.5f }, { aspectRatio * 5.0f, 7.0f }, m_ghosty);
}

DemonyCat::DemonyCat()
{
	m_demony = Texture2D::create("C:/dev/Shadow/Sandbox/assets/textures/demony.png");
}

void DemonyCat::onRender(float aspectRatio) 
{
	Renderer2D::drawQuad({ aspectRatio * -6.5f,-5.0f, -0.5f }, { aspectRatio * 5.0f, 7.0f }, m_demony);
}
