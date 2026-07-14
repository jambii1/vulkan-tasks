#include "triangle_app.hpp"

#include <fstream>
#include <iostream>

vk::VertexInputBindingDescription vulkan_app::Vertex::getBindingDescription() {
  return {.binding = 0,
          .stride = sizeof(Vertex),
          .inputRate = vk::VertexInputRate::eVertex};
}

std::array<vk::VertexInputAttributeDescription, 2>
vulkan_app::Vertex::getAttributeDescriptions() {
  return {{{.location = 0,
            .binding = 0,
            .format = vk::Format::eR32G32Sfloat,
            .offset = offsetof(Vertex, pos)},
           {.location = 1,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = offsetof(Vertex, color)}}};
}

void vulkan_app::TriangleApp::run() {
  initWindow();
  initVulkan();
  mainLoop();
  cleanup();
}

void vulkan_app::TriangleApp::initWindow() {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  window_ = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Triangle", nullptr,
                             nullptr);
  glfwSetWindowUserPointer(window_, this);
  glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);
}

void vulkan_app::TriangleApp::initVulkan() {
  createInstance();
  setupDebugMessenger();
  createSurface();
  pickPhysicalDevice();
  createLogicalDevice();

  createSwapChain();
  createImageViews();

  createGraphicsPipeline();

  createCommandPool();

  createVertexBuffer();
  createIndexBuffer();

  createCommandBuffers();

  createSyncObjects();
}

void vulkan_app::TriangleApp::mainLoop() {
  while (!glfwWindowShouldClose(window_)) {
    glfwPollEvents();
    drawFrame();
  }

  device_.waitIdle();
}

void vulkan_app::TriangleApp::cleanup() {
  glfwDestroyWindow(window_);
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

  instance_ = vk::raii::Instance(context_, createInfo);
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
    debugMessenger_ = instance_.createDebugUtilsMessengerEXT(
        debugUtilsMessengerCreateInfoEXT);
  } catch (vk::SystemError &err) {
    std::cout << "Debug messenger not available. Validation layers may not be "
                 "enabled."
              << std::endl;
  }
}

void vulkan_app::TriangleApp::createSurface() {
  VkSurfaceKHR surface;
  if (glfwCreateWindowSurface(*instance_, window_, nullptr, &surface) != 0) {
    throw std::runtime_error("failed to create window surface!");
  }
  surface_ = vk::raii::SurfaceKHR(instance_, surface);
}

void vulkan_app::TriangleApp::pickPhysicalDevice() {
  std::vector<vk::raii::PhysicalDevice> physicalDevices =
      instance_.enumeratePhysicalDevices();
  const auto devIter =
      std::ranges::find_if(physicalDevices, [&](const auto &physicalDevice) {
        return isDeviceSuitable(physicalDevice);
      });
  if (devIter == physicalDevices.end()) {
    throw std::runtime_error("failed to find a suitable GPU!");
  }
  physicalDevice_ = *devIter;

  vk::PhysicalDeviceProperties deviceProperties =
      physicalDevice_.getProperties();
  std::cout << "Selected GPU: " << deviceProperties.deviceName << std::endl;
  std::cout << "API Version: " << VK_VERSION_MAJOR(deviceProperties.apiVersion)
            << "." << VK_VERSION_MINOR(deviceProperties.apiVersion) << "."
            << VK_VERSION_PATCH(deviceProperties.apiVersion) << std::endl;
}

