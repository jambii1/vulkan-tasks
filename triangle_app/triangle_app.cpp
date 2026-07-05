#include "triangle_app.hpp"
#include <iostream>

void vulkan_app::TriangleApp::run() {
  initWindow();
  initVulkan();
  mainLoop();
  cleanup();
}

void vulkan_app::TriangleApp::initWindow() {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Triangle", nullptr,
                            nullptr);
}

void vulkan_app::TriangleApp::initVulkan() {
  createInstance();
  setupDebugMessenger();
  createSurface();
  pickPhysicalDevice();
  checkFeatureSupport();
  createLogicalDevice();
}

void vulkan_app::TriangleApp::mainLoop() const {
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }
}

void vulkan_app::TriangleApp::cleanup() {
  glfwDestroyWindow(window);

  glfwTerminate();
}

void vulkan_app::TriangleApp::createInstance() {
  constexpr vk::ApplicationInfo appInfo{
      .pApplicationName = "Triangle",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "No engine",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = vk::ApiVersion14};

  auto requiredExtensions = getRequiredInstanceExtensions();

  vk::InstanceCreateInfo createInfo{
      .pApplicationInfo = &appInfo,
      .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
      .ppEnabledExtensionNames = requiredExtensions.data()};

  instance = vk::raii::Instance(context, createInfo);
}

void vulkan_app::TriangleApp::setupDebugMessenger() {
  vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);

  vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
      vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
      vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
      vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

  vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
      .messageSeverity = severityFlags,
      .messageType = messageTypeFlags,
      .pfnUserCallback = &debugCallback};

  try {
    debugMessenger =
        instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
  } catch (vk::SystemError &err) {
    std::cout << "Debug messenger not available. Validation layers may not be "
                 "enabled."
              << std::endl;
  }
}

void vulkan_app::TriangleApp::createSurface() {
  VkSurfaceKHR _surface;
  if (glfwCreateWindowSurface(*instance, window, nullptr, &_surface) != 0) {
    throw std::runtime_error("failed to create window surface!");
  }
  surface = vk::raii::SurfaceKHR(instance, _surface);
}

void vulkan_app::TriangleApp::pickPhysicalDevice() {
  std::vector<vk::raii::PhysicalDevice> physicalDevices =
      instance.enumeratePhysicalDevices();
  const auto devIter =
      std::ranges::find_if(physicalDevices, [&](const auto &physicalDevice) {
        return isDeviceSuitable(physicalDevice);
      });
  if (devIter == physicalDevices.end()) {
    throw std::runtime_error("failed to find a suitable GPU!");
  }
  physicalDevice = *devIter;

  vk::PhysicalDeviceProperties deviceProperties =
      physicalDevice.getProperties();
  std::cout << "Selected GPU: " << deviceProperties.deviceName << std::endl;
  std::cout << "API Version: " << VK_VERSION_MAJOR(deviceProperties.apiVersion)
            << "." << VK_VERSION_MINOR(deviceProperties.apiVersion) << "."
            << VK_VERSION_PATCH(deviceProperties.apiVersion) << std::endl;
}

void vulkan_app::TriangleApp::checkFeatureSupport() {
  appInfo.profile = {VP_KHR_ROADMAP_2022_NAME,
                     VP_KHR_ROADMAP_2022_SPEC_VERSION};

  VkBool32 supported = VK_FALSE;
  VkResult result = vpGetPhysicalDeviceProfileSupport(
      *instance, *physicalDevice, &appInfo.profile, &supported);

  if (!(result == VK_SUCCESS) || !(supported == VK_TRUE)) {
    appInfo.profileSupported = false;
  }

  appInfo.profileSupported = true;
  std::cout << "Using KHR roadmap 2022 profile" << std::endl;
}

void vulkan_app::TriangleApp::createLogicalDevice() {
  std::vector<vk::QueueFamilyProperties> queueFamilyProperties =
      physicalDevice.getQueueFamilyProperties();

  for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size();
       qfpIndex++) {
    if ((queueFamilyProperties[qfpIndex].queueFlags &
         vk::QueueFlagBits::eGraphics) &&
        physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface)) {
      queueIndex = qfpIndex;
      break;
    }
  }
  if (queueIndex == ~0) {
    throw std::runtime_error(
        "could not find a queue for graphics and present -> terminating");
  }

  float queuePriority = 0.5f;
  vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
      .queueFamilyIndex = queueIndex,
      .queueCount = 1,
      .pQueuePriorities = &queuePriority};

  if (!appInfo.profileSupported) {
    throw std::runtime_error(
        "KHR roadmap 2022 profile is not supported -> terminating");
  }

  vk::PhysicalDeviceFeatures2 features2;
  vk::PhysicalDeviceFeatures deviceFeatures{};
  features2.features = deviceFeatures;

  vk::PhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures;
  dynamicRenderingFeatures.dynamicRendering = VK_TRUE;
  features2.pNext = &dynamicRenderingFeatures;

  vk::DeviceCreateInfo vkDeviceCreateInfo{
      .pNext = &features2,
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &deviceQueueCreateInfo,
      .enabledExtensionCount =
          static_cast<uint32_t>(requiredDeviceExtension.size()),
      .ppEnabledExtensionNames = requiredDeviceExtension.data()};

  device = vk::raii::Device(physicalDevice, vkDeviceCreateInfo);
  queue = device.getQueue(queueIndex, 0);
}

std::vector<const char *>
vulkan_app::TriangleApp::getRequiredInstanceExtensions() {
  uint32_t glfwExtensionCount = 0;
  auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

  std::vector<vk::ExtensionProperties> props =
      context.enumerateInstanceExtensionProperties();
  bool debugUtilsAvailable =
      std::ranges::any_of(props, [](const vk::ExtensionProperties &ep) {
        return strcmp(ep.extensionName, vk::EXTDebugUtilsExtensionName) == 0;
      });

  if (debugUtilsAvailable) {
    extensions.push_back(vk::EXTDebugUtilsExtensionName);
  } else {
    std::cout << "VK_EXT_debug_utils extension not available. Validation "
                 "layers may not work."
              << std::endl;
  }

  return extensions;
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL vulkan_app::TriangleApp::debugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    vk::DebugUtilsMessageTypeFlagsEXT type,
    const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData, void *) {
  if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError ||
      severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
    std::cerr << "validation layer: type " << to_string(type)
              << " msg: " << pCallbackData->pMessage << std::endl;
  }

  return vk::False;
}

bool vulkan_app::TriangleApp::isDeviceSuitable(
    const vk::raii::PhysicalDevice &physicalDevice) {
  bool supportsVulkan1_3 =
      physicalDevice.getProperties().apiVersion >= VK_API_VERSION_1_3;

  auto queueFamilies = physicalDevice.getQueueFamilyProperties();
  bool supportsGraphics =
      std::ranges::any_of(queueFamilies, [](const auto &qfp) {
        return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics);
      });

  auto availableDeviceExtensions =
      physicalDevice.enumerateDeviceExtensionProperties();
  bool supportsAllRequiredExtensions = std::ranges::all_of(
      requiredDeviceExtension,
      [&availableDeviceExtensions](const auto &requiredDeviceExtension) {
        return std::ranges::any_of(
            availableDeviceExtensions,
            [requiredDeviceExtension](const auto &availableDeviceExtension) {
              return strcmp(availableDeviceExtension.extensionName,
                            requiredDeviceExtension) == 0;
            });
      });

  return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions;
}
