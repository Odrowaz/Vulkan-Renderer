#include "Material.h"
#include "Mesh.h"
#include "Texture.h"
#include "VulkanPipeline.h"
#include "XWindow.h"
#include <format>

int main() {
  bool VsyncEnabled = false;
  VulkanContext Context;
  XWindow Window{800, 600, "Hello Vulkan", Context};
  Window.SetVSync(VsyncEnabled);

  Texture stormtrooperTexture(Context, "resources/textures/stormtrooper.png");
  Texture smileTexture(Context, "resources/textures/smile.png");

  Mesh stormtrooperMesh(Context, "resources/models/stormtrooper.obj");

  VulkanPipeline ModelPipeline(Context, "resources/shaders/model.vert.spv",
                               "resources/shaders/model.frag.spv", 10);

  Material stormtrooperMaterial(
      Context, ModelPipeline.GetDescriptorSetLayout(),
      {stormtrooperTexture.GetImageView(), smileTexture.GetImageView()},
      {Context.GetTextureSampler(), Context.GetTextureSampler()});

  bool buttonPressed = false;

  while (Window.IsOpened()) {
    static float TitleUpdateMaxTime = 1.f;
    static float TitleUpdateElapsed = 0.f;
    static int FrameCount = 0;

    float DeltaTime = Window.GetDeltaTime();
    TitleUpdateElapsed += DeltaTime;
    FrameCount++;

    if (TitleUpdateElapsed >= TitleUpdateMaxTime) {
      float AvgFps = FrameCount / TitleUpdateElapsed;
      std::string Title = std::format("Vulkan App | DeltaTime: {} - FPS: {}",
                                      DeltaTime, static_cast<int>(AvgFps));
      Window.SetTitle(Title);
      TitleUpdateElapsed -= TitleUpdateMaxTime;
      FrameCount = 0;
    }

    Window.Update([&](VkCommandBuffer InCmd) {
      ModelPipeline.Draw(InCmd, stormtrooperMesh, stormtrooperMaterial,
                         Context.GetSwapchainExtent(), DeltaTime);
    });

    if (Window.KeyPressed(GLFW_KEY_V) && !buttonPressed) {
      buttonPressed = true;
      VsyncEnabled = !VsyncEnabled;
      Window.SetVSync(VsyncEnabled);
    } else if (!Window.KeyPressed(GLFW_KEY_V)) {
      buttonPressed = false;
    }
  }

  Context.WaitIdle();

  return 0;
}
