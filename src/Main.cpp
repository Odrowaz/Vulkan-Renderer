#include "CameraManager.h"
#include "GameObject.h"
#include "Material.h"
#include "Mesh.h"
#include "Texture.h"
#include "VulkanContext.h"
#include "VulkanPipeline.h"
#include "XWindow.h"
#include <format>
#include <glm/gtc/matrix_transform.hpp>

int main() {
  bool VsyncEnabled = false;

  XWindow Window{800, 600, "Hello Vulkan"};
  Window.SetVSync(VsyncEnabled);

  // Model Pipeline
  VulkanPipeline pipeline("resources/shaders/model.vert.spv",
    "resources/shaders/model.frag.spv", 10);

  //Camera
  CameraManager::AddMainCamera(60.f, Window.GetAspectRatio(), 0.1f, 100.f);
  CameraManager::AddCamera("Secondary", 60.f, Window.GetAspectRatio(), 0.1f, 100.f);

  // Textures
  Texture stormtrooperTexture("resources/textures/stormtrooper.png");
  Texture smileTexture("resources/textures/smile.png");

  // Material
  Material stormtrooperMaterial(
      pipeline,
      {stormtrooperTexture.GetImageView(), smileTexture.GetImageView()},
      {VulkanContext::GetInstance().GetTextureSampler(), VulkanContext::GetInstance().GetTextureSampler()});
      
  // Mesh
  Mesh stormtrooperMesh("resources/models/stormtrooper.obj");
  Mesh suzanneMesh("resources/models/suzanne.obj");

  // Logic Object
  GameObject stormtrooper{stormtrooperMesh, stormtrooperMaterial};
  GameObject suzanne{suzanneMesh, stormtrooperMaterial, CameraManager::GetCamera("Secondary")};

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
      
    CameraManager::UpdateAspectRatio(Window.GetAspectRatio());

    if( Window.KeyPressed(GLFW_KEY_ESCAPE)) {
      break;
    }

    glm::vec3 Forward;
    Forward.x = cos(glm::radians(CameraManager::GetMainCamera().Rotation.x)) * sin(glm::radians(CameraManager::GetMainCamera().Rotation.y));
    Forward.y = sin(glm::radians(CameraManager::GetMainCamera().Rotation.x));
    Forward.z = -cos(glm::radians(CameraManager::GetMainCamera().Rotation.x)) * cos(glm::radians(CameraManager::GetMainCamera().Rotation.y));

    glm::vec3 Right = glm::normalize(glm::cross(Forward, glm::vec3(0.f, 1.f, 0.f)));

    if(Window.KeyPressed(GLFW_KEY_W)) {
      CameraManager::GetMainCamera().Position += Forward * DeltaTime * 5.f;
    }
    else if(Window.KeyPressed(GLFW_KEY_S)) {
      CameraManager::GetMainCamera().Position -= Forward * DeltaTime * 5.f;
    }
    if(Window.KeyPressed(GLFW_KEY_A)) {
      CameraManager::GetMainCamera().Position -= Right * DeltaTime * 5.f;
    }
    else if(Window.KeyPressed(GLFW_KEY_D)) {
      CameraManager::GetMainCamera().Position += Right * DeltaTime * 5.f;
    }

    if(Window.KeyPressed(GLFW_KEY_UP)) {
      CameraManager::GetMainCamera().Rotation.x += 50.f * DeltaTime;
    }
    else if(Window.KeyPressed(GLFW_KEY_DOWN)) {
      CameraManager::GetMainCamera().Rotation.x -= 50.f * DeltaTime;
    }
    if(Window.KeyPressed(GLFW_KEY_LEFT)) {
      CameraManager::GetMainCamera().Rotation.y -= 50.f * DeltaTime;
    }
    else if(Window.KeyPressed(GLFW_KEY_RIGHT)) {
      CameraManager::GetMainCamera().Rotation.y += 50.f * DeltaTime;
    }

    //stormtrooper.Update(DeltaTime);
    //suzanne.Update(DeltaTime);

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
