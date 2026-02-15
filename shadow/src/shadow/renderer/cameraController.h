#pragma once

#include "shadow/renderer/camera.h"
#include "shadow/core/timestep.h"
#include "shadow/events/eventTypes.h"

namespace Shadow
{
	class PerspectiveCameraController 
	{
	public:
		PerspectiveCameraController(float fov, float aspectRatio);

		void onUpdate(Timestep ts);

		inline void setCameraTranslationSpeed(float speed) { m_cameraTranslationSpeed = speed; }

		inline PerspectiveCamera& getCamera() { return m_camera; }
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

	struct OrthoCameraBounds
	{
		float left, right;
		float bottom, top;

		inline float getWidth() const { return right - left; }
		inline float getHeight() const { return top - bottom; }
	};

	class OrthoCameraController
	{
	public:
		OrthoCameraController(float aspectRatio, bool rotation = false);

		void onUpdate(Timestep ts);

		void setCameraTranslationSpeed(float speed) { m_cameraTranslationSpeed = speed; }
		void setCameraRotationSpeed(float speed) { m_cameraRotationSpeed = speed; }
		void setZoomLevel(float level) { m_zoomLevel = level; calculateView(); }

		inline const OrthoCamera& getCamera() const { return m_camera; }
		inline const OrthoCameraBounds& getBounds() const { return m_bounds; }
		inline float getZoomLevel() const { return m_zoomLevel; }
	private: 
		void calculateView();

		bool onMouseScrolled(const MouseScrolledEvent& e);
		bool onWindowResized(const WindowResizedEvent& e);
	private:
		float m_aspectRatio;
		float m_zoomLevel = 1.0f;

		OrthoCameraBounds m_bounds;
		OrthoCamera m_camera;

		bool m_rotation;
		float m_cameraTranslationSpeed, m_cameraRotationSpeed = 1.0f;
	};
}