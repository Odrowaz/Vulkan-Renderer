#pragma once

#include <functional>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <optional>
#include <string>

struct QueueFamilyIndices
{
    std::optional<uint32_t> GraphicsFamily;
    std::optional<uint32_t> PresentFamily;
    bool IsComplete() const { return GraphicsFamily.has_value() && PresentFamily.has_value(); }
};

struct SwapchainSupportDetails
{
    VkSurfaceCapabilitiesKHR        Capabilities{};
    std::vector<VkSurfaceFormatKHR> Formats;
    std::vector<VkPresentModeKHR>   PresentModes;
};

class VulkanContext
{
public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    void Init(GLFWwindow* InWindow);
    void DrawFrame(std::function<void(VkCommandBuffer)> DrawCallback);
    void WaitIdle();
    void Cleanup();

    VkPresentModeKHR PreferredPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR; // Default to immediate mode (no VSync)

    VkDevice GetDevice() const { return Device; }
    VkFormat& GetSwapchainImageFormat() { return SwapchainImageFormat; }
    VkDescriptorPool GetDescriptorPool() const { return DescriptorPool; }
    VkSampler GetTextureSampler() const { return TextureSampler; }
    VkExtent2D& GetSwapchainExtent() { return SwapchainExtent; }

    uint32_t                            FindMemoryType(uint32_t InTypeFilter, VkMemoryPropertyFlags InProps);
    VkCommandBuffer                     BeginSingleTimeCommands();
    void                                EndSingleTimeCommands(VkCommandBuffer Cmd);    
    void                                TransitionImageLayout(VkCommandBuffer Cmd, VkImage Image, VkImageLayout OldLayout, VkImageLayout NewLayout);

    void NotifyFramebufferResized() { FramebufferResized = true; }

    std::pair<VkImage, VkDeviceMemory>  CreateTexture(std::string InPath);
    VkImageView                         CreateTextureImageView(VkImage TextureImage);

private:
    GLFWwindow*                  Window = nullptr;

    // Core
    VkInstance                   Instance       = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT     DebugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR                 Surface        = VK_NULL_HANDLE;
    VkPhysicalDevice             PhysicalDevice = VK_NULL_HANDLE;
    VkDevice                     Device         = VK_NULL_HANDLE;
    VkQueue                      GraphicsQueue  = VK_NULL_HANDLE;
    VkQueue                      PresentQueue   = VK_NULL_HANDLE;

    // Swapchain
    VkSwapchainKHR               Swapchain = VK_NULL_HANDLE;
    VkFormat                     SwapchainImageFormat{};
    VkExtent2D                   SwapchainExtent{};
    std::vector<VkImage>         SwapchainImages;
    std::vector<VkImageView>     SwapchainImageViews;

    // Commands
    VkCommandPool                CommandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> CommandBuffers;

    // Synchronisation
    std::vector<VkSemaphore>     ImageAvailableSemaphores;
    std::vector<VkSemaphore>     RenderFinishedSemaphores;
    std::vector<VkFence>         InFlightFences;
    uint32_t                     CurrentFrame       = 0;
    bool                         FramebufferResized = false;
    bool                         ValidationEnabled  = false;

    // Texture
    VkSampler                    TextureSampler = VK_NULL_HANDLE;

    // Descriptor                   
    VkDescriptorPool             DescriptorPool = VK_NULL_HANDLE;

    // Model Pipeline
    VkImage                      DepthImage = VK_NULL_HANDLE;
    VkDeviceMemory               DepthImageMemory = VK_NULL_HANDLE;
    VkImageView                  DepthImageView = VK_NULL_HANDLE;


    // Initialisation steps (called in order by Init)
    void CreateInstance();
    void SetupDebugMessenger();
    void CreateSurface();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSwapchain();
    void CreateImageViews();
    void CreateCommandPool();
    void AllocateCommandBuffers();
    void CreateRenderFinishedSemaphores();
    void CreateSyncObjects();

    // Runtime helpers
    void RecordCommandBuffer(VkCommandBuffer InCmd, uint32_t InImageIndex, std::function<void(VkCommandBuffer)> DrawCallback);
    void RecreateSwapchain();
    void CleanupSwapchain();

    // Query helpers
    QueueFamilyIndices                  FindQueueFamilies(VkPhysicalDevice InDevice);
    SwapchainSupportDetails             QuerySwapchainSupport(VkPhysicalDevice InDevice);
    bool                                IsDeviceSuitable(VkPhysicalDevice InDevice);
    VkSurfaceFormatKHR                  ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& InFormats);
    VkPresentModeKHR                    ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& InModes);
    VkExtent2D                          ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& InCaps);

    void                                CreateDepthResources();

    void                                CreateTextureSampler();
    void                                CreateDescriptorPool();
};
