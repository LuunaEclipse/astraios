#include <cstdlib>
#include <spdlog/spdlog.h>
#include "window_manager.hpp"

using ::std::unique_ptr;

int main(int argc, char** argv) {

  unique_ptr<WindowManager> window_manager = WindowManager::Create();
  if (!window_manager) {
    return EXIT_FAILURE;
  }

  window_manager->Run();

  return EXIT_SUCCESS;
}
