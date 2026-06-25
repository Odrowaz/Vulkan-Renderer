#pragma once

#include <GLFW/glfw3.h>
#include <string>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include "VulkanContext.h"

class XWindow
{
public:
    XWindow(int InWidth, int InHeight, const std::string& InTitle);
    ~XWindow();
    void PollEvents();
    void Update(std::function<void(VkCommandBuffer)> DrawCallback);
    bool IsOpened() const;
    void SetTitle(const std::string& InTitle);
    float GetDeltaTime() const;
    void SetVSync(bool InEnabled);
    int GetWidth() const;
    int GetHeight() const;
    float GetAspectRatio() const;
    void SetCursorMode(int Mode) { glfwSetInputMode(RawWindow, GLFW_CURSOR, Mode); }
    glm::vec2 Mouse() const {
        double x, y;
        glfwGetCursorPos(RawWindow, &x, &y);
        return glm::vec2(static_cast<float>(x), static_cast<float>(y));
    }

    bool KeyPressed(int Key) const { return glfwGetKey(RawWindow, Key) == GLFW_PRESS; }
private:
    int Width;
    int Height;
    std::string Title;
    GLFWwindow* RawWindow;
    float DeltaTime;
};