void vulkan_app::TriangleApp::createLogicalDevice() {
  std::vector<vk::QueueFamilyProperties> queueFamilyProperties =
      physicalDevice_.getQueueFamilyProperties();

  for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size();
       qfpIndex++) {
    if ((queueFamilyProperties[qfpIndex].queueFlags &
         vk::QueueFlagBits::eGraphics) &&
        physicalDevice_.getSurfaceSupportKHR(qfpIndex, *surface_)) {
      queueIndex_ = qfpIndex;
      break;
    }
  }
  if (queueIndex_ == ~0) {
    throw std::runtime_error("could not find a queue for graphics and present");
  }

  vk::StructureChain<vk::PhysicalDeviceFeatures2,
                     vk::PhysicalDeviceVulkan13Features,
                     vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
      featureChain = {
          {},
          {.synchronization2 = VK_TRUE, .dynamicRendering = VK_TRUE},
          {.extendedDynamicState = VK_TRUE}};

  float queuePriority = 0.5f;
  vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
      .queueFamilyIndex = queueIndex_,
      .queueCount = 1,
      .pQueuePriorities = &queuePriority};

  vk::DeviceCreateInfo vkDeviceCreateInfo{
      .pNext = &featureChain.get<vk::PhysicalDeviceVulkan13Features>(),
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &deviceQueueCreateInfo,
      .enabledExtensionCount =
          static_cast<uint32_t>(requiredDeviceExtensions_.size()),
      .ppEnabledExtensionNames = requiredDeviceExtensions_.data()};

  device_ = vk::raii::Device(physicalDevice_, vkDeviceCreateInfo);
  queue_ = device_.getQueue(queueIndex_, 0);
}

void vulkan_app::TriangleApp::createSwapChain() {
  vk::SurfaceCapabilitiesKHR surfaceCapabilities =
      physicalDevice_.getSurfaceCapabilitiesKHR(*surface_);
  swapChainExtent_ = chooseSwapExtent(surfaceCapabilities);
  uint32_t minImageCount = chooseSwapMinImageCount(surfaceCapabilities);

  std::vector<vk::SurfaceFormatKHR> availableFormats =
      physicalDevice_.getSurfaceFormatsKHR(*surface_);
  swapChainSurfaceFormat_ = chooseSwapSurfaceFormat(availableFormats);

  std::vector<vk::PresentModeKHR> availablePresentModes =
      physicalDevice_.getSurfacePresentModesKHR(*surface_);
  vk::PresentModeKHR presentMode = chooseSwapPresentMode(availablePresentModes);

  vk::SwapchainCreateInfoKHR swapChainCreateInfo{
      .surface = *surface_,
      .minImageCount = minImageCount,
      .imageFormat = swapChainSurfaceFormat_.format,
      .imageColorSpace = swapChainSurfaceFormat_.colorSpace,
      .imageExtent = swapChainExtent_,
      .imageArrayLayers = 1,
      .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
      .imageSharingMode = vk::SharingMode::eExclusive,
      .preTransform = surfaceCapabilities.currentTransform,
      .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
      .presentMode = presentMode,
      .clipped = VK_TRUE};

  swapChain_ = vk::raii::SwapchainKHR(device_, swapChainCreateInfo);
  swapChainImages_ = swapChain_.getImages();
}

