#include "VulkanContext.h"
#include "Common.h"
#include <algorithm>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

// ── Validation-layer configuration ───────────────────────────────────

#ifdef _DEBUG
static constexpr bool kWantValidation = true;
#else
static constexpr bool kWantValidation = false;
#endif

static const std::vector<const char *> kValidationLayers = {
    "VK_LAYER_KHRONOS_validation"};

static const std::vector<const char *> kDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

// ── Debug-messenger helpers ──────────────────────────────────────────

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT InSeverity,
    VkDebugUtilsMessageTypeFlagsEXT /*InType*/,
    const VkDebugUtilsMessengerCallbackDataEXT *InData, void * /*InUserData*/) {
  if (InSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    std::cerr << "[Vulkan Validation] " << InData->pMessage << "\n";
  return VK_FALSE;
}

static VkResult
CreateDebugUtilsMessengerEXT(VkInstance InInstance,
                             const VkDebugUtilsMessengerCreateInfoEXT *InInfo,
                             const VkAllocationCallbacks *InAlloc,
                             VkDebugUtilsMessengerEXT *OutMessenger) {
  auto Fn = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(InInstance, "vkCreateDebugUtilsMessengerEXT"));
  return Fn ? Fn(InInstance, InInfo, InAlloc, OutMessenger)
            : VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void
DestroyDebugUtilsMessengerEXT(VkInstance InInstance,
                              VkDebugUtilsMessengerEXT InMessenger,
                              const VkAllocationCallbacks *InAlloc) {
  auto Fn = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(InInstance, "vkDestroyDebugUtilsMessengerEXT"));
  if (Fn)
    Fn(InInstance, InMessenger, InAlloc);
}

static void
PopulateDebugCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &OutInfo) {
  OutInfo = {};
  OutInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  OutInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  OutInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  OutInfo.pfnUserCallback = DebugCallback;
}

// ── Public interface ─────────────────────────────────────────────────

void VulkanContext::Init(GLFWwindow *InWindow) {
  Window = InWindow;
  CreateInstance();
  SetupDebugMessenger();
  CreateSurface();
  PickPhysicalDevice();
  CreateLogicalDevice();
  CreateSwapchain();
  CreateImageViews();
  CreateDepthResources();
  CreateCommandPool();

  CreateDescriptorPool();
  CreateTextureSampler();

  AllocateCommandBuffers();
  CreateSyncObjects();
}

void VulkanContext::DrawFrame(
    std::function<void(VkCommandBuffer)> DrawCallback) {
  vkWaitForFences(Device, 1, &InFlightFences[CurrentFrame], VK_TRUE,
                  UINT64_MAX);

  uint32_t ImageIndex;
  VkResult Result = vkAcquireNextImageKHR(
      Device, Swapchain, UINT64_MAX, ImageAvailableSemaphores[CurrentFrame],
      VK_NULL_HANDLE, &ImageIndex);

  if (Result == VK_ERROR_OUT_OF_DATE_KHR) {
    RecreateSwapchain();
    return;
  }
  if (Result != VK_SUCCESS && Result != VK_SUBOPTIMAL_KHR)
    throw std::runtime_error("Failed to acquire swapchain image");

  vkResetFences(Device, 1, &InFlightFences[CurrentFrame]);
  vkResetCommandBuffer(CommandBuffers[CurrentFrame], 0);
  RecordCommandBuffer(CommandBuffers[CurrentFrame], ImageIndex, DrawCallback);

  VkSemaphore WaitSems[] = {ImageAvailableSemaphores[CurrentFrame]};
  VkPipelineStageFlags WaitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSemaphore SignalSems[] = {RenderFinishedSemaphores[ImageIndex]};

  VkSubmitInfo SubmitInfo{};
  SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  SubmitInfo.waitSemaphoreCount = 1;
  SubmitInfo.pWaitSemaphores = WaitSems;
  SubmitInfo.pWaitDstStageMask = WaitStages;
  SubmitInfo.commandBufferCount = 1;
  SubmitInfo.pCommandBuffers = &CommandBuffers[CurrentFrame];
  SubmitInfo.signalSemaphoreCount = 1;
  SubmitInfo.pSignalSemaphores = SignalSems;

  CHECK_VK(vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo,
                         InFlightFences[CurrentFrame]),
           "Failed to submit draw command buffer");

  VkSwapchainKHR Swapchains[] = {Swapchain};
  VkPresentInfoKHR PresentInfo{};
  PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  PresentInfo.waitSemaphoreCount = 1;
  PresentInfo.pWaitSemaphores = SignalSems;
  PresentInfo.swapchainCount = 1;
  PresentInfo.pSwapchains = Swapchains;
  PresentInfo.pImageIndices = &ImageIndex;

  Result = vkQueuePresentKHR(PresentQueue, &PresentInfo);

  if (Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR ||
      FramebufferResized) {
    FramebufferResized = false;
    RecreateSwapchain();
  } else if (Result != VK_SUCCESS) {
    DIE("Failed to present swapchain image");
  }

  CurrentFrame = (CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanContext::WaitIdle() { vkDeviceWaitIdle(Device); }

void VulkanContext::Cleanup() {
  CleanupSwapchain();

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(Device, ImageAvailableSemaphores[i], nullptr);
    vkDestroyFence(Device, InFlightFences[i], nullptr);
  }

  vkDestroySampler(Device, TextureSampler, nullptr);
  vkDestroyDescriptorPool(Device, DescriptorPool, nullptr);

  vkDestroyCommandPool(Device, CommandPool, nullptr);
  vkDestroyDevice(Device, nullptr);

  if (ValidationEnabled)
    DestroyDebugUtilsMessengerEXT(Instance, DebugMessenger, nullptr);

  vkDestroySurfaceKHR(Instance, Surface, nullptr);
  vkDestroyInstance(Instance, nullptr);
}

