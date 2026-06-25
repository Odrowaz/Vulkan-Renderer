#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Transform {
public:
  Transform() : Position(0.0f), Rotation(0.0f), Scale(1.0f) {}
  glm::vec3 Position;
  glm::vec3 Rotation;
  glm::vec3 Scale;

  glm::vec3 GetForward() {
    glm::vec3 Forward;
    Forward.x = cos(glm::radians(Rotation.x)) * sin(glm::radians(Rotation.y));
    Forward.y = sin(glm::radians(Rotation.x));
    Forward.z = -cos(glm::radians(Rotation.x)) * cos(glm::radians(Rotation.y));
    return Forward;
  }

  glm::vec3 GetRight() {
    glm::vec3 Right =
        glm::normalize(glm::cross(GetForward(), glm::vec3(0.f, 1.f, 0.f)));
    return Right;
  }
};