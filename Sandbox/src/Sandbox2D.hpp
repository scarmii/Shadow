#pragma once

#include "Shadow.hpp"

class Sandbox2D : public Shadow::Layer
{
public:
	Sandbox2D();
	~Sandbox2D() = default;

	virtual void onAttach() override;
	virtual void onDetach() override;
	virtual void onUpdate(Shadow::Timestep ts) override;
	virtual void onRender() override;
	virtual void onImGuiRender() override;
private:
	Shadow::OrthoCameraController m_cameraController;
	glm::vec3 m_squareColor = { 0.0f, 0.2f, 0.8f };

	Shadow::Ref<Shadow::Texture2D> m_catTex;
	Shadow::Ref<Shadow::Texture2D> m_assasinTex;
};
