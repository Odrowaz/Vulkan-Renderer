#include "Texture.h"
#include "Common.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


Texture::Texture(VulkanContext &InContext, std::string InPath) {
  this->Device = InContext.GetDevice();

  // Load image with stb_image
  int Width, Height, Channels;
  uint8_t *Data = stbi_load(InPath.c_str(), &Width, &Height, &Channels, 0);
  if (Data == NULL) {
    std::cout << "Error reading image: " << InPath << std::endl;
    throw std::runtime_error("Error reading image");
  }

  // Texture buffer ----------------
  VkDeviceSize BufferSize = Width * Height * Channels * sizeof(uint8_t);

  VkBufferCreateInfo BufferInfo{};
  BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  BufferInfo.size = BufferSize;
  BufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkBuffer TextureBuffer;

  CHECK_VK(vkCreateBuffer(Device, &BufferInfo, nullptr, &TextureBuffer),
           "Failed to create texture buffer");

  VkMemoryRequirements MemReqs;
  vkGetBufferMemoryRequirements(Device, TextureBuffer, &MemReqs);

  VkMemoryAllocateInfo AllocInfo{};
  AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  AllocInfo.allocationSize = MemReqs.size;
  AllocInfo.memoryTypeIndex = InContext.FindMemoryType(
      MemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  VkDeviceMemory TextureBufferMemory;

  CHECK_VK(vkAllocateMemory(Device, &AllocInfo, nullptr, &TextureBufferMemory),
           "Failed to create buffer memory");

  vkBindBufferMemory(Device, TextureBuffer, TextureBufferMemory, 0);

  // Copy texture data to GPU
  void *TexBuffData;
  vkMapMemory(Device, TextureBufferMemory, 0, BufferSize, 0, &TexBuffData);
  memcpy(TexBuffData, Data, BufferSize);
  vkUnmapMemory(Device, TextureBufferMemory);

  stbi_image_free(Data);

  // Create Image
  VkImageCreateInfo ImageInfo{};
  ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  ImageInfo.imageType = VK_IMAGE_TYPE_2D;
  ImageInfo.extent.width = Width;
  ImageInfo.extent.height = Height;
  ImageInfo.extent.depth = 1;
  ImageInfo.mipLevels = 1;
  ImageInfo.arrayLayers = 1;
  ImageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
  ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  ImageInfo.usage =
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

  CHECK_VK(vkCreateImage(Device, &ImageInfo, nullptr, &Image),
           "Failed to create texture image");

  vkGetImageMemoryRequirements(Device, Image, &MemReqs);

  AllocInfo = {};
  AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  AllocInfo.allocationSize = MemReqs.size;
  AllocInfo.memoryTypeIndex = InContext.FindMemoryType(
      MemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  CHECK_VK(vkAllocateMemory(Device, &AllocInfo, nullptr, &ImageMemory),
           "Failed to allocate texture image memory");

  vkBindImageMemory(Device, Image, ImageMemory, 0);

  // Copy Buffer to Image
  VkCommandBuffer Cmd = InContext.BeginSingleTimeCommands();

  // Transition: UNDEFINED -> TRANSFER_DST_OPTIMAL
  InContext.TransitionImageLayout(Cmd, Image, VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  VkBufferImageCopy Region{};
  Region.bufferOffset = 0;
  Region.bufferRowLength = 0;
  Region.bufferImageHeight = 0;
  Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  Region.imageSubresource.mipLevel = 0;
  Region.imageSubresource.baseArrayLayer = 0;
  Region.imageSubresource.layerCount = 1;
  Region.imageOffset = {0, 0, 0};
  Region.imageExtent = {(uint32_t)Width, (uint32_t)Height, 1};

  vkCmdCopyBufferToImage(Cmd, TextureBuffer, Image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);

  // Transition: TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
  InContext.TransitionImageLayout(Cmd, Image,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  InContext.EndSingleTimeCommands(Cmd);

  vkDestroyBuffer(Device, TextureBuffer, nullptr);
  vkFreeMemory(Device, TextureBufferMemory, nullptr);

  this->ImageView = CreateTextureImageView(Image);
}

Texture::~Texture() {
  vkDestroyImageView(Device, ImageView, nullptr);
  vkDestroyImage(Device, Image, nullptr);
  vkFreeMemory(Device, ImageMemory, nullptr);
};

VkImageView Texture::CreateTextureImageView(VkImage TextureImage) {
  VkImageViewCreateInfo ViewInfo{};
  ViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  ViewInfo.image = TextureImage;
  ViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  ViewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
  ViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  ViewInfo.subresourceRange.baseMipLevel = 0;
  ViewInfo.subresourceRange.levelCount = 1;
  ViewInfo.subresourceRange.baseArrayLayer = 0;
  ViewInfo.subresourceRange.layerCount = 1;

  VkImageView TextureImageView;

  CHECK_VK(vkCreateImageView(Device, &ViewInfo, nullptr, &TextureImageView),
           "Failed to create texture image view");

  return TextureImageView;
}