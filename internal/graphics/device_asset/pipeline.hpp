#pragma once

#include <memory>
#include <stdexcept>
#include <vector>

#include <glad/vulkan.h>

#include "utils/binary.hpp"

#include "device.hpp"
#include "shader.hpp"
#include "vertex.hpp"

#include "resourceloader.hpp"

class Pipeline
{
  private:
    const LogicalDevice &device;

  private:
    // shader containing all the pipeline stage information (vertex shader stage, fragment shader stage, ...)
    std::shared_ptr<Shader> shaderProgram;

    VkDescriptorSetLayout setlayout;
    std::vector<VkDescriptorSet> sets;

    VkPipelineLayout layout;

    VkPipeline handle;

  public:
    Pipeline() = delete;
    Pipeline(const LogicalDevice &device, const VkRenderPass &renderPass, const char *shaderName,
             const VkExtent2D &viewportExtent)
        : device(device)
    {
        shaderProgram = ResourceLoader::load<Shader>(ResourceLoadInfoI{
            .deviceptr = &device,
            .filepath = std::filesystem::path("shaders/" + std::string(shaderName)),
        });

        auto createPipeline = [this, &renderPass, &device, &viewportExtent]() {
            // vertex buffer (enabling the binding for our Vertex structure)
            auto binding = Vertex::getBindingDescription();
            auto attribs = Vertex::getAttributeDescription();
            VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                .vertexBindingDescriptionCount = 1,
                .pVertexBindingDescriptions = &binding,
                .vertexAttributeDescriptionCount = static_cast<uint32_t>(attribs.size()),
                .pVertexAttributeDescriptions = attribs.data()};

            // draw mode
            VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                .primitiveRestartEnable = VK_FALSE};

            // viewport
            VkViewport viewport = {.x = 0.f,
                                   .y = 0.f,
                                   .width = static_cast<float>(viewportExtent.width),
                                   .height = static_cast<float>(viewportExtent.height),
                                   .minDepth = 0.f,
                                   .maxDepth = 1.f};

            VkRect2D scissor = {.offset = {0, 0}, .extent = viewportExtent};

            VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                .viewportCount = 1,
                .pViewports = &viewport,
                .scissorCount = 1,
                .pScissors = &scissor,
            };

            // rasterizer
            VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                .depthClampEnable = VK_FALSE,
                .rasterizerDiscardEnable = VK_FALSE,
                .polygonMode = VK_POLYGON_MODE_FILL,
                .cullMode = VK_CULL_MODE_BACK_BIT,
                .frontFace = VK_FRONT_FACE_CLOCKWISE,
                .depthBiasEnable = VK_FALSE,
                .depthBiasConstantFactor = 0.f,
                .depthBiasClamp = 0.f,
                .depthBiasSlopeFactor = 0.f,
                .lineWidth = 1.f};

            // multisampling, anti-aliasing
            VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
                .sampleShadingEnable = VK_FALSE,
                .minSampleShading = 1.f,
                .pSampleMask = nullptr,
                .alphaToCoverageEnable = VK_FALSE,
                .alphaToOneEnable = VK_FALSE};

            // color blending
            VkPipelineColorBlendAttachmentState colorBlendAttachment = {
                .blendEnable = VK_TRUE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                                  VK_COLOR_COMPONENT_A_BIT};

            VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                .logicOpEnable = VK_FALSE,
                .logicOp = VK_LOGIC_OP_COPY,
                .attachmentCount = 1,
                .pAttachments = &colorBlendAttachment,
                .blendConstants = {0.f, 0.f, 0.f, 0.f}};

            std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

            VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
                .pDynamicStates = dynamicStates.data()};

            // uniforms
            VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {.sType =
                                                                       VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                                                                   .setLayoutCount = 0,
                                                                   .pSetLayouts = nullptr,
                                                                   .pushConstantRangeCount = 0,
                                                                   .pPushConstantRanges = nullptr};

            if (vkCreatePipelineLayout(device.handle, &pipelineLayoutCreateInfo, nullptr, &layout) != VK_SUCCESS)
                throw std::runtime_error("Failed to create pipeline layout");

            auto shaderModule = std::dynamic_pointer_cast<GPUShader>(shaderProgram->local);
            auto shaderStageCreateInfo = shaderModule->getShaderStageCreateInfo();
            VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                // shader stage
                .stageCount = static_cast<uint32_t>(shaderStageCreateInfo.size()),
                .pStages = shaderStageCreateInfo.data(),
                // fixed function stage
                .pVertexInputState = &vertexInputCreateInfo,
                .pInputAssemblyState = &inputAssemblyCreateInfo,
                .pViewportState = &viewportStateCreateInfo,
                .pRasterizationState = &rasterizerCreateInfo,
                .pMultisampleState = &multisamplingCreateInfo,
                .pDepthStencilState = nullptr,
                .pColorBlendState = &colorBlendCreateInfo,
                .pDynamicState = &dynamicStateCreateInfo,
                // TODO : descriptor sets
                // pipeline layout
                .layout = layout,
                // render pass
                .renderPass = renderPass,
                .subpass = 0,
                .basePipelineHandle = VK_NULL_HANDLE,
                .basePipelineIndex = -1,
            };

            if (vkCreateGraphicsPipelines(device.handle, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr,
                                          &this->handle) != VK_SUCCESS)
                throw std::runtime_error("Failed to create graphics pipeline");
        };

        if (shaderProgram->local->loaded.test())
            createPipeline();
        else
            Utils::GlobalThreadPool::addTask(createPipeline, false); // TODO : multithread
    }
    ~Pipeline()
    {
        vkDestroyPipeline(device.handle, handle, nullptr);
        vkDestroyPipelineLayout(device.handle, layout, nullptr);
    }
};