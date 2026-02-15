#pragma once

#include "shadow/renderer/camera.h"

namespace Shadow
{
	class SceneCamera : public Camera
	{
	public:
		enum class ProjectionType {Perspective = 0, Orthographic = 1};
	public:
		SceneCamera();
		virtual ~SceneCamera() = default;

		void setPerspective(float fov, float znear, float zfar);
		void setOrtho(float size, float nearClip, float farClip);
		void setViewportSize(uint32_t width, uint32_t height);

		float getOrthoSize() const { return m_ortho.size; }
		inline float getOrthoNearClip() const { return m_ortho.nearClip; }
		inline float getOrthoFarClip() const { return m_ortho.farClip; }
		void setOrthoSize(float size) { m_ortho.size = size; recalculateProjection(); }
		void setOrthoNearClip(float nearClip) { m_ortho.nearClip = nearClip; recalculateProjection(); }
		void setOrthoFarClip(float farClip) { m_ortho.farClip = farClip; recalculateProjection(); }

		inline float getPerspectiveVerticalFOV() const { return m_perspective.fov; }
		inline float getPerspectiveNearClip() const { return m_perspective.zNear; }
		inline float getPerspectiveFarClip() const { return m_perspective.zFar; }
		void setPerspectiveVerticalFOV(float fov) { m_perspective.fov = fov; recalculateProjection(); }
		void setPerspectiveNearClip(float nearClip) { m_perspective.zNear = nearClip; recalculateProjection(); }
		void setPerspectiveFarClip(float farClip) { m_perspective.zFar = farClip; recalculateProjection(); }

		inline ProjectionType getProjectionType() const { return m_projectionType; }
		void setProjectionType(ProjectionType type);
	private:
		void recalculateProjection();
	private:
		ProjectionType m_projectionType = ProjectionType::Orthographic;

		struct Ortho
		{
			float size = 10.0f;
			float nearClip = -1.0f;
			float farClip = 1.0f;
		} m_ortho;

		struct Perspective
		{
			float fov = 45.0f; // in degrees
			float zNear = 0.01f;
			float zFar = 1000.0f;
		} m_perspective;

		float m_aspectRatio = 1.0f;
	};
}