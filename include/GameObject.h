#pragma once
#include "Texture.h"
#include "Mesh.h"
#include "VulkanPipeline.h"


class GameObject {
public:
  GameObject();
  ~GameObject();
private:
  Texture ObjectTexture;
  Mesh ObjectMesh;
  VulkanPipeline ObjectPipeline;
};
