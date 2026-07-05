#ifndef TRIANGLE_APP_HPP
#define TRIANGLE_APP_HPP

#include <GLFW/glfw3.h>

namespace vulkan_app {
  class TriangleApp
  {
  public:
    void run();

  private:
    const uint32_t WINDOW_WIDTH = 800;
    const uint32_t WINDOW_HEIGHT = 600;

    GLFWwindow* window = nullptr;

    void initWindow();
    void initVulkan();
    void mainLoop() const;
    void cleanup();
  };
}

#endif
