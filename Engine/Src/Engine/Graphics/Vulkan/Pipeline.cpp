#include "Pipeline.hpp"

#include "Context.hpp"
#include "Device.hpp"
#include "RenderPassCache.hpp"
#include "VertexLayout.hpp"

#include "Engine/Assets/File.hpp"
#include "Engine/Graphics/PipelineCreateInfo.hpp"

#include <array>

namespace engine::graphics::vulkan {

std::expected<Pipeline, PipelineError> Pipeline::Create(Context& context,
                                                        const RenderPass& renderPass,
                                                        const PipelineCreateInfo& info,
                                                        VkDescriptorSetLayout descriptorSetLayout) {
  auto vkDevice = context.GetDevice().Logical();
  VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};
  VkPipeline pipeline{VK_NULL_HANDLE};

  if (auto result = createPipelineLayout_(context, descriptorSetLayout); !result) {
    return std::unexpected(result.error());
  } else {
    pipelineLayout = *result;
  }

  if (auto result = createPipeline_(context, info, renderPass, pipelineLayout); !result) {
    vkDestroyPipelineLayout(vkDevice, pipelineLayout, nullptr);
    return std::unexpected(result.error());
  } else {
    pipeline = *result;
  }

  return Pipeline(context, pipelineLayout, pipeline);
}

Pipeline::Pipeline(Context& context, VkPipelineLayout pipelineLayout, VkPipeline pipeline)
    : context_{context}, pipelineLayout_{pipelineLayout}, pipeline_{pipeline} {}

Pipeline::~Pipeline() {
  const auto vkDevice = context_.GetDevice().Logical();

  vkDestroyPipeline(vkDevice, pipeline_, nullptr);
  vkDestroyPipelineLayout(vkDevice, pipelineLayout_, nullptr);
}

Pipeline::Pipeline(Pipeline&& other) noexcept
    : context_{other.context_}, pipelineLayout_{other.pipelineLayout_}, pipeline_{other.pipeline_} {
  other.pipelineLayout_ = VK_NULL_HANDLE;
  other.pipeline_ = VK_NULL_HANDLE;
}

VkPipelineLayout Pipeline::PipelineLayout() const noexcept {
  return pipelineLayout_;
}

VkPipeline Pipeline::Handle() const noexcept {
  return pipeline_;
}

std::expected<VkPipelineLayout, PipelineError>
Pipeline::createPipelineLayout_(const Context& context, VkDescriptorSetLayout descriptorLayout) noexcept {
  const auto vkDevice = context.GetDevice().Logical();
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &descriptorLayout,
      .pushConstantRangeCount = 0,
  };

  VkPipelineLayout layout{VK_NULL_HANDLE};
  if (vkCreatePipelineLayout(vkDevice, &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS) {
    return std::unexpected(PipelineError::FailedPipelineLayoutCreation);
  }
  return layout;
}

std::expected<VkPipeline, PipelineError> Pipeline::createPipeline_(const Context& context,
                                                                   const PipelineCreateInfo& info,
                                                                   const RenderPass& renderPass,
                                                                   VkPipelineLayout pipelineLayout) noexcept {

  const auto& device = context.GetDevice();
  const auto vkDevice = device.Logical();

  auto vertShaderCode = assets::ReadBinaryFile(info.VertexShaderFile);
  auto fragShaderCode = assets::ReadBinaryFile(info.FragmentShaderFile);

  struct ShaderModules {
    VkDevice device{VK_NULL_HANDLE};
    VkShaderModule vert{VK_NULL_HANDLE};
    VkShaderModule frag{VK_NULL_HANDLE};

    explicit ShaderModules(VkDevice d) noexcept : device{d} {}

    ~ShaderModules() {
      if (vert)
        vkDestroyShaderModule(device, vert, nullptr);
      if (frag)
        vkDestroyShaderModule(device, frag, nullptr);
    }
  };
  ShaderModules shaderModules{vkDevice};

  if (auto result = createShaderModule_(context, vertShaderCode); !result) {
    return std::unexpected(result.error());
  } else {
    shaderModules.vert = *result;
  }

  if (auto result = createShaderModule_(context, fragShaderCode); !result) {
    return std::unexpected(result.error());
  } else {
    shaderModules.frag = *result;
  }

  VkPipelineShaderStageCreateInfo vertShaderStageInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = shaderModules.vert,
      .pName = "main",
  };

  VkPipelineShaderStageCreateInfo fragShaderStageInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = shaderModules.frag,
      .pName = "main",
  };

  std::array shaderStages = {vertShaderStageInfo, fragShaderStageInfo};

  auto bindingDescription = VertexLayout::GetBindingDescription();
  auto attributeDescriptions = VertexLayout::GetAttributeDescriptions();

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &bindingDescription,
      .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
      .pVertexAttributeDescriptions = attributeDescriptions.data(),
  };

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE,
  };

  VkPipelineViewportStateCreateInfo viewportState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .scissorCount = 1,
  };

  VkPipelineRasterizationStateCreateInfo rasterizer{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_BACK_BIT,
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .depthBiasEnable = VK_FALSE,
      .lineWidth = 1.0f,
  };

  VkPipelineMultisampleStateCreateInfo multisampling{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = device.MsaaSamples(),
      .sampleShadingEnable = VK_FALSE,
  };

  VkPipelineColorBlendAttachmentState colorBlendAttachment{
      .blendEnable = VK_FALSE,
      .colorWriteMask =
          VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
  };

  VkPipelineColorBlendStateCreateInfo colorBlending{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOpEnable = VK_FALSE,
      .logicOp = VK_LOGIC_OP_COPY,
      .attachmentCount = 1,
      .pAttachments = &colorBlendAttachment,
      .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f},
  };

  constexpr std::array<VkDynamicState, 2> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
      .pDynamicStates = dynamicStates.data(),
  };

  VkPipelineDepthStencilStateCreateInfo depthStencil{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = VK_TRUE,
      .depthWriteEnable = VK_TRUE,
      .depthCompareOp = VK_COMPARE_OP_LESS,
      .depthBoundsTestEnable = VK_FALSE,
      .stencilTestEnable = VK_FALSE,
  };

  VkGraphicsPipelineCreateInfo pipelineInfo{
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount = static_cast<uint32_t>(shaderStages.size()),
      .pStages = shaderStages.data(),
      .pVertexInputState = &vertexInputInfo,
      .pInputAssemblyState = &inputAssembly,
      .pViewportState = &viewportState,
      .pRasterizationState = &rasterizer,
      .pMultisampleState = &multisampling,
      .pDepthStencilState = &depthStencil,
      .pColorBlendState = &colorBlending,
      .pDynamicState = &dynamicState,
      .layout = pipelineLayout,
      .renderPass = renderPass.Handle,
      .subpass = 0,
  };

  VkPipeline pipeline{VK_NULL_HANDLE};
  if (vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
    return std::unexpected(PipelineError::FailedPipelineCreation);
  }

  return pipeline;
}

std::expected<VkShaderModule, PipelineError> Pipeline::createShaderModule_(const Context& context,
                                                                           const std::vector<char>& code) noexcept {
  const auto device = context.GetDevice().Logical();

  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

  VkShaderModule shaderModule{};
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
    return std::unexpected(PipelineError::FailedShaderCompile);
  }
  return shaderModule;
}

} // namespace engine::graphics::vulkan
