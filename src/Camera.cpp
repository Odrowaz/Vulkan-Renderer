#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

glm::mat4 Camera::GetProjectionMatrix() const {
  glm::mat4 Proj = glm::perspective(glm::radians(FOV), AspectRatio, NearPlane, FarPlane);
  Proj[1][1] *= -1; // Flip Y for Vulkan's coordinate system

  return Proj;
}

glm::mat4 Camera::GetViewMatrix() const {
  glm::vec3 Forward;

  Forward.x = cos(glm::radians(Rotation.x)) * sin(glm::radians(Rotation.y));
  Forward.y = sin(glm::radians(Rotation.x));
  Forward.z = -cos(glm::radians(Rotation.x)) * cos(glm::radians(Rotation.y));

  Forward = glm::normalize(Forward);

  Forward = glm::normalize(Forward);

  glm::mat4 View = glm::lookAt(Position,Position + Forward,glm::vec3(0.f, 1.f, 0.f));

  return View;
}