void vulkan_app::TriangleApp::createImageViews() {
  vk::ImageViewCreateInfo imageViewCreateInfo{
      .viewType = vk::ImageViewType::e2D,
      .format = swapChainSurfaceFormat_.format,
      .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
  for (auto &image : swapChainImages_) {
    imageViewCreateInfo.image = image;
    swapChainImageViews_.emplace_back(device_, imageViewCreateInfo);
  }
}

void vulkan_app::TriangleApp::createGraphicsPipeline() {
  vk::raii::ShaderModule shaderModule =
      createShaderModule(readFile("shaders/triangle.spv"));

  vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
      .stage = vk::ShaderStageFlagBits::eVertex,
      .module = shaderModule,
      .pName = "vertMain"};
  vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
      .stage = vk::ShaderStageFlagBits::eFragment,
      .module = shaderModule,
      .pName = "fragMain"};
  vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                      fragShaderStageInfo};

  auto bindingDescription = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescriptions();
  vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &bindingDescription,
      .vertexAttributeDescriptionCount =
          static_cast<uint32_t>(attributeDescriptions.size()),
      .pVertexAttributeDescriptions = attributeDescriptions.data()};

  vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
      .topology = vk::PrimitiveTopology::eTriangleList};

  vk::PipelineViewportStateCreateInfo viewportState{.viewportCount = 1,
                                                    .scissorCount = 1};

  vk::PipelineRasterizationStateCreateInfo rasterizer{
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = vk::PolygonMode::eFill,
      .cullMode = vk::CullModeFlagBits::eBack,
      .frontFace = vk::FrontFace::eClockwise,
      .depthBiasEnable = VK_FALSE,
      .lineWidth = 1.0f};

  vk::PipelineMultisampleStateCreateInfo multisampling{
      .rasterizationSamples = vk::SampleCountFlagBits::e1,
      .sampleShadingEnable = VK_FALSE};

  vk::PipelineColorBlendAttachmentState colorBlendAttachment{
      .blendEnable = VK_FALSE,
      .colorWriteMask =
          vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};

  vk::PipelineColorBlendStateCreateInfo colorBlending{
      .logicOpEnable = VK_FALSE,
      .logicOp = vk::LogicOp::eCopy,
      .attachmentCount = 1,
      .pAttachments = &colorBlendAttachment};

  std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport,
                                                 vk::DynamicState::eScissor};
  vk::PipelineDynamicStateCreateInfo dynamicState{
      .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
      .pDynamicStates = dynamicStates.data()};

  vk::PipelineLayoutCreateInfo pipelineLayoutInfo{.setLayoutCount = 0,
                                                  .pushConstantRangeCount = 0};
  pipelineLayout_ = vk::raii::PipelineLayout(device_, pipelineLayoutInfo);

  vk::StructureChain<vk::GraphicsPipelineCreateInfo,
                     vk::PipelineRenderingCreateInfo>
      pipelineCreateInfoChain = {
          {.stageCount = 2,
           .pStages = shaderStages,
           .pVertexInputState = &vertexInputInfo,
           .pInputAssemblyState = &inputAssembly,
           .pViewportState = &viewportState,
           .pRasterizationState = &rasterizer,
           .pMultisampleState = &multisampling,
           .pColorBlendState = &colorBlending,
           .pDynamicState = &dynamicState,
           .layout = pipelineLayout_,
           .renderPass = nullptr},
          {.colorAttachmentCount = 1,
           .pColorAttachmentFormats = &swapChainSurfaceFormat_.format}};

  graphicsPipeline_ = vk::raii::Pipeline(
      device_, nullptr,
      pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
}

void vulkan_app::TriangleApp::createCommandPool() {
  vk::CommandPoolCreateInfo poolInfo{
      .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
      .queueFamilyIndex = queueIndex_};
  commandPool_ = vk::raii::CommandPool(device_, poolInfo);
}

void vulkan_app::TriangleApp::createCommandBuffers() {
  commandBuffers_.clear();
  vk::CommandBufferAllocateInfo allocInfo{
      .commandPool = *commandPool_,
      .level = vk::CommandBufferLevel::ePrimary,
      .commandBufferCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)};
  commandBuffers_ = vk::raii::CommandBuffers(device_, allocInfo);
}

void vulkan_app::TriangleApp::createVertexBuffer() {
  vk::DeviceSize bufferSize = sizeof(vertices_[0]) * vertices_.size();

  auto [stagingBuffer, stagingBufferMemory] =
      createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
                   vk::MemoryPropertyFlagBits::eHostVisible |
                       vk::MemoryPropertyFlagBits::eHostCoherent);

  void *data = stagingBufferMemory.mapMemory(0, bufferSize);
  memcpy(data, vertices_.data(), (size_t)bufferSize);
  stagingBufferMemory.unmapMemory();

  std::tie(vertexBuffer_, vertexBufferMemory_) =
      createBuffer(bufferSize,
                   vk::BufferUsageFlagBits::eVertexBuffer |
                       vk::BufferUsageFlagBits::eTransferDst,
                   vk::MemoryPropertyFlagBits::eDeviceLocal);

  copyBuffer(stagingBuffer, vertexBuffer_, bufferSize);
}

void vulkan_app::TriangleApp::createIndexBuffer() {
  vk::DeviceSize bufferSize = sizeof(indices_[0]) * indices_.size();

  auto [stagingBuffer, stagingBufferMemory] =
      createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
                   vk::MemoryPropertyFlagBits::eHostVisible |
                       vk::MemoryPropertyFlagBits::eHostCoherent);

  void *data = stagingBufferMemory.mapMemory(0, bufferSize);
  memcpy(data, indices_.data(), (size_t)bufferSize);
  stagingBufferMemory.unmapMemory();

  std::tie(indexBuffer_, indexBufferMemory_) =
      createBuffer(bufferSize,
                   vk::BufferUsageFlagBits::eIndexBuffer |
                       vk::BufferUsageFlagBits::eTransferDst,
                   vk::MemoryPropertyFlagBits::eDeviceLocal);

  copyBuffer(stagingBuffer, indexBuffer_, bufferSize);
}