// ═════════════════════════════════════════════════════════════════════
//  Instance
// ═════════════════════════════════════════════════════════════════════

void VulkanContext::CreateInstance() {
  // Check whether validation layers are available (requires LunarG Vulkan SDK)
  if constexpr (kWantValidation) {
    uint32_t LayerCount;
    vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);
    std::vector<VkLayerProperties> Available(LayerCount);
    vkEnumerateInstanceLayerProperties(&LayerCount, Available.data());

    ValidationEnabled = true;
    for (const char *Name : kValidationLayers) {
      bool Found = false;
      for (const auto &Layer : Available)
        if (std::strcmp(Name, Layer.layerName) == 0) {
          Found = true;
          break;
        }
      if (!Found) {
        std::cerr
            << "[Warning] Validation layer not available: " << Name
            << "\n  Install the LunarG Vulkan SDK to enable validation.\n";
        ValidationEnabled = false;
        break;
      }
    }
  }

  VkApplicationInfo AppInfo{};
  AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  AppInfo.pApplicationName = "Vulkan App";
  AppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  AppInfo.pEngineName = "No Engine";
  AppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  AppInfo.apiVersion = VK_API_VERSION_1_3;

  // GLFW tells us which instance extensions it needs for surface creation
  uint32_t GlfwExtCount = 0;
  const char **GlfwExts = glfwGetRequiredInstanceExtensions(&GlfwExtCount);
  std::vector<const char *> Extensions(GlfwExts, GlfwExts + GlfwExtCount);

  if (ValidationEnabled)
    Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  VkInstanceCreateInfo CreateInfo{};
  CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  CreateInfo.pApplicationInfo = &AppInfo;
  CreateInfo.enabledExtensionCount = static_cast<uint32_t>(Extensions.size());
  CreateInfo.ppEnabledExtensionNames = Extensions.data();

  // Chain a debug messenger into instance creation so we catch errors
  // during vkCreateInstance / vkDestroyInstance as well.
  VkDebugUtilsMessengerCreateInfoEXT DebugCI{};
  if (ValidationEnabled) {
    CreateInfo.enabledLayerCount =
        static_cast<uint32_t>(kValidationLayers.size());
    CreateInfo.ppEnabledLayerNames = kValidationLayers.data();
    PopulateDebugCreateInfo(DebugCI);
    CreateInfo.pNext = &DebugCI;
  }

  CHECK_VK(vkCreateInstance(&CreateInfo, nullptr, &Instance),
           "Failed to create Vulkan instance");
}

// ═════════════════════════════════════════════════════════════════════
//  Debug messenger
// ═════════════════════════════════════════════════════════════════════

