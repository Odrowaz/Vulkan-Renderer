#pragma once
#include "Material.h"
#include "Mesh.h"
#include "VulkanContext.h"
#include <glm/glm.hpp>
#include <string>
#include <vulkan/vulkan.h>

struct PushConstants {
  VkShaderStageFlagBits stage;
  size_t size;
  size_t offset;
  void *data;
};

class VulkanPipeline {
  friend class VulkanContext;

public:
  VulkanPipeline(VulkanContext &InContext, std::string VertexShaderPath,
                 std::string FragmentShaderPath, int MaxViews);
  ~VulkanPipeline();

  VkDescriptorSetLayout GetDescriptorSetLayout() const {
    return DescriptorSetLayout;
  }

  void Draw(VkCommandBuffer InCmd, Mesh &InMesh, Material &InMaterial,
            std::vector<PushConstants> PushDatas);

private:
  VkDevice Device = VK_NULL_HANDLE;
  VkPipeline GraphicsPipeline = VK_NULL_HANDLE;
  VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;

  VkDescriptorSetLayout DescriptorSetLayout = VK_NULL_HANDLE;

  VkShaderModule CreateShaderModule(const std::string &InPath);
};