void vulkan_app::TriangleApp::createSyncObjects() {
  presentCompleteSemaphores_.clear();
  renderFinishedSemaphores_.clear();
  inFlightFences_.clear();

  for (size_t i = 0; i < swapChainImages_.size(); ++i) {
    renderFinishedSemaphores_.emplace_back(device_, vk::SemaphoreCreateInfo());
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    presentCompleteSemaphores_.emplace_back(device_, vk::SemaphoreCreateInfo());
    inFlightFences_.emplace_back(
        device_,
        vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
  }
}

void vulkan_app::TriangleApp::drawFrame() {
  vk::Result fenceResult =
      device_.waitForFences(*inFlightFences_[frameIndex_], VK_TRUE, UINT64_MAX);
  if (fenceResult != vk::Result::eSuccess) {
    throw std::runtime_error("failed to wait for fence");
  }

  auto [result, imageIndex] = swapChain_.acquireNextImage(
      UINT64_MAX, *presentCompleteSemaphores_[frameIndex_], nullptr);
  if (result == vk::Result::eErrorOutOfDateKHR) {
    recreateSwapChain();
    return;
  }
  if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
    throw std::runtime_error("failed to acquire swap chain image");
  }

  device_.resetFences(*inFlightFences_[frameIndex_]);

  {
    commandBuffers_[frameIndex_].reset();
    recordCommandBuffer(imageIndex);

    vk::PipelineStageFlags waitDestinationStageMask(
        vk::PipelineStageFlagBits::eColorAttachmentOutput);
    const vk::SubmitInfo submitInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*presentCompleteSemaphores_[frameIndex_],
        .pWaitDstStageMask = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &*commandBuffers_[frameIndex_],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*renderFinishedSemaphores_[imageIndex]};
    queue_.submit(submitInfo, *inFlightFences_[frameIndex_]);

    const vk::PresentInfoKHR presentInfoKHR{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*renderFinishedSemaphores_[imageIndex],
        .swapchainCount = 1,
        .pSwapchains = &*swapChain_,
        .pImageIndices = &imageIndex};
    result = queue_.presentKHR(presentInfoKHR);
    if ((result == vk::Result::eSuboptimalKHR) ||
        (result == vk::Result::eErrorOutOfDateKHR) || framebufferResized_) {
      framebufferResized_ = false;
      recreateSwapChain();
    }
  }

  frameIndex_ = (frameIndex_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

void vulkan_app::TriangleApp::framebufferResizeCallback(GLFWwindow *window,
                                                        int width, int height) {
  auto app = reinterpret_cast<TriangleApp *>(glfwGetWindowUserPointer(window));
  app->framebufferResized_ = true;
}

void vulkan_app::TriangleApp::cleanupSwapChain() {
  swapChainImageViews_.clear();
  swapChain_ = nullptr;
}

void vulkan_app::TriangleApp::recreateSwapChain() {
  int width = 0, height = 0;
  glfwGetFramebufferSize(window_, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window_, &width, &height);
    glfwWaitEvents();
  }

  device_.waitIdle();

  cleanupSwapChain();
  createSwapChain();
  createImageViews();
}

