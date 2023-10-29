# ImGuiVulkanHppImage

A demo application for rendering a triangle with Vulkan-Hpp into a docked ImGui window via `Shaders->vk::Image->MSAA->Texture->ImGui::Image`, with an SDL3 backend.

![](DemoTriangle.png)

Features:
- Recreate sampler & texture every time the docked scene window size changes.
- Reuse as much as possible between re-renders. (E.g. dynamic command buffer, viewport and scissor states.)
- Multisample anti-aliasing (MSAA), using the max supported samples.
- Minimal, condensed, modern C++ Vulkan-HPP code.
- No dependencies other than the Vulkan SDK, ImGui, and SDL3.

## Clone/build/run

- Clone recursively
  ```shell
  $ git clone --recurse-submodules git@github.com:khiner/ImGuiVulkanHppImage.git
  ```
- Download and install the latest SDK from https://vulkan.lunarg.com/sdk/home
- Set the `VULKAN_SDK` environment variable.
  For example, add the following to your `.zshrc` file:
  ```shell
  export VULKAN_SDK="$HOME/VulkanSDK/{version}/macOS"
  ```
- Build/Run
  ```shell
  $ mkdir build && cd build && cmake ..
  $ make
  $ ./ImGuiVulkanHppImage
  ```
