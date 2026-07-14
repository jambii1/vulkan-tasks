#include <iostream>

#include "triangle_app.hpp"

int main() {
  try {
    vulkan_app::TriangleApp app;
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;

    return 1;
  }
}
