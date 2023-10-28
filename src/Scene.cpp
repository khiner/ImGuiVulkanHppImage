#include "Scene.h"

#include <shaderc/shaderc.hpp>

Scene::Scene(const VulkanContext &vc, uint width, uint height) : VC(vc) {
    TC.Extent = vk::Extent2D{width, height};

    static const std::string vert_shader = R"vertexshader(
        #version 450
        #extension GL_ARB_separate_shader_objects : enable

        out gl_PerVertex {
            vec4 gl_Position;
        };

        layout(location = 0) out vec3 fragColor;

        vec2 positions[3] = vec2[](
            vec2(0.0, -0.5),
            vec2(0.5, 0.5),
            vec2(-0.5, 0.5)
        );
        vec3 colors[3] = vec3[](
            vec3(1.0, 0.0, 0.0),
            vec3(0.0, 1.0, 0.0),
            vec3(0.0, 0.0, 1.0)
        );

        void main() {
            gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
            fragColor = colors[gl_VertexIndex];
        }
        )vertexshader";

    static const std::string frag_shader = R"frag_shader(
        #version 450
        #extension GL_ARB_separate_shader_objects : enable

        layout(location = 0) in vec3 fragColor;

        layout(location = 0) out vec4 outColor;

        void main() {
            outColor = vec4(fragColor, 1.0);
        }
        )frag_shader";

    shaderc::CompileOptions options;
    options.SetOptimizationLevel(shaderc_optimization_level_performance);

    shaderc::Compiler compiler;
    const auto vert_shader_module = compiler.CompileGlslToSpv(vert_shader, shaderc_glsl_vertex_shader, "vertex shader", options);
    if (vert_shader_module.GetCompilationStatus() != shaderc_compilation_status_success) {
        throw std::runtime_error(std::format("Failed to compile vertex shader: {}", vert_shader_module.GetErrorMessage()));
    }
    const std::vector<uint> vert_shader_code{vert_shader_module.cbegin(), vert_shader_module.cend()};
    const auto vert_size = std::distance(vert_shader_code.begin(), vert_shader_code.end());
    const vk::ShaderModuleCreateInfo vert_shader_info{{}, vert_size * sizeof(uint), vert_shader_code.data()};
    TC.VertexShaderModule = VC.Device->createShaderModuleUnique(vert_shader_info);

    const auto frag_shader_module = compiler.CompileGlslToSpv(frag_shader, shaderc_glsl_fragment_shader, "fragment shader", options);
    if (frag_shader_module.GetCompilationStatus() != shaderc_compilation_status_success) {
        throw std::runtime_error(std::format("Failed to compile fragment shader: {}", frag_shader_module.GetErrorMessage()));
    }
    const auto frag_shader_code = std::vector<uint>{frag_shader_module.cbegin(), frag_shader_module.cend()};
    const auto frag_size = std::distance(frag_shader_code.begin(), frag_shader_code.end());
    const vk::ShaderModuleCreateInfo frag_shader_info{{}, frag_size * sizeof(uint), frag_shader_code.data()};
    TC.FragmentShaderModule = VC.Device->createShaderModuleUnique(frag_shader_info);

    const vk::PipelineShaderStageCreateInfo vert_shader_stage_info{{}, vk::ShaderStageFlagBits::eVertex, *TC.VertexShaderModule, "main"};
    const vk::PipelineShaderStageCreateInfo frag_shader_stage_info{{}, vk::ShaderStageFlagBits::eFragment, *TC.FragmentShaderModule, "main"};
    const std::vector<vk::PipelineShaderStageCreateInfo> pipeline_shader_stages{vert_shader_stage_info, frag_shader_stage_info};

    const vk::PipelineVertexInputStateCreateInfo vertex_input_info{{}, 0u, nullptr, 0u, nullptr};
    const vk::PipelineInputAssemblyStateCreateInfo input_assemply{{}, vk::PrimitiveTopology::eTriangleList, false};
    const vk::Viewport viewport{0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f};
    const vk::Rect2D scissor{{0, 0}, TC.Extent};
    const vk::PipelineViewportStateCreateInfo viewport_state{{}, 1, &viewport, 1, &scissor};

    const vk::PipelineRasterizationStateCreateInfo rasterizer{
        {}, /*depthClamp*/ false,
        /*rasterizeDiscard*/ false,
        vk::PolygonMode::eFill,
        {},
        /*frontFace*/ vk::FrontFace::eCounterClockwise,
        {},
        {},
        {},
        {},
        1.0f};

    TC.PipelineLayout = VC.Device->createPipelineLayoutUnique({}, nullptr);

    const auto format = vk::Format::eB8G8R8A8Unorm;
    const vk::AttachmentDescription color_attachment{
        {},
        format,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        {},
        {},
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eShaderReadOnlyOptimal};
    const vk::AttachmentReference color_attachment_ref{0, vk::ImageLayout::eColorAttachmentOptimal};
    const vk::PipelineMultisampleStateCreateInfo multisampling{{}, vk::SampleCountFlagBits::e1, false, 1.0};
    const vk::PipelineColorBlendAttachmentState color_blend_attachment{
        {},
        /*srcCol*/ vk::BlendFactor::eOne,
        /*dstCol*/ vk::BlendFactor::eZero,
        /*colBlend*/ vk::BlendOp::eAdd,
        /*srcAlpha*/ vk::BlendFactor::eOne,
        /*dstAlpha*/ vk::BlendFactor::eZero,
        /*alphaBlend*/ vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
    const vk::PipelineColorBlendStateCreateInfo color_blending{{}, false, vk::LogicOp::eCopy, 1, &color_blend_attachment};
    const vk::SubpassDescription subpass{{}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &color_attachment_ref};
    const vk::SubpassDependency subpass_dependency{VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite};
    TC.RenderPass = VC.Device->createRenderPassUnique(vk::RenderPassCreateInfo{{}, 1, &color_attachment, 1, &subpass, 1, &subpass_dependency});

    const vk::GraphicsPipelineCreateInfo pipeline_info{{}, 2, pipeline_shader_stages.data(), &vertex_input_info, &input_assemply, nullptr, &viewport_state, &rasterizer, &multisampling, nullptr, &color_blending, nullptr, *TC.PipelineLayout, *TC.RenderPass, 0};
    TC.GraphicsPipeline = VC.Device->createGraphicsPipelineUnique({}, pipeline_info).value;

    // Create an offscreen image to render the triangle into.
    const vk::ImageCreateInfo image_info{
        {},
        vk::ImageType::e2D,
        format,
        vk::Extent3D{width, height, 1},
        1,
        1,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
        vk::SharingMode::eExclusive,
        0,
        nullptr,
        vk::ImageLayout::eUndefined};

    TC.OffscreenImage = VC.Device->createImageUnique(image_info);
    const auto image_mem_reqs = VC.Device->getImageMemoryRequirements(TC.OffscreenImage.get());
    TC.OffscreenImageMemory = VC.Device->allocateMemoryUnique({image_mem_reqs.size, VC.FindMemoryType(image_mem_reqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)});
    VC.Device->bindImageMemory(TC.OffscreenImage.get(), TC.OffscreenImageMemory.get(), 0);
    TC.OffscreenImageView = VC.Device->createImageViewUnique({{}, TC.OffscreenImage.get(), vk::ImageViewType::e2D, format, vk::ComponentMapping{}, vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}});

    // Create a framebuffer using the offscreen image.
    const uint framebuffer_count = 1;
    const vk::FramebufferCreateInfo framebuffer_info{{}, TC.RenderPass.get(), 1, &(*TC.OffscreenImageView), width, height, 1};
    TC.Framebuffer = VC.Device->createFramebufferUnique(framebuffer_info);
    TC.CommandPool = VC.Device->createCommandPoolUnique({{}, VC.QueueFamily});
    TC.CommandBuffers = VC.Device->allocateCommandBuffersUnique({TC.CommandPool.get(), vk::CommandBufferLevel::ePrimary, framebuffer_count});

    // Image layout transition to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL before rendering.
    const vk::ImageMemoryBarrier barrier{
        {},
        {},
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        TC.OffscreenImage.get(),
        {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
    };

    const auto &command_buffer = TC.CommandBuffers[0];
    command_buffer->begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    command_buffer->pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::DependencyFlags{}, 0, nullptr, 0, nullptr, 1, &barrier
    );
    vk::ClearValue clear_value{};
    const auto render_pass_begin_info = vk::RenderPassBeginInfo{TC.RenderPass.get(), TC.Framebuffer.get(), vk::Rect2D{{0, 0}, TC.Extent}, 1, &clear_value};
    command_buffer->beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
    command_buffer->bindPipeline(vk::PipelineBindPoint::eGraphics, *TC.GraphicsPipeline);
    command_buffer->draw(3, 1, 0, 0);
    command_buffer->endRenderPass();
    command_buffer->end();

    vk::SubmitInfo submit;
    submit.setCommandBuffers(command_buffer.get());
    VC.Queue.submit(submit);
    VC.Device->waitIdle();

    vk::SamplerCreateInfo sampler_info;
    sampler_info.magFilter = vk::Filter::eLinear;
    sampler_info.minFilter = vk::Filter::eLinear;
    // sampler_info.borderColor = vk::BorderColor::eIntOpaqueBlack;
    // sampler_info.compareOp = vk::CompareOp::eAlways;
    // sampler_info.mipmapMode = vk::SamplerMipmapMode::eLinear;

    TC.TextureSampler = VC.Device->createSamplerUnique(sampler_info);
}