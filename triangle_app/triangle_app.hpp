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
  const int MAX_FRAMES_IN_FLIGHT = 2;

  AppInfo appInfo_ = {};

  const std::vector<const char *> requiredDeviceExtension_ = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  const std::vector<Vertex> vertices_ = {{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                                         {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
                                         {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}};
  const std::vector<uint16_t> indices_ = {0, 1, 2};

  GLFWwindow *window_ = nullptr;
  vk::raii::Context context_;
  vk::raii::Instance instance_ = nullptr;
  vk::raii::DebugUtilsMessengerEXT debugMessenger_ = nullptr;
  vk::raii::SurfaceKHR surface_ = nullptr;
  vk::raii::PhysicalDevice physicalDevice_ = nullptr;
  vk::raii::Device device_ = nullptr;
  uint32_t queueIndex_ = ~0;
  vk::raii::Queue queue_ = nullptr;

  vk::raii::SwapchainKHR swapChain_ = nullptr;
  std::vector<vk::Image> swapChainImages_;
  vk::SurfaceFormatKHR swapChainSurfaceFormat_;
  vk::Extent2D swapChainExtent_;
  std::vector<vk::raii::ImageView> swapChainImageViews_;

  vk::raii::PipelineLayout pipelineLayout_ = nullptr;
  vk::raii::Pipeline graphicsPipeline_ = nullptr;

  vk::raii::CommandPool commandPool_ = nullptr;
  std::vector<vk::raii::CommandBuffer> commandBuffers_;

  vk::raii::Buffer vertexBuffer_ = nullptr;
  vk::raii::DeviceMemory vertexBufferMemory_ = nullptr;
  vk::raii::Buffer indexBuffer_ = nullptr;
  vk::raii::DeviceMemory indexBufferMemory_ = nullptr;

  vk::raii::Semaphore semaphore_ = nullptr;
  uint64_t timelineValue_ = 0;
  std::vector<vk::raii::Fence> inFlightFences_;
  uint32_t frameIndex_ = 0;

  bool framebufferResized_ = false;

  void initWindow();
  void initVulkan();
  void mainLoop();
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

  void createSyncObjects();

  void drawFrame();

  static void framebufferResizeCallback(GLFWwindow *window, int width,
                                        int height);
  void cleanupSwapChain();
  void recreateSwapChain();

  std::vector<const char *> getRequiredInstanceExtensions() const;
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

  vk::raii::ShaderModule
  createShaderModule(const std::vector<char> &code) const;

  static std::vector<char> readFile(const std::string &filename);

  std::pair<vk::raii::Buffer, vk::raii::DeviceMemory>
  createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
               vk::MemoryPropertyFlags properties) const;
  void copyBuffer(vk::raii::Buffer &srcBuffer, vk::raii::Buffer &dstBuffer,
                  vk::DeviceSize size);
  uint32_t findMemoryType(uint32_t typeFilter,
                          vk::MemoryPropertyFlags properties) const;

  void recordCommandBuffer(uint32_t imageIndex);
  void transitionImageLayout(uint32_t imageIndex, vk::ImageLayout oldLayout,
                             vk::ImageLayout newLayout,
                             vk::AccessFlags2 srcAccessMask,
                             vk::AccessFlags2 dstAccessMask,
                             vk::PipelineStageFlags2 srcStageMask,
                             vk::PipelineStageFlags2 dstStageMask);
};
} // namespace vulkan_app

#endif
