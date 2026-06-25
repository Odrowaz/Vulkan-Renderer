#include "GameObject.h"
#include "Camera.h"
#include "CameraManager.h"
#include "Material.h"
#include <glm/gtc/matrix_transform.hpp>

GameObject::GameObject(Mesh &Mesh, Material &Material)
    : ObjectMesh(Mesh), ObjectMaterial(Material), MainCamera(CameraManager::GetMainCamera()) {}

GameObject::GameObject(Mesh &Mesh, Material &Material, Camera &InCamera)
    : ObjectMesh(Mesh), ObjectMaterial(Material), MainCamera(InCamera) {}

void GameObject::Update(float DeltaTime) { 
    Rotation.y += 25.f * DeltaTime; 
}

void GameObject::Draw(VkCommandBuffer InCmd) {
  glm::mat4 Model = glm::mat4(1.0f);
  Model = glm::translate(Model,  Position);
  Model = glm::rotate(Model, glm::radians(Rotation.x), glm::vec3(1.f, 0.f, 0.f));
  Model = glm::rotate(Model, glm::radians(Rotation.y), glm::vec3(0.f, 1.f, 0.f));
  Model = glm::rotate(Model, glm::radians(Rotation.z), glm::vec3(0.f, 0.f, 1.f));
  Model = glm::scale(Model, Scale);

  glm::mat4 Mvp = MainCamera.GetProjectionMatrix() * MainCamera.GetViewMatrix() * Model;

  struct {glm::mat4 Mvp; glm::mat4 Model;} VertexData = {Mvp, Model};

  ObjectMaterial.Bind(InCmd);

  ObjectMaterial.GetPipeline().SetPushConstants(InCmd, {
    {
          VK_SHADER_STAGE_VERTEX_BIT,
          2 * sizeof(glm::mat4),
          0,
          &VertexData
        },
    {
          VK_SHADER_STAGE_FRAGMENT_BIT,
          sizeof(glm::vec3),
          2 * sizeof(glm::mat4),
          &MainCamera.Position
        }
  });

  ObjectMesh.Draw(InCmd);

}

GameObject::~GameObject() {}
