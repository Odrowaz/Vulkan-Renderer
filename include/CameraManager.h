#pragma once
#include "Camera.h"
#include <stdexcept>
#include <string>
#include <unordered_map>

class CameraManager {
private:
  inline static std::unordered_map<std::string, Camera> CameraList{};
  CameraManager() = delete;

public:
  template <typename... Args>
  static Camera &AddCamera(const std::string &CameraName, Args... cameraCtor) {
    CameraList.insert({CameraName, Camera{cameraCtor...}});
    return CameraList.at(CameraName);
  }
  template <typename... Args> static Camera &AddMainCamera(Args... cameraCtor) {
    return AddCamera("Main", cameraCtor...);
  }
  static Camera &GetMainCamera() { return GetCamera("Main"); }
  static Camera &GetCamera(const std::string &CameraName) {
    auto it = CameraList.find(CameraName);

    if (it != CameraList.end()) {
      return it->second;
    }

    throw std::runtime_error("Camera not found");
  }

  static bool RemoveCamera(const std::string &CameraName) {
    return CameraList.erase(CameraName) > 0;
  }

  static void UpdateAspectRatio(float InAspectRatio) {
    for (auto& camera : CameraList) {
        camera.second.SetAspectRatio(InAspectRatio);
    }
  }
};