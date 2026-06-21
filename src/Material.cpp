#include "Material.h"
#include "Common.h"
#include <vector>
#include <vulkan/vulkan.h>
#include "VulkanPipeline.h"

Material::Material(VulkanContext &Context, VkDescriptorSetLayout DescriptorSetLayout,
                   const std::vector<VkImageView> &TextureImageViews, const std::vector<VkSampler> &TextureSamplers)
    : Context(Context), Device(Context.GetDevice()), DescriptorSetLayout(DescriptorSetLayout) {

  VkDescriptorSetAllocateInfo AllocInfo{};
  AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  AllocInfo.descriptorPool = Context.GetDescriptorPool();
  AllocInfo.descriptorSetCount = 1;
  AllocInfo.pSetLayouts = &DescriptorSetLayout;

  CHECK_VK(vkAllocateDescriptorSets(Device, &AllocInfo, &DescriptorSet),
           "Failed to allocate descriptor set");

  std::vector<VkDescriptorImageInfo> ImageInfos(TextureImageViews.size());
  std::vector<VkWriteDescriptorSet> DescriptorWrites(TextureImageViews.size());

  for (size_t i = 0; i < TextureImageViews.size(); ++i) {
    ImageInfos[i] = {};
    ImageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ImageInfos[i].imageView = TextureImageViews[i];
    ImageInfos[i].sampler = TextureSamplers[i];

    DescriptorWrites[i] = {};
    DescriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    DescriptorWrites[i].dstSet = DescriptorSet;
    DescriptorWrites[i].dstBinding = i;
    DescriptorWrites[i].dstArrayElement = 0;
    DescriptorWrites[i].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    DescriptorWrites[i].descriptorCount = 1;
    DescriptorWrites[i].pImageInfo = &ImageInfos[i];
  }

  vkUpdateDescriptorSets(Device, TextureImageViews.size(), DescriptorWrites.data(), 0, nullptr);
}

Material::~Material() {
  vkFreeDescriptorSets(Device, Context.GetDescriptorPool(), 1, &DescriptorSet);
}