#pragma once
#include "VulkanContext.h"

class Material {
    friend class VulkanPipeline;

public:
  Material(VulkanContext &Context, VkDescriptorSetLayout DescriptorSetLayout, const std::vector<VkImageView> &TextureImageViews, const std::vector<VkSampler> &TextureSamplers);
  ~Material();

private:
  VulkanContext &Context;
  VkDevice Device = VK_NULL_HANDLE;
  VkDescriptorSetLayout DescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;
};