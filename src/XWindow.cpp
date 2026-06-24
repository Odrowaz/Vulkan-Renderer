#include "XWindow.h"
#include "Common.h"
#include <iostream>

XWindow::XWindow(int InWidth, int InHeight, const std::string& InTitle, VulkanContext& InContext)
    : Width(InWidth), Height(InHeight), Title(InTitle), DeltaTime(0.f), Context(InContext)
{
    CHECK_DIE(glfwInit(), "GLFW Initialization");

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // No OpenGL context

	GLFWmonitor* Monitor = NULL; //windowed mode
	RawWindow = glfwCreateWindow(Width, Height, Title.c_str(), Monitor, NULL);
    CHECK_DIE(RawWindow, "GLFW Create Window");

    glfwSetWindowUserPointer(RawWindow, this);
    glfwSetFramebufferSizeCallback(RawWindow, [](GLFWwindow* w, int, int) {
        static_cast<XWindow*>(glfwGetWindowUserPointer(w))->Context.NotifyFramebufferResized();
    });

    Context.Init(RawWindow);
}

XWindow::~XWindow()
{
    Context.WaitIdle();
    Context.Cleanup();
    glfwDestroyWindow(RawWindow);
    glfwTerminate();
}

void XWindow::Update(std::function<void(VkCommandBuffer)> DrawCallback)
{
    static float LastTime = glfwGetTime();
    float CurrentTime = glfwGetTime();
    DeltaTime = CurrentTime - LastTime;
    LastTime = CurrentTime;

	glfwPollEvents();
    Context.DrawFrame(DrawCallback);
}

bool XWindow::IsOpened() const 
{
    return !glfwWindowShouldClose(RawWindow);
}

void XWindow::SetTitle(const std::string& InTitle)
{
    Title = InTitle;
    glfwSetWindowTitle(RawWindow, Title.c_str());
}

float XWindow::GetDeltaTime() const
{
    return DeltaTime;
}

void XWindow::SetVSync(bool InEnabled)
{
    std::cout << "VSync " << (InEnabled ? "enabled" : "disabled") << std::endl;
    if (InEnabled)
        Context.PreferredPresentMode = VK_PRESENT_MODE_FIFO_KHR; // Enable VSync
    else
        Context.PreferredPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR; // Disable VSync

    Context.NotifyFramebufferResized(); // Trigger swapchain recreation to apply the new present mode
}

float XWindow::GetAspectRatio() const
{
    return static_cast<float>(Context.GetSwapchainExtent().width) / static_cast<float>(Context.GetSwapchainExtent().height);
}

int XWindow::GetWidth() const
{
    return Width;
}

int XWindow::GetHeight() const
{
    return Height;
}
