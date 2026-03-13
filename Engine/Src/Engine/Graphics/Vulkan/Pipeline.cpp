#include "Pipeline.hpp"

#include "Context.hpp"
#include "Device.hpp"
#include "RenderPassCache.hpp"
#include "VertexLayout.hpp"

#include "Engine/Assets/File.hpp"
#include "Engine/Graphics/Mesh.hpp"
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
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &descriptorLayout;

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

  const auto vkDevice = context.GetDevice().Logical();

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

  VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = shaderModules.vert;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = shaderModules.frag;
  fragShaderStageInfo.pName = "main";

  std::array shaderStages = {vertShaderStageInfo, fragShaderStageInfo};

  auto bindingDescription = VertexLayout::GetBindingDescription();
  auto attributeDescriptions = VertexLayout::GetAttributeDescriptions();

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;

  constexpr std::array<VkDynamicState, 2> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates = dynamicStates.data();

  VkPipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = VK_TRUE;
  depthStencil.depthWriteEnable = VK_TRUE;
  depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.stencilTestEnable = VK_FALSE;

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages.data();
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = renderPass.Handle;
  pipelineInfo.subpass = 0;

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