# ImGuiVulkanHppImage

A demo application for rendering a triangle with Vulkan-Hpp into a docked ImGui window via `vk::Image->Texture->ImGui::Image`, using SDL3 backend.

## Build/run

- Clone recursively
  ```shell
  $ git clone --recurse-submodules git@github.com:khiner/ImGuiVulkanHppImage.git
  ```
- Download and install the latest SDK from https://vulkan.lunarg.com/sdk/home
- Set the `VULKAN_SDK` environment variable.
  For example, I have the following in my `.zshrc` file:
  ```shell
  export VULKAN_SDK="$HOME/VulkanSDK/1.3.261.1/macOS"
  ```
