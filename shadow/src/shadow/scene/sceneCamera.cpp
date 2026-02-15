#include "shpch.h"
#include "sceneCamera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Shadow
{
	SceneCamera::SceneCamera()
	{
		recalculateProjection();
	}

	void SceneCamera::setPerspective(float fov, float znear, float zfar)
	{
		m_projectionType = ProjectionType::Perspective;
		m_perspective.fov = fov;
		m_perspective.zNear = znear;
		m_perspective.zFar = zfar;
		recalculateProjection();
	}

	void SceneCamera::setOrtho(float size, float nearClip, float farClip)
	{
		m_projectionType = ProjectionType::Orthographic;
		m_ortho.size = size;
		m_ortho.nearClip = nearClip;
		m_ortho.farClip = farClip;
		recalculateProjection();
	}

	void SceneCamera::setViewportSize(uint32_t width, uint32_t height)
	{
		m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
		recalculateProjection();
	}

	void SceneCamera::setProjectionType(ProjectionType type)
	{
		m_projectionType = type;
		recalculateProjection();
	}

	void SceneCamera::recalculateProjection()
	{
		if (m_projectionType == ProjectionType::Perspective)
		{
			m_projection = glm::perspective(glm::radians(m_perspective.fov), m_aspectRatio, m_perspective.zNear, m_perspective.zFar);
			m_projection[1][1] *= -1;
		}
		else
		{
			float left = -m_ortho.size * m_aspectRatio * 0.5f;
			float right = m_ortho.size * m_aspectRatio * 0.5f;
			float bottom = -m_ortho.size * 0.5f;
			float top = m_ortho.size * 0.5f;

			m_projection = glm::ortho(left, right, bottom, top, m_ortho.nearClip, m_ortho.farClip);
			m_projection[1][1] *= -1;
		}
	}
}