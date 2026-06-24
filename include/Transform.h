#pragma once
#include <glm/glm.hpp>

class Transform {
public:
    Transform() : Position(0.0f), Rotation(0.0f), Scale(1.0f) {}
    glm::vec3 Position;
    glm::vec3 Rotation;
    glm::vec3 Scale;
};