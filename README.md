# Vulkan Renderer - Coursework Project

A C++/Vulkan renderer started as coursework at AIV, originally provided by the instructor as a single-file example rendering a cube with normal visualization (no texturing, minimal structure).

## My contribution
Building on the base provided, I implemented:
- **Texture mapping**, missing from the original base
- **Helper classes** to wrap Vulkan boilerplate
- **Code restructuring**: moved logic out of the original monolithic setup into separate, focused classes
- **Camera Controls**: added a Perspective Camera class with keybinding for movements

## Tech
- **Language:** C++
- **Graphics API:** Vulkan
- **Build:** CMake + Conan

## Note
This is a fork of the instructor's base repository (`aiv01/2526_PROG3_Vulkan`). The original provided only a minimal monolitich setup (missing the obj loader implementation) example with normals displayed as texture; the work above was added on top of that foundation.

## Setup (VS Code)
In `.vscode/settings.json`:
```json
{
    "cmake.configureSettings": {
        "CMAKE_PROJECT_TOP_LEVEL_INCLUDES": "./conan_provider.cmake"
    },
    "cmake.environment": {
        "VULKAN_SDK": "<path-to-vulkan-sdk>"
    }
}
```
