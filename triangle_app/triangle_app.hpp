#ifndef TRIANGLE_APP_HPP
#define TRIANGLE_APP_HPP

#ifndef VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#endif
#include <vulkan/vulkan_profiles.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <GLFW/glfw3.h>

namespace vulkan_app {
struct AppInfo {
  bool profileSupported = false;
  VpProfileProperties profile;
};

class TriangleApp {
public:
  void run();

private:
  const uint32_t WINDOW_WIDTH = 800;
  const uint32_t WINDOW_HEIGHT = 600;

  GLFWwindow *window = nullptr;
  vk::raii::Context context;
  vk::raii::Instance instance = nullptr;
  vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
  vk::raii::SurfaceKHR surface = nullptr;
  vk::raii::PhysicalDevice physicalDevice = nullptr;
  vk::raii::Device device = nullptr;
  uint32_t queueIndex = ~0;
  vk::raii::Queue queue = nullptr;

  AppInfo appInfo = {};

  const std::vector<const char *> requiredDeviceExtension = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  void initWindow();
  void initVulkan();
  void mainLoop() const;
  void cleanup();

  void createInstance();
  void setupDebugMessenger();
  void createSurface();
  void pickPhysicalDevice();
  void checkFeatureSupport();
  void createLogicalDevice();

  std::vector<const char *> getRequiredInstanceExtensions();
  static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
      vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
      vk::DebugUtilsMessageTypeFlagsEXT type,
      const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData, void *);
  bool isDeviceSuitable(const vk::raii::PhysicalDevice &physicalDevice);
};
} // namespace vulkan_app

#endif
