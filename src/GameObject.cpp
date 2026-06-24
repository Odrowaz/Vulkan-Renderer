#include "GameObject.h"
#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

GameObject::GameObject(VulkanContext &Context, Mesh &Mesh, Material &Material,
                       VulkanPipeline &Pipeline, Camera &InCamera)
    : ObjectMesh(Mesh), Context(Context), ObjectMaterial(Material),
      ObjectPipeline(Pipeline), MainCamera(InCamera) {}

void GameObject::Update(float DeltaTime) { 
    Rotation.y += 25.f * DeltaTime; 
}

void GameObject::Draw(VkCommandBuffer InCmd) {
  glm::mat4 Model = glm::mat4(1.0f);
  Model = glm::translate(Model, glm::vec3(0.0f, -2.0f, -4.f));
  Model = glm::rotate(Model, glm::radians(Rotation.x), glm::vec3(1.f, 0.f, 0.f));
  Model = glm::rotate(Model, glm::radians(Rotation.y), glm::vec3(0.f, 1.f, 0.f));
  Model = glm::rotate(Model, glm::radians(Rotation.z), glm::vec3(0.f, 0.f, 1.f));
  Model = glm::scale(Model,glm::vec3(1.f, 1.f,1.f));

  glm::mat4 Mvp = MainCamera.GetProjectionMatrix() * MainCamera.GetViewMatrix() * Model;

  PushConstants data = {Mvp, Model};
  ObjectPipeline.Draw(InCmd, ObjectMesh, ObjectMaterial, data);
}

GameObject::~GameObject() {}
