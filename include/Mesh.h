#pragma once
#include "VulkanContext.h"
#include <vulkan/vulkan.h>

class Mesh {
  friend class VulkanPipeline;

public:
  Mesh(VulkanContext &InContext, std::string InPath);
  ~Mesh();

private:
  VkDevice Device = VK_NULL_HANDLE;
  VkBuffer VertexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory VertexBufferMemory = VK_NULL_HANDLE;
  VkBuffer IndexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory IndexBufferMemory = VK_NULL_HANDLE;
  uint32_t VertexCount = 0;
  uint32_t IndexCount = 0;
};