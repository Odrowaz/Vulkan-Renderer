#include "VulkanPipeline.h"
#include "Common.h"
#include <fstream>
#include <glm/glm.hpp>
#include <vector>

VulkanPipeline::VulkanPipeline(VulkanContext &InContext,
                               std::string VertexShaderPath,
                               std::string FragmentShaderPath, int MaxViews)
    : Device(InContext.GetDevice()) {
  VkShaderModule VertModule = CreateShaderModule(VertexShaderPath);
  VkShaderModule FragModule = CreateShaderModule(FragmentShaderPath);

  VkPipelineShaderStageCreateInfo ShaderStages[2]{};
  ShaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  ShaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  ShaderStages[0].module = VertModule;
  ShaderStages[0].pName = "main";
  ShaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  ShaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  ShaderStages[1].module = FragModule;
  ShaderStages[1].pName = "main";

  VkVertexInputBindingDescription BindingDesc{};
  BindingDesc.binding = 0; // slot index matches vkCmdBindVertexBuffers()
  BindingDesc.stride =
      8 * sizeof(float); // pos(3) + normal(3) + uv(2) per vertex
  BindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkVertexInputAttributeDescription AttrDescs[3]{};
  AttrDescs[0].binding = 0;
  AttrDescs[0].location = 0; // Position
  AttrDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  AttrDescs[0].offset = 0;

  AttrDescs[1].binding = 0;
  AttrDescs[1].location = 1; // Normal
  AttrDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  AttrDescs[1].offset = 3 * sizeof(float);

  AttrDescs[2].binding = 0;
  AttrDescs[2].location = 2; // Uvs
  AttrDescs[2].format = VK_FORMAT_R32G32_SFLOAT;
  AttrDescs[2].offset = 6 * sizeof(float);

  VkPipelineVertexInputStateCreateInfo VertexInput{};
  VertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  VertexInput.vertexBindingDescriptionCount = 1;
  VertexInput.pVertexBindingDescriptions = &BindingDesc;
  VertexInput.vertexAttributeDescriptionCount = 3;
  VertexInput.pVertexAttributeDescriptions = AttrDescs;

  VkPipelineInputAssemblyStateCreateInfo InputAssembly{};
  InputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

  VkPipelineRasterizationStateCreateInfo Rasterizer{};
  Rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  Rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  Rasterizer.lineWidth = 1.0f;
  Rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  Rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

  VkDynamicState DynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                    VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo DynamicState{};
  DynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  DynamicState.dynamicStateCount = 2;
  DynamicState.pDynamicStates = DynamicStates;

  VkPipelineViewportStateCreateInfo ViewportState{};
  ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  ViewportState.viewportCount = 1;
  ViewportState.scissorCount = 1;

  VkPipelineMultisampleStateCreateInfo Multisampling{};
  Multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  Multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState BlendAttachment{};
  BlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

  VkPipelineColorBlendStateCreateInfo ColorBlend{};
  ColorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  ColorBlend.attachmentCount = 1;
  ColorBlend.pAttachments = &BlendAttachment;

  VkPipelineRenderingCreateInfo RenderingInfo{};
  RenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  RenderingInfo.colorAttachmentCount = 1;
  RenderingInfo.pColorAttachmentFormats = &InContext.GetSwapchainImageFormat();
  RenderingInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;

  std::vector<VkPushConstantRange> PushRanges(2);
  PushRanges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  PushRanges[0].offset = 0;
  PushRanges[0].size = 2 * sizeof(glm::mat4); // MVP = 64 bytes

  PushRanges[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  PushRanges[1].offset = 2 * sizeof(glm::mat4);
  PushRanges[1].size = sizeof(glm::vec3); // CameraPos = 12 bytes

  // Create descriptor set layout for the material
  std::vector<VkDescriptorSetLayoutBinding> Bindings(MaxViews);

  for (int i = 0; i < MaxViews; ++i) {
    Bindings[i].binding = i;
    Bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    Bindings[i].descriptorCount = 1;
    Bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  }

  VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutInfo{};
  DescriptorSetLayoutInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  DescriptorSetLayoutInfo.bindingCount = MaxViews;
  DescriptorSetLayoutInfo.pBindings = Bindings.data();

  CHECK_VK(vkCreateDescriptorSetLayout(Device, &DescriptorSetLayoutInfo,
                                       nullptr, &DescriptorSetLayout),
           "Failed to create descriptor set layout");

  //

  VkPipelineLayoutCreateInfo LayoutInfo{};
  LayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  LayoutInfo.pushConstantRangeCount = 2;
  LayoutInfo.pPushConstantRanges = PushRanges.data();
  LayoutInfo.setLayoutCount = 1;
  LayoutInfo.pSetLayouts = &DescriptorSetLayout;

  CHECK_VK(
      vkCreatePipelineLayout(Device, &LayoutInfo, nullptr, &PipelineLayout),
      "Failed creating layout");

  VkPipelineDepthStencilStateCreateInfo DepthStencil{};
  DepthStencil.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  DepthStencil.depthTestEnable = VK_TRUE;
  DepthStencil.depthWriteEnable = VK_TRUE;
  DepthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

  VkGraphicsPipelineCreateInfo PipelineInfo{};
  PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  PipelineInfo.pNext = &RenderingInfo;
  PipelineInfo.stageCount = 2;
  PipelineInfo.pStages = ShaderStages;
  PipelineInfo.pVertexInputState = &VertexInput;
  PipelineInfo.pInputAssemblyState = &InputAssembly;
  PipelineInfo.pRasterizationState = &Rasterizer;
  PipelineInfo.pViewportState = &ViewportState;
  PipelineInfo.pDynamicState = &DynamicState;
  PipelineInfo.pMultisampleState = &Multisampling;
  PipelineInfo.pColorBlendState = &ColorBlend;
  PipelineInfo.layout = PipelineLayout;
  PipelineInfo.pDepthStencilState = &DepthStencil;
  PipelineInfo.renderPass = VK_NULL_HANDLE;

  CHECK_VK(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &PipelineInfo,
                                     nullptr, &GraphicsPipeline),
           "Failed creating pipeline");

  vkDestroyShaderModule(Device, VertModule, nullptr);
  vkDestroyShaderModule(Device, FragModule, nullptr);
}

