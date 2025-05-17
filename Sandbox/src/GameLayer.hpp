#pragma once

#include "Level.hpp"

class GameLayer : public Shadow::Layer
{
public:
	GameLayer();
	~GameLayer() = default;

	virtual void onAttach() override;
	virtual void onDetach() override;
	virtual void onUpdate(Shadow::Timestep ts) override {}
	virtual void onRender() override;
	virtual void onImGuiRender() override;
private:
	bool onWindowResize(const Shadow::WindowResizedEvent& e);
	void createCamera(uint32_t width, uint32_t height);
private:
	Shadow::Scope<Shadow::OrthoCamera> m_camera;

	Level m_level;
};