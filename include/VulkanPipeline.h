#pragma once
#include "Material.h"
#include "Mesh.h"
#include "VulkanContext.h"
#include <string>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>


struct PushConstants {
  glm::mat4 Mvp;
  glm::mat4 Model;
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
            PushConstants PushData);

private:
  VkDevice Device = VK_NULL_HANDLE;
  VkPipeline GraphicsPipeline = VK_NULL_HANDLE;
  VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;

  VkDescriptorSetLayout DescriptorSetLayout = VK_NULL_HANDLE;

  VkShaderModule CreateShaderModule(const std::string &InPath);
};