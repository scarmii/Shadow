#include "Level.hpp"

using namespace Shadow;

void Level::init()
{
	m_ghosty = createScope<GhostyCat>();
	m_demony = createScope<DemonyCat>();

	m_background = Texture2D::create("C:/dev/Shadow/Sandbox/assets/textures/background.jpg");
}

void Level::onRender()
{
	auto& window = ShEngine::get().getWindow();

	Renderer2D::drawQuad({ 0.0f,-1.0f,-0.9f }, { window.getAspectRatio() *16.0f, 18.0f }, m_background);

	m_ghosty->onRender(window.getAspectRatio());
	m_demony->onRender(window.getAspectRatio());
}
