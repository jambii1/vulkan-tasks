#include "triangle_app.hpp"

void vulkan_app::TriangleApp::run()
{
  initWindow();
  initVulkan();
  mainLoop();
  cleanup();
}

void vulkan_app::TriangleApp::initWindow()
{
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Triangle", nullptr, nullptr);
}

void vulkan_app::TriangleApp::initVulkan()
{}

void vulkan_app::TriangleApp::mainLoop() const
{
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }
}

void vulkan_app::TriangleApp::cleanup()
{
  glfwDestroyWindow(window);

  glfwTerminate();
}