std::vector<const char *>
vulkan_app::TriangleApp::getRequiredInstanceExtensions() const {
  uint32_t glfwExtensionCount = 0;
  auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

  std::vector<vk::ExtensionProperties> props =
      context_.enumerateInstanceExtensionProperties();
  bool debugUtilsAvailable =
      std::ranges::any_of(props, [](const vk::ExtensionProperties &ep) {
        return std::strcmp(ep.extensionName, vk::EXTDebugUtilsExtensionName) ==
               0;
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
    std::cerr << "validation layer: type " << vk::to_string(type)
              << " msg: " << pCallbackData->pMessage << std::endl;
  }

  return VK_FALSE;
}

bool vulkan_app::TriangleApp::isDeviceSuitable(
    const vk::raii::PhysicalDevice &physicalDevice) const {
  bool supportsVulkan13 =
      physicalDevice.getProperties().apiVersion >= VK_API_VERSION_1_3;

  auto queueFamilies = physicalDevice.getQueueFamilyProperties();
  bool supportsGraphics =
      std::ranges::any_of(queueFamilies, [](const auto &qfp) {
        return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics);
      });

  auto availableDeviceExtensions =
      physicalDevice.enumerateDeviceExtensionProperties();
  bool supportsAllRequiredExtensions = std::ranges::all_of(
      requiredDeviceExtensions_,
      [&availableDeviceExtensions](const auto &requiredDeviceExtension) {
        return std::ranges::any_of(
            availableDeviceExtensions,
            [requiredDeviceExtension](const auto &availableDeviceExtension) {
              return std::strcmp(availableDeviceExtension.extensionName,
                                 requiredDeviceExtension) == 0;
            });
      });

  return supportsVulkan13 && supportsGraphics && supportsAllRequiredExtensions;
}

vk::Extent2D vulkan_app::TriangleApp::chooseSwapExtent(
    const vk::SurfaceCapabilitiesKHR &capabilities) const {
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  }
  int width = 0, height = 0;
  glfwGetFramebufferSize(window_, &width, &height);

  return {std::clamp<uint32_t>(width, capabilities.minImageExtent.width,
                               capabilities.maxImageExtent.width),
          std::clamp<uint32_t>(height, capabilities.minImageExtent.height,
                               capabilities.maxImageExtent.height)};
}

uint32_t vulkan_app::TriangleApp::chooseSwapMinImageCount(
    const vk::SurfaceCapabilitiesKHR &surfaceCapabilities) {
  auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
  if ((0 < surfaceCapabilities.maxImageCount) &&
      (surfaceCapabilities.maxImageCount < minImageCount)) {
    minImageCount = surfaceCapabilities.maxImageCount;
  }

  return minImageCount;
}

vk::SurfaceFormatKHR vulkan_app::TriangleApp::chooseSwapSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR> &availableFormats) {
  const auto formatIt =
      std::ranges::find_if(availableFormats, [](const auto &format) {
        return format.format == vk::Format::eB8G8R8A8Srgb &&
               format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
      });

  return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
}

vk::PresentModeKHR vulkan_app::TriangleApp::chooseSwapPresentMode(
    std::vector<vk::PresentModeKHR> const &availablePresentModes) {
  return std::ranges::any_of(availablePresentModes,
                             [](const vk::PresentModeKHR value) {
                               return vk::PresentModeKHR::eMailbox == value;
                             })
             ? vk::PresentModeKHR::eMailbox
             : vk::PresentModeKHR::eFifo;
}

vk::raii::ShaderModule vulkan_app::TriangleApp::createShaderModule(
    const std::vector<char> &code) const {
  vk::ShaderModuleCreateInfo createInfo{
      .codeSize = code.size(),
      .pCode = reinterpret_cast<const uint32_t *>(code.data())};
  vk::raii::ShaderModule shaderModule{device_, createInfo};

  return shaderModule;
}

std::vector<char>
vulkan_app::TriangleApp::readFile(const std::string &filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("failed to open file");
  }

  std::vector<char> buffer(file.tellg());
  file.seekg(0, std::ios::beg);
  file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
  file.close();

  return buffer;
}

std::pair<vk::raii::Buffer, vk::raii::DeviceMemory>
vulkan_app::TriangleApp::createBuffer(
    vk::DeviceSize size, vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags properties) const {
  vk::BufferCreateInfo bufferInfo{
      .size = size, .usage = usage, .sharingMode = vk::SharingMode::eExclusive};
  vk::raii::Buffer buffer = vk::raii::Buffer(device_, bufferInfo);
  vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
  vk::MemoryAllocateInfo allocInfo{
      .allocationSize = memRequirements.size,
      .memoryTypeIndex =
          findMemoryType(memRequirements.memoryTypeBits, properties)};
  vk::raii::DeviceMemory bufferMemory =
      vk::raii::DeviceMemory(device_, allocInfo);
  buffer.bindMemory(*bufferMemory, 0);

  return {std::move(buffer), std::move(bufferMemory)};
}

