#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

glm::mat4 Camera::GetProjectionMatrix() const {
  glm::mat4 Proj =
      glm::perspective(glm::radians(FOV), AspectRatio, NearPlane, FarPlane);
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

  glm::mat4 View =
      glm::lookAt(Position, Position + Forward, glm::vec3(0.f, 1.f, 0.f));

  return View;
}

void Camera::Update(XWindow &Window, float DeltaTime) {
  
  if (Window.KeyPressed(GLFW_KEY_W)) {
    Position += GetForward() * DeltaTime * 5.f;
  } else if (Window.KeyPressed(GLFW_KEY_S)) {
    Position -= GetForward() * DeltaTime * 5.f;
  }
  if (Window.KeyPressed(GLFW_KEY_A)) {
    Position -= GetRight() * DeltaTime * 5.f;
  } else if (Window.KeyPressed(GLFW_KEY_D)) {
    Position += GetRight() * DeltaTime * 5.f;
  }

  static glm::vec2 LastMousePosition = Window.Mouse();
  glm::vec2 MouseDelta = Window.Mouse() - LastMousePosition;
  LastMousePosition = Window.Mouse();

  Rotation.y += MouseDelta.x * DeltaTime * 100.f;
  Rotation.x -= MouseDelta.y * DeltaTime * 100.f;

}
