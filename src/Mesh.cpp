#include "Mesh.h"
#include "Common.h"
#include "ObjLoader.h"


Mesh::Mesh(VulkanContext &InContext, std::string InPath) : Device(InContext.GetDevice()) {
  ModelData Data = LoadObj(InPath);
  std::cout << "Loaded model with " << Data.VertexCount << " vertices and "
            << Data.IndexCount << " indices.\n";
  VertexCount = Data.VertexCount;
  IndexCount = Data.IndexCount;

  // Vertex buffer ----------------
  VkDeviceSize BufferSize = Data.Vertices.size() * sizeof(float);

  VkBufferCreateInfo BufferInfo{};
  BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  BufferInfo.size = BufferSize;
  BufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  CHECK_VK(vkCreateBuffer(Device, &BufferInfo, nullptr, &VertexBuffer),
           "Failed to create vertex buffer");

  VkMemoryRequirements MemReqs;
  vkGetBufferMemoryRequirements(Device, VertexBuffer, &MemReqs);

  VkMemoryAllocateInfo AllocInfo{};
  AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  AllocInfo.allocationSize = MemReqs.size;
  AllocInfo.memoryTypeIndex = InContext.FindMemoryType(
      MemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  CHECK_VK(vkAllocateMemory(Device, &AllocInfo, nullptr, &VertexBufferMemory),
           "Failed to create buffer memory");

  vkBindBufferMemory(Device, VertexBuffer, VertexBufferMemory, 0);

  // Copy vertext data to GPU
  void *VertBuffData;
  vkMapMemory(Device, VertexBufferMemory, 0, BufferSize, 0, &VertBuffData);
  memcpy(VertBuffData, Data.Vertices.data(), BufferSize);
  vkUnmapMemory(Device, VertexBufferMemory);

  // Index buffer ----------------
  VkDeviceSize IndexBufSize = Data.Indices.size() * sizeof(uint32_t);

  VkBufferCreateInfo IndexBuffInfo{};
  IndexBuffInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  IndexBuffInfo.size = IndexBufSize;
  IndexBuffInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  IndexBuffInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  CHECK_VK(vkCreateBuffer(Device, &IndexBuffInfo, nullptr, &IndexBuffer),
           "Failed to create index buffer");

  VkMemoryRequirements IndexBuffMemReqs;
  vkGetBufferMemoryRequirements(Device, IndexBuffer, &IndexBuffMemReqs);

  VkMemoryAllocateInfo IndexBuffAllocInfo{};
  IndexBuffAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  IndexBuffAllocInfo.allocationSize = IndexBuffMemReqs.size;
  IndexBuffAllocInfo.memoryTypeIndex =
      InContext.FindMemoryType(IndexBuffMemReqs.memoryTypeBits,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  CHECK_VK(vkAllocateMemory(Device, &IndexBuffAllocInfo, nullptr,
                            &IndexBufferMemory),
           "Failed to create buffer memory");

  vkBindBufferMemory(Device, IndexBuffer, IndexBufferMemory, 0);

  // Copy index data to GPU
  void *IndexBuffData;
  vkMapMemory(Device, IndexBufferMemory, 0, IndexBufSize, 0, &IndexBuffData);
  memcpy(IndexBuffData, Data.Indices.data(), IndexBufSize);
  vkUnmapMemory(Device, IndexBufferMemory);
}

Mesh::~Mesh() {
  vkDestroyBuffer(Device, IndexBuffer, nullptr);
  vkFreeMemory(Device, IndexBufferMemory, nullptr);

  vkDestroyBuffer(Device, VertexBuffer, nullptr);
  vkFreeMemory(Device, VertexBufferMemory, nullptr);
}