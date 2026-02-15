#include "sandbox2D.h"

#include "imgui/imgui.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/ext.hpp>
#include <imgui_internal.h>

using namespace Shadow;

static const uint32_t s_mapWidth = 24;
static const char* s_mapTiles =
"WWWWWWWWWWWWWWWWWWWWWWWW"
"WWWWWWWDDDDDDWWWWWWWWWWW"
"WWWWDDDDDDDDDDDWWWWWWWWW"
"WWWDDDDDDDDDDDDDWWWWWWWW"
"WWDDDDWWWDDDDDDDDWWWWWWW"
"WDDDDDWWWDDDDDDDDDWWWWWW"
"WWDDDDDDDDDDDDDDWWWWWWWW"
"WWWDDDDDDDDDDDWWWWWWWWWW"
"WWWWWDDDDDDDWWWWWWWWWWWW"
"WWWWWWWDDDDWWWWWWWWWWWWW"
"WWCWWWWWWWWWWWWWWWWWWWWW"
"WWWWWWWWWWWWWWWWWWWWWWWW"
"WWWWWWWWWWWWWWWWWWWWCWWW"
"WWWWWWWWWWWWWWWWWWWWWWWW";

Sandbox2D::Sandbox2D()
	: m_cameraController(1280.0f/720.0f, true), m_particleSystem(1000)
{
	m_particle.colorBegin = { 254 / 255.0f, 212 / 255.0f, 123 / 255.0f, 1.0f };
	m_particle.colorEnd = { 254 / 255.0f, 109 / 255.0f, 41 / 255.0f, 1.0f };
	m_particle.sizeBegin = 0.5f, m_particle.sizeVariation = 0.3f, m_particle.sizeEnd = 0.0f;
	m_particle.lifeTime = 1.0f;
	m_particle.velocity = { 0.0f, 0.0f };
	m_particle.velocityVariation = { 3.0f, 1.0f };
	m_particle.position = { 0.0f, 0.0f };

	m_cameraController.setCameraTranslationSpeed(1.0f);

	m_catTex = Texture2D::create("assets/textures/cat.png");
	m_assasinTex = Texture2D::create("assets/textures/assasin_girl.png");
	m_spriteSheet = Texture2D::create("assets/textures/RPGpack_sheet.png");

	m_mapWidth = s_mapWidth;
	m_mapHeight = strlen(s_mapTiles) / s_mapWidth;

	m_texStairs = Sprite2D::create(m_spriteSheet, glm::vec2{ 0, 11 }, glm::vec2{ 128,128 });

	m_spriteMap['D'] = Sprite2D::create(m_spriteSheet, glm::vec2{ 6, 11 }, glm::vec2{ 128,128 });
	m_spriteMap['W'] = Sprite2D::create(m_spriteSheet, glm::vec2{ 11,11 }, glm::vec2{ 128,128 });
}

Sandbox2D::~Sandbox2D()
{
	ImGui::ClearWindowSettings("Settings");
}

void Sandbox2D::onUpdate(Timestep ts)
{
	m_cameraController.onUpdate(ts);
	if (Input::isMouseButtonPressed(MouseCode::ButtonLeft))
	{
		glm::vec2 mousePos = Input::getMousePosition();
		auto width = ShApp::get().getWindow().getWidth();
		auto height = ShApp::get().getWindow().getHeight();

		auto bounds = m_cameraController.getBounds();
		auto pos = m_cameraController.getCamera().getPosition();
		mousePos.x = (mousePos.x / width) * bounds.getWidth() - bounds.getWidth() * 0.5f;
		mousePos.y = bounds.getHeight() * 0.5f - (mousePos.y / height) * bounds.getHeight();
		m_particle.position = { mousePos.x + pos.x, mousePos.y + pos.y };

		for (int i = 0; i < 5; i++)
			m_particleSystem.emit(m_particle);
	}
	m_particleSystem.onUpdate(ts);
}

void Sandbox2D::onRender()
{
	Renderer2D::resetStats();

	QuadProperties blueCat{};
	blueCat.position = glm::vec3{ -0.5f,-0.75f,0.7f };
	blueCat.size = glm::vec2(0.75f);
	blueCat.color = glm::vec4{ m_squareColor, 1.0f };
	blueCat.texture = m_catTex;
	blueCat.tilingFactor = 5.0f;

	QuadProperties assasinTex{};
	assasinTex.position = glm::vec3{ 0.5f,-0.75f,0.6f };
	assasinTex.size = glm::vec2(0.5f);
	assasinTex.color = glm::vec4{ 0.8f, 0.1f, 0.3f, 0.9f };
	assasinTex.texture = m_assasinTex;
	assasinTex.tilingFactor = 2.0f;

	static float angle = 0.0f;

	Renderer2D::beginScene();

	blueCat.tilingFactor = 1.0f;
	blueCat.position = { -0.9f,0.0f,0.3f };
	Renderer2D::drawQuad(blueCat);
	blueCat.color = { 0.5f,0.5f,0.5f,1.0f };
	angle += 0.1f;

	Renderer2D::drawQuad({ 0.5f,0.5f,0.1f }, { 0.25f,0.25f }, { 0.5f,0.4f,0.6f,0.8f });
	Renderer2D::drawRotatedQuad({ 0.0f,-0.5f,0.6f }, { 1.0f,1.0f }, glm::radians(-angle), m_assasinTex);
	Renderer2D::drawQuad(assasinTex);
	Renderer2D::drawRotatedQuad(blueCat, glm::radians(angle));

	m_particleSystem.onRender(m_cameraController.getCamera());
	Renderer2D::endScene(m_cameraController.getCamera());
}

void Sandbox2D::onImGuiRender() 
{
	SH_PROFILE_FUNCTION();

	float frameRate = Shadow::ShApp::get().getFrameRate();
	uint32_t fps = static_cast<uint32_t>(1000.0f / frameRate);
	auto& stats = Shadow::Renderer2D::getStats(); 

	ImGui::Begin("Settings");
	ImGui::Text("Renderer2D stats: ");
	ImGui::Text("Draw calls: %u", stats.drawCall);
	ImGui::Text("Quads: %u", stats.quadCount);
	ImGui::Text("Vertices: %u", stats.getTotalVertexCount());
	ImGui::Text("Indices: %u", stats.getTotalIndexCount());

	ImGui::Text("fps: %u", fps);
	ImGui::Text("Frame time: %f ms", frameRate);
	ImGui::ColorEdit3("Square color", &m_squareColor.x);

	ImGui::End();
}
