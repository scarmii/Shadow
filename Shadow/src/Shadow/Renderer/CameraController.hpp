#pragma once

#include "Shadow/Renderer/Camera.hpp"
#include "Shadow/Core/Timestep.hpp"
#include "Shadow/Events/EventTypes.hpp"

namespace Shadow
{
	class PerspectiveCameraController 
	{
	public:
		PerspectiveCameraController(float fov, float aspectRatio);

		void onUpdate(Timestep ts);

		inline void setCameraTranslationSpeed(float speed) { m_cameraTranslationSpeed = speed; }

		inline const PerspectiveCamera& getCamera() const { return m_camera; }
	private:
		bool onMouseMoved(const Shadow::MouseMovedEvent& event);
		bool onMouseScrolled(const MouseScrolledEvent& e);
		bool onWindowResized(const WindowResizedEvent& e);
	private:
		float m_aspectRatio;
		float m_zoomLevel = 1.0f;
		float m_lastX, m_lastY;
		PerspectiveCamera m_camera;

		float m_cameraTranslationSpeed = 1.0f;
		const float m_mouseSensivity = 0.1f;
		float m_yaw = -90.0f, m_pitch = 0.0f;
		bool m_firstMouse = true;
		float m_fov = 45.0f;
	};

	class OrthoCameraController
	{
	public:
		OrthoCameraController(float aspectRatio, bool rotation = false);

		void onUpdate(Timestep ts);

		inline void setCameraTranslationSpeed(float speed) { m_cameraTranslationSpeed = speed; }
		inline void setCameraRotationSpeed(float speed) { m_cameraRotationSpeed = speed; }

		inline const OrthoCamera& getCamera() const { return m_camera; }
	private: 
		bool onMouseScrolled(const MouseScrolledEvent& e);
		bool onWindowResized(const WindowResizedEvent& e);
	private:
		float m_aspectRatio;
		float m_zoomLevel = 1.0f;
		OrthoCamera m_camera;

		bool m_rotation;
		float m_cameraTranslationSpeed, m_cameraRotationSpeed = 1.0f;
	};
}