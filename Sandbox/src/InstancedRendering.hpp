#pragma once

#include "Shadow.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/ext.hpp>

class InstancedRendering : public Shadow::Layer
{
public:
	InstancedRendering();
	virtual ~InstancedRendering();

	virtual void onUpdate(Shadow::Timestep ts) override; 
	virtual void onRender() override;
	virtual void onImGuiRender() override;
private:
	struct
	{
		Shadow::Ref<Shadow::GraphicsPipeline> attachmentWrite;
		Shadow::Ref<Shadow::GraphicsPipeline> attachmentRead;
	} m_pipelines;

	struct
	{
		Shadow::Ref<Shadow::Shader> attachmentWrite;
		Shadow::Ref<Shadow::Shader> attachmentRead;
	} m_shaders;

	struct
	{

		Shadow::Ref<Shadow::VertexBuffer> attachmentWrite;
		Shadow::Ref<Shadow::VertexBuffer> attachmentRead;
		//Shadow::Ref<Shadow::RenderBuffer> attachmentWrite;
		//Shadow::Ref<Shadow::RenderBuffer> attachmentRead;
	} m_vertexBuffers;

	struct
	{

		Shadow::Ref<Shadow::IndexBuffer> attachmentWrite;
		Shadow::Ref<Shadow::IndexBuffer> attachmentRead;
		//Shadow::Ref<Shadow::RenderBuffer> attachmentWrite;
		//Shadow::Ref<Shadow::RenderBuffer> attachmentRead;
	} m_indexBuffers;

	Shadow::Ref<Shadow::Renderpass> m_renderpass;
	Shadow::Ref<Shadow::UniformBuffer> m_uniformBuffer;
	Shadow::Ref<Shadow::StorageBuffer> m_trs;

	Shadow::Ref<Shadow::VertexBuffer> m_instanceBuffer; 
	//Shadow::Ref<Shadow::RenderBuffer> m_instanceBuffer;

	Shadow::PerspectiveCameraController m_cameraController;

	uint32_t m_instanceCount = 0;
	float m_aspectRatio;

	const float m_cameraSpeed = 35.0f;
	float m_lastX, m_lastY;
	const float m_mouseSensivity = 0.1f;
	float m_yaw = -90.0f, m_pitch = 0.0f;
	bool m_firstMouse = true;
	float m_fov = 45.0f;
};