VkShaderModule VulkanPipeline::CreateShaderModule(const std::string &InPath) {
  std::ifstream File(InPath, std::ios::ate | std::ios::binary);
  CHECK_DIE(File.is_open(), "Failed to open shader file");

  size_t FileSize = static_cast<size_t>(File.tellg());
  std::vector<char> Code(FileSize);
  File.seekg(0);
  File.read(Code.data(), FileSize);

  VkShaderModuleCreateInfo CreateInfo{};
  CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  CreateInfo.codeSize = Code.size();
  CreateInfo.pCode = reinterpret_cast<const uint32_t *>(Code.data());

  VkShaderModule Module;
  CHECK_VK(vkCreateShaderModule(Device, &CreateInfo, nullptr, &Module),
           "Failed creating shader module");
  return Module;
}

VulkanPipeline::~VulkanPipeline() {
  vkDestroyPipeline(Device, GraphicsPipeline, nullptr);
  vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
  vkDestroyDescriptorSetLayout(Device, DescriptorSetLayout, nullptr);
}

void VulkanPipeline::Draw(VkCommandBuffer InCmd, Mesh &InMesh,
                          Material &InMaterial,
                          std::vector<PushConstants> PushDatas) {

  VkBuffer Buffers[] = {InMesh.VertexBuffer};
  VkDeviceSize Offsets[] = {0};
  vkCmdBindVertexBuffers(InCmd, 0, 1, Buffers, Offsets);

  vkCmdBindPipeline(InCmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    this->GraphicsPipeline);

  vkCmdBindDescriptorSets(InCmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          this->PipelineLayout, 0, 1, &InMaterial.DescriptorSet,
                          0, nullptr);

  for (auto &PushData : PushDatas) {
    vkCmdPushConstants(InCmd, this->PipelineLayout, PushData.stage,
                       PushData.offset, PushData.size, PushData.data);
  }

  vkCmdBindIndexBuffer(InCmd, InMesh.IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(InCmd, InMesh.IndexCount, 1, 0, 0, 0);
}