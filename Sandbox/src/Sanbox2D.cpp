#include "Sandbox2D.hpp"

#include "imgui/imgui.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/ext.hpp>

Sandbox2D::Sandbox2D()
	: m_cameraController(1280.0f/720.0f, true)
{
}

void Sandbox2D::onAttach()
{
	m_cameraController.setCameraTranslationSpeed(1.0f);

	m_catTex = Shadow::Texture2D::create("C:/dev/Shadow/Shadow/assets/textures/cat.png");
	m_assasinTex = Shadow::Texture2D::create("C:/dev/Shadow/Shadow/assets/textures/assasin_girl.png");
}

void Sandbox2D::onDetach()
{
}

void Sandbox2D::onUpdate(Shadow::Timestep ts)
{
	m_cameraController.onUpdate(ts);
}

void Sandbox2D::onRender()
{
	Shadow::Renderer2D::resetStats();

	Shadow::QuadProperties blueCat{};
	blueCat.position = glm::vec3{ -0.5f,-0.75f,0.7f };
	blueCat.size = glm::vec2(0.75f);
	blueCat.color = glm::vec4{m_squareColor, 1.0f };
	blueCat.texture = m_catTex;
	blueCat.tilingFactor = 5.0f;

	Shadow::QuadProperties assasinTex{};
	assasinTex.position = glm::vec3{ 0.5f,-0.75f,0.6f };
	assasinTex.size = glm::vec2(0.5f);
	assasinTex.color = glm::vec4{ 0.8f, 0.1f, 0.3f, 0.9f };
	assasinTex.texture = m_assasinTex;
	assasinTex.tilingFactor = 2.0f;

	static float angle = 0.0f;

	Shadow::Renderer2D::beginScene(m_cameraController.getCamera());

	blueCat.tilingFactor = 1.0f;
	blueCat.position = { -0.9f,0.0f,0.3f };
	Shadow::Renderer2D::drawQuad(blueCat); 
	blueCat.color = { 0.5f,0.5f,0.5f,1.0f };
	angle += 0.1f;

	Shadow::Renderer2D::drawQuad({ 0.5f,0.5f,0.1f }, { 0.25f,0.25f }, { 0.5f,0.4f,0.6f,0.8f });
	Shadow::Renderer2D::drawRotatedQuad({ 0.0f,-0.5f,0.75f }, {1.0f,1.0f }, -angle, m_assasinTex);
	Shadow::Renderer2D::drawQuad(assasinTex);
	Shadow::Renderer2D::drawRotatedQuad(blueCat, angle);

	Shadow::Renderer2D::endScene();
}

void Sandbox2D::onImGuiRender() 
{
	SH_PROFILE_FUNCTION();

	float frameRate = Shadow::ShEngine::get().getFrameRate();
	uint32_t fps = static_cast<uint32_t>(1000.0f / frameRate);
	auto& stats = Shadow::Renderer2D::getStats(); 

	ImGui::Begin("Settings");
	ImGui::Text("Renderer2D stats: ");
	ImGui::Text("Draw calls: %u", stats.drawCalls);
	ImGui::Text("Quads: %u", stats.quadCount);
	ImGui::Text("Vertices: %u", stats.getTotalVertexCount());
	ImGui::Text("Indices: %u", stats.getTotalIndexCount());

	ImGui::Text("fps: %u", fps);
	ImGui::Text("Frame time: %f ms", frameRate);
	ImGui::ColorEdit3("Square color", &m_squareColor.x);
	ImGui::End();
}
