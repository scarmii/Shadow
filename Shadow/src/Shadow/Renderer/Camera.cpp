#include "shpch.hpp"
#include "Shadow/Renderer/Camera.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/ext.hpp>

namespace Shadow
{
	PerspectiveCamera::PerspectiveCamera(float fov, float aspectRatio, float znear, float zfar)
		: m_projectionMatrix(glm::perspective(fov, aspectRatio, znear, zfar)), m_viewMatrix(1.0f)
	{
		m_projectionMatrix[1][1] *= -1;
		m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;

		m_front = glm::vec3(0.0f, 0.0f, -1.0f);
		m_up = glm::vec3(0.0f, 1.0f, 0.0f);
		m_right = -glm::normalize(glm::cross(m_front, m_up));

		m_position = m_right + m_up + m_front;
	}

	void PerspectiveCamera::move(const glm::vec3& velocity)
	{
		m_position += m_right * velocity.x + m_up * velocity.y + m_front * velocity.z;
		recalculateViewMatrix();
	}

	void PerspectiveCamera::rotate(const glm::vec2& axis)
	{ 
		glm::vec3 forwardDir{};
		forwardDir.x = cos(glm::radians(axis.y));
		forwardDir.y = sin(glm::radians(axis.x));
		forwardDir.z = sin(glm::radians(axis.y)) * cos(glm::radians(axis.x));
		m_front = glm::normalize(forwardDir);
		m_right = -glm::normalize(glm::cross(m_front, m_up));
		recalculateViewMatrix();
	}

	void PerspectiveCamera::rotate(float pitch, float yaw)
	{
		glm::vec3 forwardDir{};
		forwardDir.x = cos(glm::radians(yaw));
		forwardDir.y = sin(glm::radians(pitch));
		forwardDir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		m_front = glm::normalize(forwardDir);
		m_right = -glm::normalize(glm::cross(m_front, m_up));
		recalculateViewMatrix();
	}

	void PerspectiveCamera::setProjection(float fov, float aspectRatio, float znear, float zfar)
	{
		m_projectionMatrix = glm::perspective(fov, aspectRatio, znear, zfar);
		m_projectionMatrix[1][1] *= -1;
		recalculateViewMatrix();
	}

	void PerspectiveCamera::recalculateViewMatrix()
	{
		m_viewMatrix = glm::lookAt(m_position, m_position + m_front, m_up);
		m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
	}


	OrthoCamera::OrthoCamera(float left, float right, float bottom, float top, float zNear, float zFar)
		: m_projectionMatrix(glm::ortho(left, right, bottom, top, zNear, zFar)), m_viewMatrix(1.0f)
	{
		m_projectionMatrix[1][1] *= -1;
		m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
	}

	void OrthoCamera::setProjection(float left, float right, float bottom, float top, float zNear, float zFar)
	{
		SH_PROFILE_FUNCTION();

		m_projectionMatrix = glm::ortho(left, right, bottom, top, zNear, zFar);
		m_projectionMatrix[1][1] *= -1;
		m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
	}

	void OrthoCamera::move(const glm::vec2& velocity)
	{
		m_position.x += velocity.x;
		m_position.y += velocity.y;
		recalculateViewMatrix();
	}

	void OrthoCamera::recalculateViewMatrix()
	{
		SH_PROFILE_FUNCTION();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_position) *
			glm::rotate(glm::mat4(1.0f), m_rotation, glm::vec3(0, 0, 1));

		m_viewMatrix = glm::inverse(transform);
		m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
	}
}