#pragma once
#include "VulkanContext.h"
#include <vulkan/vulkan.h>

class Texture {
  friend class VulkanPipeline;

public:
  Texture(VulkanContext &InContext, std::string InPath);
  ~Texture();

  VkImageView GetImageView() const { return ImageView; }

private:
  VkDevice Device = VK_NULL_HANDLE;
  VkImage Image = VK_NULL_HANDLE;
  VkDeviceMemory ImageMemory = VK_NULL_HANDLE;
  VkImageView ImageView = VK_NULL_HANDLE;
  VkSampler Sampler = VK_NULL_HANDLE;

  VkImageView CreateTextureImageView(VkImage TextureImage);
};