#ifndef TRIANGLE_APP_HPP
#define TRIANGLE_APP_HPP

#ifndef VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#endif
#include <vulkan/vulkan_profiles.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace vulkan_app {
struct AppInfo {
  bool profileSupported = false;
  VpProfileProperties profile;
};

struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;

  static vk::VertexInputBindingDescription getBindingDescription();
  static std::array<vk::VertexInputAttributeDescription, 2>
  getAttributeDescriptions();
};

class TriangleApp {
public:
  void run();

private:
  const uint32_t WINDOW_WIDTH = 800;
  const uint32_t WINDOW_HEIGHT = 600;

  AppInfo appInfo = {};

  const std::vector<const char *> requiredDeviceExtension = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  const std::vector<Vertex> vertices = {{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                                        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
                                        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}};

  const std::vector<uint16_t> indices = {0, 1, 2};

  GLFWwindow *window = nullptr;
  vk::raii::Context context;
  vk::raii::Instance instance = nullptr;
  vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
  vk::raii::SurfaceKHR surface = nullptr;
  vk::raii::PhysicalDevice physicalDevice = nullptr;
  vk::raii::Device device = nullptr;
  uint32_t queueIndex = ~0;
  vk::raii::Queue queue = nullptr;

  vk::raii::SwapchainKHR swapChain = nullptr;
  std::vector<vk::Image> swapChainImages;
  vk::SurfaceFormatKHR swapChainSurfaceFormat;
  vk::Extent2D swapChainExtent;
  std::vector<vk::raii::ImageView> swapChainImageViews;

  vk::raii::PipelineLayout pipelineLayout = nullptr;
  vk::raii::Pipeline graphicsPipeline = nullptr;

  vk::raii::CommandPool commandPool = nullptr;
  std::vector<vk::raii::CommandBuffer> commandBuffers;

  vk::raii::Buffer vertexBuffer = nullptr;
  vk::raii::DeviceMemory vertexBufferMemory = nullptr;
  vk::raii::Buffer indexBuffer = nullptr;
  vk::raii::DeviceMemory indexBufferMemory = nullptr;

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

  void createSwapChain();
  void createImageViews();

  void createGraphicsPipeline();

  void createCommandPool();
  void createCommandBuffers();

  void createVertexBuffer();
  void createIndexBuffer();

  std::vector<const char *> getRequiredInstanceExtensions();
  static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
      vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
      vk::DebugUtilsMessageTypeFlagsEXT type,
      const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData, void *);
  bool isDeviceSuitable(const vk::raii::PhysicalDevice &physicalDevice) const;

  vk::Extent2D
  chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities) const;
  static uint32_t chooseSwapMinImageCount(
      const vk::SurfaceCapabilitiesKHR &surfaceCapabilities);
  static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<vk::SurfaceFormatKHR> &availableFormats);
  static vk::PresentModeKHR chooseSwapPresentMode(
      std::vector<vk::PresentModeKHR> const &availablePresentModes);

  vk::raii::ShaderModule createShaderModule(const std::vector<char> &code);

  static std::vector<char> readFile(const std::string &filename);

  std::pair<vk::raii::Buffer, vk::raii::DeviceMemory>
  createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
               vk::MemoryPropertyFlags properties);
  void copyBuffer(vk::raii::Buffer &srcBuffer, vk::raii::Buffer &dstBuffer,
                  vk::DeviceSize size);
  uint32_t findMemoryType(uint32_t typeFilter,
                          vk::MemoryPropertyFlags properties);
};
} // namespace vulkan_app

#endif
