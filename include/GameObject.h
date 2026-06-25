#pragma once
#include "Mesh.h"
#include "Material.h"
#include "Transform.h"
#include "Camera.h"


class GameObject: public Transform {
public:
  GameObject(Mesh& Mesh, Material& Material);
  GameObject(Mesh& Mesh, Material& Material, Camera& InCamera);
  virtual ~GameObject();
  virtual void Update(float DeltaTime);
  virtual void Draw(VkCommandBuffer InCmd);
private:
  Mesh& ObjectMesh;
  Material& ObjectMaterial;
  Camera& MainCamera;
};