void VulkanContext::SetupDebugMessenger() {
  if (!ValidationEnabled)
    return;

  VkDebugUtilsMessengerCreateInfoEXT CreateInfo{};
  PopulateDebugCreateInfo(CreateInfo);

  CHECK_VK(CreateDebugUtilsMessengerEXT(Instance, &CreateInfo, nullptr,
                                        &DebugMessenger),
           "Failed to set up debug messenger");
}

// ═════════════════════════════════════════════════════════════════════
//  Surface
// ═════════════════════════════════════════════════════════════════════

void VulkanContext::CreateSurface() {
  CHECK_VK(glfwCreateWindowSurface(Instance, Window, nullptr, &Surface),
           "Failed to create window surface");
}

// ═════════════════════════════════════════════════════════════════════
//  Physical device selection
// ═════════════════════════════════════════════════════════════════════

QueueFamilyIndices VulkanContext::FindQueueFamilies(VkPhysicalDevice InDevice) {
  QueueFamilyIndices Indices;

  uint32_t Count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(InDevice, &Count, nullptr);
  std::vector<VkQueueFamilyProperties> Families(Count);
  vkGetPhysicalDeviceQueueFamilyProperties(InDevice, &Count, Families.data());

  for (uint32_t i = 0; i < Count; i++) {
    if (Families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
      Indices.GraphicsFamily = i;

    VkBool32 PresentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(InDevice, i, Surface, &PresentSupport);
    if (PresentSupport)
      Indices.PresentFamily = i;

    if (Indices.IsComplete())
      break;
  }
  return Indices;
}

SwapchainSupportDetails
VulkanContext::QuerySwapchainSupport(VkPhysicalDevice InDevice) {
  SwapchainSupportDetails Details;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(InDevice, Surface,
                                            &Details.Capabilities);

  uint32_t FormatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(InDevice, Surface, &FormatCount,
                                       nullptr);
  if (FormatCount) {
    Details.Formats.resize(FormatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(InDevice, Surface, &FormatCount,
                                         Details.Formats.data());
  }

  uint32_t ModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(InDevice, Surface, &ModeCount,
                                            nullptr);
  if (ModeCount) {
    Details.PresentModes.resize(ModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(InDevice, Surface, &ModeCount,
                                              Details.PresentModes.data());
  }
  return Details;
}

bool VulkanContext::IsDeviceSuitable(VkPhysicalDevice InDevice) {
  if (!FindQueueFamilies(InDevice).IsComplete())
    return false;

  // Verify required device extensions
  uint32_t ExtCount;
  vkEnumerateDeviceExtensionProperties(InDevice, nullptr, &ExtCount, nullptr);
  std::vector<VkExtensionProperties> Available(ExtCount);
  vkEnumerateDeviceExtensionProperties(InDevice, nullptr, &ExtCount,
                                       Available.data());

  std::set<std::string> Required(kDeviceExtensions.begin(),
                                 kDeviceExtensions.end());
  for (const auto &Ext : Available)
    Required.erase(Ext.extensionName);
  if (!Required.empty())
    return false;

  // At least one format and one present mode
  auto Support = QuerySwapchainSupport(InDevice);
  return !Support.Formats.empty() && !Support.PresentModes.empty();
}

void VulkanContext::PickPhysicalDevice() {
  uint32_t Count = 0;
  vkEnumeratePhysicalDevices(Instance, &Count, nullptr);
  CHECK_DIE(Count != 0, "No GPUs with Vulkan support found");

  std::vector<VkPhysicalDevice> Devices(Count);
  vkEnumeratePhysicalDevices(Instance, &Count, Devices.data());

  // Pick best suitable device, preferring discrete GPUs
  VkPhysicalDeviceProperties DeviceProps;
  for (auto Dev : Devices) {
    VkPhysicalDeviceProperties CurrentProps;
    vkGetPhysicalDeviceProperties(Dev, &CurrentProps);
    LOG_DEBUG("GPU: {}", CurrentProps.deviceName);

    if (!IsDeviceSuitable(Dev))
      continue;

    // Just to print all the devices,
    // but get just the first in order of preference (discrete, then fallback)
    if (PhysicalDevice == VK_NULL_HANDLE ||
        CurrentProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      if (CurrentProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        PhysicalDevice = Dev;
        DeviceProps = CurrentProps;
      } else // Fallback
      {
        PhysicalDevice = Dev;
        DeviceProps = CurrentProps;
      }
    }
  }

  CHECK_DIE(PhysicalDevice, "No suitable GPU found");

  LOG_DEBUG("GPU SELECTED: {} VK {}.{}", DeviceProps.deviceName,
            VK_VERSION_MAJOR(DeviceProps.apiVersion),
            VK_VERSION_MINOR(DeviceProps.apiVersion));
}

// ═════════════════════════════════════════════════════════════════════
//  Logical device & queues
// ═════════════════════════════════════════════════════════════════════

void VulkanContext::CreateLogicalDevice() {
  QueueFamilyIndices Indices = FindQueueFamilies(PhysicalDevice);

  std::set<uint32_t> UniqueFamilies = {Indices.GraphicsFamily.value(),
                                       Indices.PresentFamily.value()};

  float Priority = 1.0f;
  std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos;
  for (uint32_t Family : UniqueFamilies) {
    VkDeviceQueueCreateInfo QueueInfo{};
    QueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    QueueInfo.queueFamilyIndex = Family;
    QueueInfo.queueCount = 1;
    QueueInfo.pQueuePriorities = &Priority;
    QueueCreateInfos.push_back(QueueInfo);
  }

  VkPhysicalDeviceFeatures Features{};
  Features.samplerAnisotropy = VK_TRUE;

  VkPhysicalDeviceDynamicRenderingFeatures DynRenderFeature{};
  DynRenderFeature.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
  DynRenderFeature.dynamicRendering = VK_TRUE;

  VkDeviceCreateInfo CreateInfo{};
  CreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  CreateInfo.pNext = &DynRenderFeature;
  CreateInfo.queueCreateInfoCount =
      static_cast<uint32_t>(QueueCreateInfos.size());
  CreateInfo.pQueueCreateInfos = QueueCreateInfos.data();
  CreateInfo.pEnabledFeatures = &Features;
  CreateInfo.enabledExtensionCount =
      static_cast<uint32_t>(kDeviceExtensions.size());
  CreateInfo.ppEnabledExtensionNames = kDeviceExtensions.data();

  if (ValidationEnabled) {
    CreateInfo.enabledLayerCount =
        static_cast<uint32_t>(kValidationLayers.size());
    CreateInfo.ppEnabledLayerNames = kValidationLayers.data();
  }

  CHECK_VK(vkCreateDevice(PhysicalDevice, &CreateInfo, nullptr, &Device),
           "Failed to create logical device");

  vkGetDeviceQueue(Device, Indices.GraphicsFamily.value(), 0, &GraphicsQueue);
  vkGetDeviceQueue(Device, Indices.PresentFamily.value(), 0, &PresentQueue);
}

// ═════════════════════════════════════════════════════════════════════
//  Swapchain
// ═════════════════════════════════════════════════════════════════════

VkSurfaceFormatKHR VulkanContext::ChooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &InFormats) {
  for (const auto &Format : InFormats)
    if (Format.format == VK_FORMAT_B8G8R8A8_SRGB &&
        Format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      return Format;
  return InFormats[0];
}

VkPresentModeKHR VulkanContext::ChooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &InModes) {
  for (auto Mode : InModes)
    if (Mode == PreferredPresentMode) {
      std::cout << "Using preferred present mode: " << Mode << "\n";
      return Mode;
    }
  std::cout << "Using preferred present mode: VK_PRESENT_MODE_FIFO_KHR" << "\n";
  return VK_PRESENT_MODE_FIFO_KHR; // guaranteed available
}

VkExtent2D
VulkanContext::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &InCaps) {
  if (InCaps.currentExtent.width != (std::numeric_limits<uint32_t>::max)())
    return InCaps.currentExtent;

  int W, H;
  glfwGetFramebufferSize(Window, &W, &H);
  VkExtent2D Extent{static_cast<uint32_t>(W), static_cast<uint32_t>(H)};
  Extent.width = std::clamp(Extent.width, InCaps.minImageExtent.width,
                            InCaps.maxImageExtent.width);
  Extent.height = std::clamp(Extent.height, InCaps.minImageExtent.height,
                             InCaps.maxImageExtent.height);
  return Extent;
}

void VulkanContext::CreateSwapchain() {
  auto Support = QuerySwapchainSupport(PhysicalDevice);
  auto Format = ChooseSwapSurfaceFormat(Support.Formats);
  auto Mode = ChooseSwapPresentMode(Support.PresentModes);
  auto Extent = ChooseSwapExtent(Support.Capabilities);

  uint32_t ImageCount = Support.Capabilities.minImageCount + 1;
  if (Support.Capabilities.maxImageCount > 0 &&
      ImageCount > Support.Capabilities.maxImageCount)
    ImageCount = Support.Capabilities.maxImageCount;

  VkSwapchainCreateInfoKHR CreateInfo{};
  CreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  CreateInfo.surface = Surface;
  CreateInfo.minImageCount = ImageCount;
  CreateInfo.imageFormat = Format.format;
  CreateInfo.imageColorSpace = Format.colorSpace;
  CreateInfo.imageExtent = Extent;
  CreateInfo.imageArrayLayers = 1;
  CreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilyIndices Indices = FindQueueFamilies(PhysicalDevice);
  uint32_t FamilyIndices[] = {Indices.GraphicsFamily.value(),
                              Indices.PresentFamily.value()};

  if (Indices.GraphicsFamily != Indices.PresentFamily) {
    CreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    CreateInfo.queueFamilyIndexCount = 2;
    CreateInfo.pQueueFamilyIndices = FamilyIndices;
  } else {
    CreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  CreateInfo.preTransform = Support.Capabilities.currentTransform;
  CreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  CreateInfo.presentMode = Mode;
  CreateInfo.clipped = VK_TRUE;
  CreateInfo.oldSwapchain = VK_NULL_HANDLE;

  CHECK_VK(vkCreateSwapchainKHR(Device, &CreateInfo, nullptr, &Swapchain),
           "Failed to create swapchain");

  vkGetSwapchainImagesKHR(Device, Swapchain, &ImageCount, nullptr);
  SwapchainImages.resize(ImageCount);
  vkGetSwapchainImagesKHR(Device, Swapchain, &ImageCount,
                          SwapchainImages.data());

  SwapchainImageFormat = Format.format;
  SwapchainExtent = Extent;
}

// ═════════════════════════════════════════════════════════════════════
//  Image views
// ═════════════════════════════════════════════════════════════════════

void VulkanContext::CreateImageViews() {
  SwapchainImageViews.resize(SwapchainImages.size());

  for (size_t i = 0; i < SwapchainImages.size(); i++) {
    VkImageViewCreateInfo CreateInfo{};
    CreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    CreateInfo.image = SwapchainImages[i];
    CreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    CreateInfo.format = SwapchainImageFormat;
    CreateInfo.components = {
        VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
    CreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    CreateInfo.subresourceRange.baseMipLevel = 0;
    CreateInfo.subresourceRange.levelCount = 1;
    CreateInfo.subresourceRange.baseArrayLayer = 0;
    CreateInfo.subresourceRange.layerCount = 1;

    CHECK_VK(vkCreateImageView(Device, &CreateInfo, nullptr,
                               &SwapchainImageViews[i]),
             "Failed to create image view");
  }
}

// ═════════════════════════════════════════════════════════════════════
//  Command pool & buffers
// ═════════════════════════════════════════════════════════════════════

void VulkanContext::CreateCommandPool() {
  QueueFamilyIndices Indices = FindQueueFamilies(PhysicalDevice);

  VkCommandPoolCreateInfo CreateInfo{};
  CreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  CreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  CreateInfo.queueFamilyIndex = Indices.GraphicsFamily.value();

  CHECK_VK(vkCreateCommandPool(Device, &CreateInfo, nullptr, &CommandPool),
           "Failed to create command pool");
}

void VulkanContext::AllocateCommandBuffers() {
  CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo AllocInfo{};
  AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  AllocInfo.commandPool = CommandPool;
  AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  AllocInfo.commandBufferCount = static_cast<uint32_t>(CommandBuffers.size());

  CHECK_VK(vkAllocateCommandBuffers(Device, &AllocInfo, CommandBuffers.data()),
           "Failed to allocate command buffers");
}

// ═════════════════════════════════════════════════════════════════════
//  Synchronisation primitives
// ═════════════════════════════════════════════════════════════════════

void VulkanContext::CreateRenderFinishedSemaphores() {
  VkSemaphoreCreateInfo SemaphoreInfo{};
  SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  RenderFinishedSemaphores.resize(SwapchainImages.size());
  for (size_t i = 0; i < SwapchainImages.size(); i++) {
    CHECK_VK(vkCreateSemaphore(Device, &SemaphoreInfo, nullptr,
                               &RenderFinishedSemaphores[i]),
             "Failed to create render-finished semaphore");
  }
}

void VulkanContext::CreateSyncObjects() {
  ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo SemaphoreInfo{};
  SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo FenceInfo{};
  FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  FenceInfo.flags =
      VK_FENCE_CREATE_SIGNALED_BIT; // start signalled so first wait succeeds

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    CHECK_VK(vkCreateSemaphore(Device, &SemaphoreInfo, nullptr,
                               &ImageAvailableSemaphores[i]),
             "Failed to create image-available semaphore");
    CHECK_VK(vkCreateFence(Device, &FenceInfo, nullptr, &InFlightFences[i]),
             "Failed to create in-flight fence");
  }

  // One render-finished semaphore per swapchain image so that a semaphore
  // used in vkQueuePresentKHR is not reused until that image is re-acquired.
  CreateRenderFinishedSemaphores();
}

VkCommandBuffer VulkanContext::BeginSingleTimeCommands() {
  VkCommandBufferAllocateInfo AllocInfo{};
  AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  AllocInfo.commandPool = CommandPool; // il tuo command pool esistente
  AllocInfo.commandBufferCount = 1;

  VkCommandBuffer Cmd;
  vkAllocateCommandBuffers(Device, &AllocInfo, &Cmd);

  VkCommandBufferBeginInfo BeginInfo{};
  BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(Cmd, &BeginInfo);
  return Cmd;
}

void VulkanContext::EndSingleTimeCommands(VkCommandBuffer Cmd) {
  vkEndCommandBuffer(Cmd);

  VkSubmitInfo SubmitInfo{};
  SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  SubmitInfo.commandBufferCount = 1;
  SubmitInfo.pCommandBuffers = &Cmd;

  vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(GraphicsQueue); // aspetta che la GPU finisca

  vkFreeCommandBuffers(Device, CommandPool, 1, &Cmd);
}

void VulkanContext::TransitionImageLayout(VkCommandBuffer Cmd, VkImage Image,
                                          VkImageLayout OldLayout,
                                          VkImageLayout NewLayout) {
  VkImageMemoryBarrier Barrier{};
  Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  Barrier.oldLayout = OldLayout;
  Barrier.newLayout = NewLayout;
  Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  Barrier.image = Image;
  Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  Barrier.subresourceRange.baseMipLevel = 0;
  Barrier.subresourceRange.levelCount = 1;
  Barrier.subresourceRange.baseArrayLayer = 0;
  Barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags SrcStage, DstStage;

  if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {

    Barrier.srcAccessMask = 0;
    Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    DstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

  } else if (OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {

    Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    SrcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    DstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

  } else {
    throw std::runtime_error("Unsupported layout transition");
  }

  vkCmdPipelineBarrier(Cmd, SrcStage, DstStage, 0, 0,
                       nullptr,      // memory barriers
                       0, nullptr,   // buffer barriers
                       1, &Barrier); // image barriers
}

// ═════════════════════════════════════════════════════════════════════
//  Command buffer recording  (clear screen only for Lesson 1)
// ═════════════════════════════════════════════════════════════════════

void VulkanContext::RecordCommandBuffer(
    VkCommandBuffer InCmd, uint32_t InImageIndex,
    std::function<void(VkCommandBuffer)> DrawCallback) {
  VkCommandBufferBeginInfo BeginInfo{};
  BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  CHECK_VK(vkBeginCommandBuffer(InCmd, &BeginInfo),
           "Failed to begin recording command buffer");

  // Transition: UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL
  VkImageMemoryBarrier ToColourBarrier{};
  ToColourBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  ToColourBarrier.srcAccessMask = 0;
  ToColourBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  ToColourBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  ToColourBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  ToColourBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  ToColourBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  ToColourBarrier.image = SwapchainImages[InImageIndex];
  ToColourBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  ToColourBarrier.subresourceRange.baseMipLevel = 0;
  ToColourBarrier.subresourceRange.levelCount = 1;
  ToColourBarrier.subresourceRange.baseArrayLayer = 0;
  ToColourBarrier.subresourceRange.layerCount = 1;

  VkImageMemoryBarrier ToDepthBarrier{};
  if (DepthImageView) {
    ToDepthBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    ToDepthBarrier.srcAccessMask = 0;
    ToDepthBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    ToDepthBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ToDepthBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    ToDepthBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    ToDepthBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    ToDepthBarrier.image = DepthImage;
    ToDepthBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    ToDepthBarrier.subresourceRange.baseMipLevel = 0;
    ToDepthBarrier.subresourceRange.levelCount = 1;
    ToDepthBarrier.subresourceRange.baseArrayLayer = 0;
    ToDepthBarrier.subresourceRange.layerCount = 1;
  }

  VkImageMemoryBarrier Barriers[] = {ToColourBarrier, ToDepthBarrier};
  uint32_t BarrierCount = DepthImageView ? 2 : 1;

  vkCmdPipelineBarrier(InCmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                           VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                           VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                       0, 0, nullptr, 0, nullptr, BarrierCount, Barriers);

  // Dynamic rendering — no VkRenderPass or VkFramebuffer needed
  VkClearValue ClearColour = {{{0.39f, 0.58f, 0.93f, 1.0f}}}; // cornflower blue

  VkRenderingAttachmentInfo ColourAttachment{};
  ColourAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  ColourAttachment.imageView = SwapchainImageViews[InImageIndex];
  ColourAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  ColourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  ColourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  ColourAttachment.clearValue = ClearColour;

  VkRenderingAttachmentInfo DepthAttachment{};
  if (DepthImageView) {
    DepthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    DepthAttachment.imageView = DepthImageView;
    DepthAttachment.imageLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    DepthAttachment.clearValue.depthStencil = {1.f, 0};
  }

  VkRenderingInfo RenderInfo{};
  RenderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  RenderInfo.renderArea.offset = {0, 0};
  RenderInfo.renderArea.extent = SwapchainExtent;
  RenderInfo.layerCount = 1;
  RenderInfo.colorAttachmentCount = 1;
  RenderInfo.pColorAttachments = &ColourAttachment;
  if (DepthImageView) {
    RenderInfo.pDepthAttachment = &DepthAttachment;
  }

  vkCmdBeginRendering(InCmd, &RenderInfo);

  VkViewport Viewport{};
  Viewport.width = static_cast<float>(SwapchainExtent.width);
  Viewport.height = static_cast<float>(SwapchainExtent.height);
  Viewport.minDepth = 0.0f;
  Viewport.maxDepth = 1.0f;
  vkCmdSetViewport(InCmd, 0, 1, &Viewport);

  VkRect2D Scissor{};
  Scissor.extent = SwapchainExtent;
  vkCmdSetScissor(InCmd, 0, 1, &Scissor);

  // Dynamic Draw
  DrawCallback(InCmd);
  // End dynamic rendering
  vkCmdEndRendering(InCmd);

  // Transition: COLOR_ATTACHMENT_OPTIMAL -> PRESENT_SRC_KHR
  VkImageMemoryBarrier ToPresentBarrier{};
  ToPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  ToPresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  ToPresentBarrier.dstAccessMask = 0;
  ToPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  ToPresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  ToPresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  ToPresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  ToPresentBarrier.image = SwapchainImages[InImageIndex];
  ToPresentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  ToPresentBarrier.subresourceRange.baseMipLevel = 0;
  ToPresentBarrier.subresourceRange.levelCount = 1;
  ToPresentBarrier.subresourceRange.baseArrayLayer = 0;
  ToPresentBarrier.subresourceRange.layerCount = 1;

  vkCmdPipelineBarrier(InCmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &ToPresentBarrier);

  CHECK_VK(vkEndCommandBuffer(InCmd), "Failed to record command buffer");
}

// ═════════════════════════════════════════════════════════════════════
//  Swapchain recreation  (handles window resize / minimise)
// ═════════════════════════════════════════════════════════════════════

void VulkanContext::CleanupSwapchain() {
  if (DepthImageView)
    vkDestroyImageView(Device, DepthImageView, nullptr);
  if (DepthImage)
    vkDestroyImage(Device, DepthImage, nullptr);
  if (DepthImageMemory)
    vkFreeMemory(Device, DepthImageMemory, nullptr);

  for (auto Sem : RenderFinishedSemaphores)
    vkDestroySemaphore(Device, Sem, nullptr);
  for (auto Iv : SwapchainImageViews)
    vkDestroyImageView(Device, Iv, nullptr);
  vkDestroySwapchainKHR(Device, Swapchain, nullptr);
}

void VulkanContext::RecreateSwapchain() {
  // Handle minimisation — wait until the window has a non-zero size again
  int W = 0, H = 0;
  glfwGetFramebufferSize(Window, &W, &H);
  while (W == 0 || H == 0) {
    glfwGetFramebufferSize(Window, &W, &H);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(Device);

  CleanupSwapchain();
  CreateSwapchain();
  CreateImageViews();
  CreateDepthResources();
  CreateRenderFinishedSemaphores();
}

// 0 1 0 0 0 0 1 ....
uint32_t VulkanContext::FindMemoryType(uint32_t InTypeFilter,
                                       VkMemoryPropertyFlags InProps) {
  VkPhysicalDeviceMemoryProperties MemProps;
  vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &MemProps);

  for (uint32_t i = 0; i < MemProps.memoryTypeCount; ++i) {
    if ((InTypeFilter & (1 << i)) &&
        (MemProps.memoryTypes[i].propertyFlags & InProps) == InProps)
      return i;
  }

  DIE("Failed to find suitable memory type");
  return 0;
}

void VulkanContext::CreateDepthResources() {

  VkFormat DepthFormat = VK_FORMAT_D32_SFLOAT;

  VkImageCreateInfo ImageInfo{};
  ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  ImageInfo.imageType = VK_IMAGE_TYPE_2D;
  ImageInfo.format = DepthFormat;
  ImageInfo.extent = {SwapchainExtent.width, SwapchainExtent.height, 1};
  ImageInfo.mipLevels = 1;
  ImageInfo.arrayLayers = 1;
  ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  ImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  CHECK_VK(vkCreateImage(Device, &ImageInfo, nullptr, &DepthImage),
           "Failed to create depth image");

  VkMemoryRequirements MemReqs;
  vkGetImageMemoryRequirements(Device, DepthImage, &MemReqs);

  VkMemoryAllocateInfo AllocInfo{};
  AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  AllocInfo.allocationSize = MemReqs.size;
  AllocInfo.memoryTypeIndex = FindMemoryType(
      MemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  CHECK_VK(vkAllocateMemory(Device, &AllocInfo, nullptr, &DepthImageMemory),
           "Failed to allocate depth image memory");
  vkBindImageMemory(Device, DepthImage, DepthImageMemory, 0);

  VkImageViewCreateInfo ViewInfo{};
  ViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  ViewInfo.image = DepthImage;
  ViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  ViewInfo.format = DepthFormat;
  ViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  ViewInfo.subresourceRange.baseMipLevel = 0;
  ViewInfo.subresourceRange.levelCount = 1;
  ViewInfo.subresourceRange.baseArrayLayer = 0;
  ViewInfo.subresourceRange.layerCount = 1;
  CHECK_VK(vkCreateImageView(Device, &ViewInfo, nullptr, &DepthImageView),
           "Failed to create depth image view");
}

void VulkanContext::CreateTextureSampler() {
  VkSamplerCreateInfo SamplerInfo{};
  SamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  SamplerInfo.magFilter = VK_FILTER_LINEAR;
  SamplerInfo.minFilter = VK_FILTER_LINEAR;
  SamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  SamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  SamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  SamplerInfo.anisotropyEnable = VK_TRUE;
  SamplerInfo.maxAnisotropy = 16.0f;
  SamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  SamplerInfo.unnormalizedCoordinates = VK_FALSE;
  SamplerInfo.compareEnable = VK_FALSE;
  SamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

  CHECK_VK(vkCreateSampler(Device, &SamplerInfo, nullptr, &TextureSampler),
           "Failed to create texture sampler");
}

void VulkanContext::CreateDescriptorPool() {
  constexpr uint32_t MaxDescriptors = 100; // Adjust as needed

  VkDescriptorPoolSize PoolSize{};
  PoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  PoolSize.descriptorCount = MaxDescriptors;

  VkDescriptorPoolCreateInfo PoolInfo{};
  PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  PoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  PoolInfo.poolSizeCount = 1;
  PoolInfo.pPoolSizes = &PoolSize;
  PoolInfo.maxSets = MaxDescriptors;

  CHECK_VK(vkCreateDescriptorPool(Device, &PoolInfo, nullptr, &DescriptorPool),
           "Failed to create descriptor pool");
}