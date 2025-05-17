#include "GameLayer.hpp"

using namespace Shadow;

GameLayer::GameLayer()
{
	auto& window = ShEngine::get().getWindow();
	createCamera(window.getWidth(), window.getHeight());
}

void GameLayer::onAttach()
{
	EventDispatcher::get().addReciever(SH_CALLBACK(GameLayer::onWindowResize));

	m_level.init();
}

void GameLayer::onDetach()
{
}

void GameLayer::onRender()
{
	Renderer2D::beginScene(*m_camera);

	m_level.onRender();

	Renderer2D::endScene();
}

void GameLayer::onImGuiRender()
{
}

bool GameLayer::onWindowResize(const Shadow::WindowResizedEvent& e)
{
	createCamera(e.width, e.height);
	return false;
}

void GameLayer::createCamera(uint32_t width, uint32_t height)
{
	float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

	float camWidth = 8.0f;
	float bottom = -camWidth;
	float top = camWidth;
	float left = bottom * aspectRatio;
	float right = top * aspectRatio;
	m_camera = createScope<OrthoCamera>(left, right, bottom, top);
}
