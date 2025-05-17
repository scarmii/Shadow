#pragma once

#include <glm/glm.hpp>

namespace Shadow
{
	// PerspectiveCamera is commonly used for 3D world
	// OrthoCamera is commonly used for 2D world

	class PerspectiveCamera
	{
	public:
		PerspectiveCamera(float fov, float aspectRatio, float znear, float zfar);

		void setProjection(float fov, float aspectRatio, float znear, float zfar);

		inline const glm::vec3 getPosition() const { return m_position; }
		inline void setPosition(const glm::vec3& position) { m_position = position; recalculateViewMatrix(); }

        void move(const glm::vec3& velocity);
		void rotate(const glm::vec2& axis);
		void rotate(float pitch, float yaw);

		inline const glm::mat4& getViewMatrix() const { return m_viewMatrix; }
		inline const glm::mat4& getProjectionMatrix() const { return m_projectionMatrix; }
		inline const glm::mat4& getVPMatrix() const { return m_viewProjectionMatrix; }
	private:
		void recalculateViewMatrix();
	private:
		glm::mat4 m_projectionMatrix;
		glm::mat4 m_viewMatrix;
		glm::mat4 m_viewProjectionMatrix;

		glm::vec3 m_position;
		glm::vec3 m_right; // local cam space (x-axis)
		glm::vec3 m_up;	   // local cam space (y-axis)
		glm::vec3 m_front; // local cam space (z-axis)
	};


	class OrthoCamera
	{
	public:
		OrthoCamera(float left, float right, float bottom, float top, float zNear = -1.0f, float zFar = 1.0f);

		void setProjection(float left, float right, float bottom, float top, float zNear = -1.0f, float zFar = 1.0f);

		inline const glm::vec3 getPosition() const { return m_position; }
		inline void setPosition(const glm::vec3& position) { m_position = position; recalculateViewMatrix(); }

		inline float getRotation() const { return m_rotation; }
		inline void rotate(float rotation) { m_rotation += rotation; recalculateViewMatrix(); }

		void move(const glm::vec2& velocity);

		inline const glm::mat4& getViewMatrix() const { return m_viewMatrix; }
		inline const glm::mat4& getProjectionMatrix() const { return m_projectionMatrix; }
		inline const glm::mat4& getVPMatrix() const { return m_viewProjectionMatrix; }
	private:
		void recalculateViewMatrix();
	private:
		glm::mat4 m_projectionMatrix;
		glm::mat4 m_viewMatrix;
		glm::mat4 m_viewProjectionMatrix;

		float m_rotation = 0.0f;
		glm::vec3 m_position = { 0.0f, 0.0f, 0.0f };
	};
}