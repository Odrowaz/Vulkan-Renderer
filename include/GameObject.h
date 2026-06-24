#pragma once
#include "Mesh.h"
#include "Material.h"
#include "Transform.h"
#include "VulkanContext.h"
#include "VulkanPipeline.h"
#include "Camera.h"


class GameObject: public Transform {
public:
  GameObject(VulkanContext& Context, Mesh& Mesh, Material& Material, VulkanPipeline& Pipeline, Camera& InCamera);
  virtual ~GameObject();
  virtual void Update(float DeltaTime);
  virtual void Draw(VkCommandBuffer InCmd);
private:
  VulkanContext& Context;
  Mesh& ObjectMesh;
  Material& ObjectMaterial;
  VulkanPipeline& ObjectPipeline;
  Camera& MainCamera;
};
