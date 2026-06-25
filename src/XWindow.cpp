#include "XWindow.h"
#include "Common.h"
#include <iostream>

XWindow::XWindow(int InWidth, int InHeight, const std::string &InTitle)
    : Width(InWidth), Height(InHeight), Title(InTitle), DeltaTime(0.f) {
  CHECK_DIE(glfwInit(), "GLFW Initialization");

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // No OpenGL context

  GLFWmonitor *Monitor = NULL; // windowed mode
  RawWindow = glfwCreateWindow(Width, Height, Title.c_str(), Monitor, NULL);
  CHECK_DIE(RawWindow, "GLFW Create Window");

  glfwSetWindowUserPointer(RawWindow, this);
  glfwSetFramebufferSizeCallback(RawWindow, [](GLFWwindow *w, int, int) {
    VulkanContext::GetInstance().NotifyFramebufferResized();
  });

  VulkanContext::GetInstance().Init(RawWindow);

  SetCursorMode(GLFW_CURSOR_DISABLED); // Hide and capture the cursor
}

XWindow::~XWindow() {
  VulkanContext::GetInstance().WaitIdle();
  VulkanContext::GetInstance().Cleanup();
  glfwDestroyWindow(RawWindow);
  glfwTerminate();
}

void XWindow::PollEvents() {
  static float LastTime = glfwGetTime();
  float CurrentTime = glfwGetTime();
  DeltaTime = CurrentTime - LastTime;
  LastTime = CurrentTime;

  glfwPollEvents();
}

void XWindow::Update(std::function<void(VkCommandBuffer)> DrawCallback) {
  VulkanContext::GetInstance().DrawFrame(DrawCallback);
}

bool XWindow::IsOpened() const { return !glfwWindowShouldClose(RawWindow); }

void XWindow::SetTitle(const std::string &InTitle) {
  Title = InTitle;
  glfwSetWindowTitle(RawWindow, Title.c_str());
}

float XWindow::GetDeltaTime() const { return DeltaTime; }

void XWindow::SetVSync(bool InEnabled) {
  std::cout << "VSync " << (InEnabled ? "enabled" : "disabled") << std::endl;
  if (InEnabled)
    VulkanContext::GetInstance().PreferredPresentMode =
        VK_PRESENT_MODE_FIFO_KHR; // Enable VSync
  else
    VulkanContext::GetInstance().PreferredPresentMode =
        VK_PRESENT_MODE_IMMEDIATE_KHR; // Disable VSync

  VulkanContext::GetInstance()
      .NotifyFramebufferResized(); // Trigger swapchain recreation to apply
                                   // the new present mode
}

float XWindow::GetAspectRatio() const {
  return static_cast<float>(
             VulkanContext::GetInstance().GetSwapchainExtent().width) /
         static_cast<float>(
             VulkanContext::GetInstance().GetSwapchainExtent().height);
}

int XWindow::GetWidth() const { return Width; }

int XWindow::GetHeight() const { return Height; }
