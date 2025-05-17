#include "ParticleSystem.hpp"

#include "Shadow/Vulkan/VulkanDevice.hpp"
#include "Shadow/Vulkan/VulkanContext.hpp"
#include "Shadow/Vulkan/VulkanBuffer.hpp"

#include <imgui/imgui.h>
#include <GLFW/glfw3.h>

#include <random>

using namespace Shadow;

static const uint32_t s_particleCount = 256 * 1024;

void ParticleSystem::onAttach()
{
    std::string assetsPath = "C:/dev/Shadow/Shadow/assets/";

    m_textures.particle = Texture2D::create(assetsPath + "textures/cat.png");
    m_textures.gradient = Texture2D::create(assetsPath + "textures/cat.png");
    m_graphics.shader = Shader::create("particle", assetsPath + "shaders/particle.vert.spv", assetsPath+"shaders/particle.frag.spv");

    m_graphics.shader->writeDescriptorSet("samplerColorMap", m_textures.particle);
    m_graphics.shader->writeDescriptorSet("samplerGradientRamp", m_textures.gradient);

    VertexAttribute particlePos(VertexAttribType::Vec2f);
    particlePos.offset = offsetof(Particle, pos);

    VertexAttribute particleGradientPos(VertexAttribType::Vec4f);
    particleGradientPos.offset = offsetof(Particle, gradientPos);

    VertexInput particleInput({ particlePos, particleGradientPos });

    FramebufferInfo info{};
    ShEngine::get().getWindow().getFramebufferSize(info.width, info.height);

    RenderpassConfig renderpassConfig{};
    renderpassConfig.framebufferInfo = info;
    renderpassConfig.subpassCount = 0;
    renderpassConfig.firstRenderpass = true;
    renderpassConfig.swapchainTarget = true;
    m_graphics.renderpass = Renderpass::create(renderpassConfig);

    GraphicsPipeStates states{};
    states.primitiveTopology = PrimitiveTopology::Point;
    states.polygonMode = PolygonMode::Fill;
    states.frontFace = FrontFace::CounterClockwise;
    states.primitiveRestartEnable = false;
    states.cullMode = CullMode::Back;
    states.lineWidth = 1.0f;
    states.blendState.blendEnable = true;
    states.blendState.srcColorBlendFactor = BlendFactor::One;  
    states.blendState.dstColorBlendFactor = BlendFactor::One;
    states.blendState.srcAlphaBlendFactor = BlendFactor::SrcAlpha;
    states.blendState.dstAlphaBlendFactor = BlendFactor::DstAlpha;

    GraphicsPipeConfiguration graphicsPipeConfig{};
    graphicsPipeConfig.subpass = 0;
    graphicsPipeConfig.shader = m_graphics.shader;
    graphicsPipeConfig.renderpass = m_graphics.renderpass;
    graphicsPipeConfig.vertexInput = &particleInput;
    graphicsPipeConfig.states = states;
    m_graphics.pipeline = GraphicsPipeline::create(graphicsPipeConfig);

    m_compute.shader = Shader::create("particleComp", assetsPath + "shaders/particle.comp.spv");
    m_compute.pipeline = ComputePipeline::create(m_compute.shader);
    m_compute.uniformBuffer = UniformBuffer::create(sizeof(Compute::UniformData));

    // Initial particle positions on a circle
    const Window& window = ShEngine::get().getWindow();

    std::default_random_engine rndEngine((unsigned)time(nullptr));
    std::uniform_real_distribution<float> rndDist(-1.0f, 1.0f);

    // Initial particle positions
    std::vector<Particle> particleBuffer(s_particleCount);
    for (auto& particle : particleBuffer)
    {
        particle.pos = glm::vec2(rndDist(rndEngine), rndDist(rndEngine));
        particle.vel = glm::vec2(0.0f);
        particle.gradientPos.x = particle.pos.x / 2.0f;
    }
    m_particles = StorageBuffer::create(particleBuffer.data(), sizeof(Particle) * s_particleCount, sizeof(Particle), BufferUsage::VertexBuffer);

    m_compute.shader->writeDescriptorSet("Pos", m_particles, false);
    m_compute.shader->writeDescriptorSet("ubo", m_compute.uniformBuffer);
}

void ParticleSystem::onDetach()
{
}

void ParticleSystem::onUpdate(Shadow::Timestep ts)
{
    auto& win = ShEngine::get().getWindow();

    m_compute.uniformData.deltaT = ts * 2.5f;

    float normalizedMx = (m_mousePos.x - static_cast<float>(win.getWidth() / 2)) / static_cast<float>(win.getWidth() / 2);
    float normalizedMy = (m_mousePos.y - static_cast<float>(win.getHeight() / 2)) / static_cast<float>(win.getHeight() / 2);
    m_compute.uniformData.dstX = normalizedMx;
    m_compute.uniformData.dstY = normalizedMy;

    m_compute.uniformBuffer->setData_RT(&m_compute.uniformData, sizeof(Compute::uniformData));
}

void ParticleSystem::onRender()
{
    auto& win = ShEngine::get().getWindow();
    Renderer::setViewport(0, 0, (float)win.getWidth(), (float)win.getHeight());

    // compute pass
    {
        Renderer::beginCompute(m_compute.pipeline, 0);
        Renderer::dispatch(s_particleCount / 256, 1, 1);
        Renderer::submitCompute(PipelineStages::VertexInput);
    }

    // graphics pass
    {
        Renderer::beginRenderPass(m_graphics.pipeline);
        Renderer::draw(m_particles);
        Renderer::endRenderPass();
    }
}

void ParticleSystem::onImGuiRender()
{
    ImGui::Begin("Compute shader");
    ImGui::End();

    float frameRate = Shadow::ShEngine::get().getFrameRate();
    uint32_t fps = static_cast<uint32_t>(1000.0f / frameRate);

    ImGui::Begin("Renderer state");
    ImGui::Text("FPS: %u", fps);
    ImGui::Text("Frame time: %f ms", frameRate);
    ImGui::End();
}

bool ParticleSystem::onMouseMove(const Shadow::MouseMovedEvent& e)
{
    m_mousePos = { e.x,e.y };
    return false;
}
