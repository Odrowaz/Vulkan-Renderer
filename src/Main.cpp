#include "CameraManager.h"
#include "GameObject.h"
#include "Material.h"
#include "Mesh.h"
#include "Texture.h"
#include "VulkanContext.h"
#include "VulkanPipeline.h"
#include "XWindow.h"
#include <format>

int main() {
  bool VsyncEnabled = false;

  XWindow Window{800, 600, "Hello Vulkan"};
  Window.SetVSync(VsyncEnabled);

  // Model Pipeline
  VulkanPipeline pipeline("resources/shaders/model.vert.spv",
                          "resources/shaders/model.frag.spv", 10);

  // Camera
  CameraManager::AddMainCamera(60.f, Window.GetAspectRatio(), 0.1f, 100.f);
  CameraManager::AddCamera("Secondary", 60.f, Window.GetAspectRatio(), 0.1f,
                           100.f);

  // Textures
  Texture stormtrooperTexture("resources/textures/stormtrooper.png");
  Texture smileTexture("resources/textures/smile.png");

  // Material
  Material stormtrooperMaterial(
      pipeline,
      {stormtrooperTexture.GetImageView(), smileTexture.GetImageView()},
      {VulkanContext::GetInstance().GetTextureSampler(),
       VulkanContext::GetInstance().GetTextureSampler()});

  // Mesh
  Mesh stormtrooperMesh("resources/models/stormtrooper.obj");
  Mesh suzanneMesh("resources/models/suzanne.obj");

  // Logic Object
  GameObject stormtrooper{stormtrooperMesh, stormtrooperMaterial};
  GameObject suzanne{suzanneMesh, stormtrooperMaterial,
                     CameraManager::GetCamera("Secondary")};

  stormtrooper.Position = glm::vec3(-2.f, -1.f, -6.f);
  suzanne.Position = glm::vec3(2.f, 1.f, -6.f);

  bool buttonPressed = false;
  float Rotation = 0.f;

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

    if (Window.KeyPressed(GLFW_KEY_ESCAPE)) {
      break;
    }

    Window.PollEvents();

    CameraManager::UpdateAspectRatio(Window.GetAspectRatio());
    CameraManager::GetMainCamera().Update(Window, DeltaTime);
    //CameraManager::GetCamera("Secondary").Update(Window, DeltaTime);

    Window.Update([&](VkCommandBuffer InCmd) {
      stormtrooper.Draw(InCmd);
      suzanne.Draw(InCmd);
    });

    if (Window.KeyPressed(GLFW_KEY_V) && !buttonPressed) {
      buttonPressed = true;
      VsyncEnabled = !VsyncEnabled;
      Window.SetVSync(VsyncEnabled);
    } else if (!Window.KeyPressed(GLFW_KEY_V)) {
      buttonPressed = false;
    }
  }

  VulkanContext::GetInstance().WaitIdle();

  return 0;
}
