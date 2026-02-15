#include "shpch.h"
#include "shadow/core/core.h"
#include "shadow/core/inputCodes.h"
#include "shadow/core/input.h"

#include "shadow/events/eventDispatcher.h"
#include "shadow/renderer/cameraController.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/ext.hpp>

namespace Shadow
{
	PerspectiveCameraController::PerspectiveCameraController(float fov, float aspectRatio)
		: m_fov(fov), m_aspectRatio(aspectRatio), m_camera(fov, aspectRatio, 0.1f, 1000.0f)
	{
		EventDispatcher& dispatcher = EventDispatcher::get();
		dispatcher.addReciever(SH_ON_EVENT_FN(onMouseMoved, MouseMovedEvent));
		dispatcher.addReciever(SH_ON_EVENT_FN(onMouseScrolled, MouseScrolledEvent));
		dispatcher.addReciever(SH_ON_EVENT_FN(onWindowResized, WindowResizedEvent));

		m_camera.setPosition(glm::vec3(0.0f, 0.0f, 3.0f));
	}

	void PerspectiveCameraController::onUpdate(Timestep ts)
	{
		if (Input::isKeyPressed(KeyCode::W))
			m_camera.move(glm::vec3(0, 0, m_cameraTranslationSpeed * ts));
		else if (Input::isKeyPressed(KeyCode::S))
			m_camera.move(glm::vec3(0, 0, -m_cameraTranslationSpeed * ts));

		if (Input::isKeyPressed(KeyCode::A))
			m_camera.move(glm::vec3(m_cameraTranslationSpeed * ts, 0, 0));
		else if (Input::isKeyPressed(KeyCode::D))
			m_camera.move(glm::vec3(-m_cameraTranslationSpeed * ts, 0, 0));
	}

	bool PerspectiveCameraController::onMouseMoved(const MouseMovedEvent& event)
	{
		SH_PROFILE_FUNCTION();

		if (m_firstMouse)
		{
			m_lastX = event.x;
			m_lastY = event.y;
			m_firstMouse = false;
		}

		float xOffset = event.x - m_lastX;
		float yOffset = event.y - m_lastY;

		m_lastX = event.x;
		m_lastY = event.y;

		xOffset *= m_mouseSensivity;
		yOffset *= m_mouseSensivity;

		m_yaw += xOffset;
		m_pitch -= yOffset;

		if (m_pitch >= 89.0f)
			m_pitch = 89.0f;
		if (m_pitch <= -89.0f)
			m_pitch = -89.0f;

		m_camera.rotate(glm::vec2{ m_pitch, m_yaw });

		return false;
	}

	bool PerspectiveCameraController::onMouseScrolled(const MouseScrolledEvent& e)
	{
		m_fov -= e.yOffset * 3.0f;

		if (m_fov < 1.0f)
			m_fov = 1.0f;
		if (m_fov > 60.0f)
			m_fov = 60.0f;

		m_camera.setProjection(glm::radians(m_fov), m_aspectRatio, 0.1f, 1000.0f);
		return false;
	}

	bool PerspectiveCameraController::onWindowResized(const WindowResizedEvent& e)
	{
		m_aspectRatio = static_cast<float>(e.width) / static_cast<float>(e.height);
		m_camera.setProjection(m_fov, m_aspectRatio, 0.1f, 1000.0f);
		return false;
	}


	OrthoCameraController::OrthoCameraController(float aspectRatio, bool rotation)
		: m_aspectRatio(aspectRatio),
		m_bounds({-m_aspectRatio * m_zoomLevel, m_aspectRatio * m_zoomLevel, -m_zoomLevel, m_zoomLevel}),
		m_camera(m_bounds.left, m_bounds.right, m_bounds.bottom, m_bounds.top), m_rotation(rotation)
	{
		EventDispatcher& dispatcher = EventDispatcher::get();
		dispatcher.addReciever(SH_ON_EVENT_FN(onMouseScrolled, MouseScrolledEvent));
		dispatcher.addReciever(SH_ON_EVENT_FN(onWindowResized, WindowResizedEvent));

		m_camera.setPosition(glm::vec3(0.0f, 0.0f, 0.75f));
		m_cameraTranslationSpeed = m_zoomLevel;
	}

	void OrthoCameraController::onUpdate(Timestep ts)
	{
		if (Input::isKeyPressed(KeyCode::D))
			m_camera.move(glm::vec2(m_cameraTranslationSpeed * ts, 0.0f));
		else if (Input::isKeyPressed(KeyCode::A))
			m_camera.move(glm::vec2(-m_cameraTranslationSpeed * ts, 0.0f));

		if (Input::isKeyPressed(KeyCode::W))
			m_camera.move(glm::vec2(0.0f, m_cameraTranslationSpeed * ts));
		else if (Input::isKeyPressed(KeyCode::S))
			m_camera.move(glm::vec2(0.0f, -m_cameraTranslationSpeed * ts));

		if (m_rotation)
		{
			if (Input::isKeyPressed(KeyCode::Q))
				m_camera.rotate(m_cameraRotationSpeed * ts);
			if (Input::isKeyPressed(KeyCode::E))
				m_camera.rotate(-m_cameraRotationSpeed * ts);
		}
	}

	void OrthoCameraController::calculateView()
	{
		m_bounds = { -m_aspectRatio * m_zoomLevel, m_aspectRatio * m_zoomLevel, -m_zoomLevel, m_zoomLevel };
		m_camera.setProjection(m_bounds.left, m_bounds.right, m_bounds.bottom, m_bounds.top);
	}

	bool OrthoCameraController::onMouseScrolled(const MouseScrolledEvent& e)
	{
		m_zoomLevel -= e.yOffset * 0.25f;
		m_zoomLevel = std::max(m_zoomLevel, 0.25f);
		calculateView();
		return false;
	}

	bool OrthoCameraController::onWindowResized(const WindowResizedEvent& e)
	{
		m_aspectRatio = static_cast<float>(e.width) / static_cast<float>(e.height);
		calculateView();
		return false;
	}
}