void vulkan_app::TriangleApp::copyBuffer(vk::raii::Buffer &srcBuffer,
                                         vk::raii::Buffer &dstBuffer,
                                         vk::DeviceSize size) {
  vk::CommandBufferAllocateInfo allocInfo{.commandPool = *commandPool_,
                                          .level =
                                              vk::CommandBufferLevel::ePrimary,
                                          .commandBufferCount = 1};
  vk::raii::CommandBuffer commandCopyBuffer =
      std::move(device_.allocateCommandBuffers(allocInfo).front());

  commandCopyBuffer.begin(
      {.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
  commandCopyBuffer.copyBuffer(*srcBuffer, *dstBuffer,
                               vk::BufferCopy(0, 0, size));
  commandCopyBuffer.end();

  queue_.submit(vk::SubmitInfo{.commandBufferCount = 1,
                               .pCommandBuffers = &*commandCopyBuffer},
                nullptr);
  queue_.waitIdle();
}

uint32_t vulkan_app::TriangleApp::findMemoryType(
    uint32_t typeFilter, vk::MemoryPropertyFlags properties) const {
  vk::PhysicalDeviceMemoryProperties memProperties =
      physicalDevice_.getMemoryProperties();

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
                                    properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("failed to find suitable memory type");
}

void vulkan_app::TriangleApp::recordCommandBuffer(uint32_t imageIndex) {
  auto &commandBuffer = commandBuffers_[frameIndex_];
  commandBuffer.begin({});

  transitionImageLayout(imageIndex, vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eColorAttachmentOptimal, {},
                        vk::AccessFlagBits2::eColorAttachmentWrite,
                        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                        vk::PipelineStageFlagBits2::eColorAttachmentOutput);

  vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
  vk::RenderingAttachmentInfo attachmentInfo = {
      .imageView = swapChainImageViews_[imageIndex],
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = clearColor};
  vk::RenderingInfo renderingInfo = {
      .renderArea = {.offset = {0, 0}, .extent = swapChainExtent_},
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &attachmentInfo};

  commandBuffer.beginRendering(renderingInfo);
  commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                             *graphicsPipeline_);
  commandBuffer.setViewport(
      0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChainExtent_.width),
                      static_cast<float>(swapChainExtent_.height), 0.0f, 1.0f));
  commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent_));
  commandBuffer.bindVertexBuffers(0, *vertexBuffer_, {0});
  commandBuffer.bindIndexBuffer(
      *indexBuffer_, 0,
      vk::IndexTypeValue<decltype(indices_)::value_type>::value);
  commandBuffer.drawIndexed(static_cast<uint32_t>(indices_.size()), 1, 0, 0, 0);
  commandBuffer.endRendering();

  transitionImageLayout(imageIndex, vk::ImageLayout::eColorAttachmentOptimal,
                        vk::ImageLayout::ePresentSrcKHR,
                        vk::AccessFlagBits2::eColorAttachmentWrite, {},
                        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                        vk::PipelineStageFlagBits2::eBottomOfPipe);
  commandBuffer.end();
}

void vulkan_app::TriangleApp::transitionImageLayout(
    uint32_t imageIndex, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
    vk::AccessFlags2 srcAccessMask, vk::AccessFlags2 dstAccessMask,
    vk::PipelineStageFlags2 srcStageMask,
    vk::PipelineStageFlags2 dstStageMask) {
  vk::ImageMemoryBarrier2 barrier = {
      .srcStageMask = srcStageMask,
      .srcAccessMask = srcAccessMask,
      .dstStageMask = dstStageMask,
      .dstAccessMask = dstAccessMask,
      .oldLayout = oldLayout,
      .newLayout = newLayout,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = swapChainImages_[imageIndex],
      .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor,
                           .baseMipLevel = 0,
                           .levelCount = 1,
                           .baseArrayLayer = 0,
                           .layerCount = 1}};
  vk::DependencyInfo dependencyInfo = {.dependencyFlags = {},
                                       .imageMemoryBarrierCount = 1,
                                       .pImageMemoryBarriers = &barrier};
  commandBuffers_[frameIndex_].pipelineBarrier2(dependencyInfo);
}
