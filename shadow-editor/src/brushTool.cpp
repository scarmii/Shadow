#include "brushTool.h"

#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui.h>

using namespace Shadow;

PaintTool::PaintTool()
{
	m_shader = Shader::create("paint_compute_shader", "C:/dev/shadow/shadow-editor/assets/shaders/brushTool.comp.spv");
	m_computePipe = ComputePipeline::create(m_shader);
	m_computePipe->setName("paint_compute_pipeline");
	setShaderInputImage();

	m_ubo = UniformBuffer::create(sizeof(PaintTool::UboData));
	m_shader->setInput("u_ubo", m_ubo);

	m_brushPropsUbo = UniformBuffer::create(sizeof(BrushProperties));
	m_shader->setInput("u_props", m_brushPropsUbo);
}

void PaintTool::onRender()
{
	m_ubo->setData_RT(&m_uboData, m_ubo->getSize());
	m_brushPropsUbo->setData_RT(&m_properties);

	auto& cmdBuffer = VulkanContext::getDevice()->getCmdBuffer();
	auto& texture = Renderer2D::getColorOutput();

	cmdBuffer->transitionImageLayout(texture,
		ImageLayout::ShaderReadOnlyOptimal, ImageLayout::General,
		PipelineStages::ColorAttachmentOutput, PipelineStages::ComputeShader,
		AccessFlags::ColorAttachmentWrite, AccessFlags::ShaderWrite);

	cmdBuffer->beginComputePass(m_computePipe);
	cmdBuffer->dispatch(texture->getWidth() / 16.0f, texture->getHeight() / 16.0f, 1.0f);
	cmdBuffer->endComputePass();

	cmdBuffer->transitionImageLayout(texture,
		ImageLayout::General, ImageLayout::ShaderReadOnlyOptimal,
		PipelineStages::ComputeShader, PipelineStages::FragmentShader,
		AccessFlags::ShaderWrite, AccessFlags::ShaderRead);
}

void PaintTool::onImGuiRender()
{
	ImGui::Begin("Paint");
	{
		ImGui::Text("Color"); ImGui::SameLine();
		ImGui::ColorEdit4("##label", glm::value_ptr(m_properties.color));

		ImGui::Text("Width"); ImGui::SameLine();
		ImGui::DragFloat("##label", &m_properties.width, 0.25f, 0.0f, 35.0f);

		if (ImGui::BeginMenu("Brush Shape"))
		{
			if (ImGui::MenuItem("Circle")) { m_properties.circle = 1; }
			if (ImGui::MenuItem("Square")) { m_properties.circle = 0; }
			ImGui::EndMenu();
		}

		if (ImGui::Button("Clear"))
		{
			auto& cmdBuffer = ShApp::get().getImGuiLayer()->getCommandBuffer();
			auto& image = Renderer2D::getColorOutput();
			cmdBuffer->clearColor(image, ImageLayout::ShaderReadOnlyOptimal, PipelineStages::FragmentShader, AccessFlags::ShaderRead, m_clearColor);
		}

		ImGui::ColorEdit4("Clear Color", glm::value_ptr(m_clearColor));
	}
	ImGui::End();
}

void PaintTool::onViewportRender()
{
	m_uboData.mousePos = { -1.0f,-1.0f};
	if (ImGui::IsWindowHovered())
	{
		m_uboData.mousePos = ImGui::GetMousePos();
		m_uboData.mousePos.x -= ImGui::GetWindowPos().x;
		m_uboData.mousePos.y -= ImGui::GetWindowPos().y;
	}


	ImVec2 mousePos = ImGui::GetMousePos();
	if (m_properties.circle)
	{
		ImGui::GetForegroundDrawList()->AddCircle(mousePos, m_properties.width,
			ImColor(m_properties.color[0], m_properties.color[1], m_properties.color[2]), 0, 1);
	}
	else
	{
		ImGui::GetForegroundDrawList()->AddRect(ImVec2{ mousePos.x - m_properties.width, mousePos.y - m_properties.width },
			ImVec2{ mousePos.x + m_properties.width, mousePos.y + m_properties.width },
			ImColor(m_properties.color.r, m_properties.color.g, m_properties.color.b));
	}
}

void PaintTool::setShaderInputImage()
{
	m_shader->setInput("outImage", Renderer2D::getColorOutput());
}
