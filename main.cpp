#include <cstdlib>
#include "window_manager.hpp"
#include <spdlog/spdlog.h>

using ::std::unique_ptr;

int main(int argc, char** argv)
{
    unique_ptr<WindowManager> window_manager = WindowManager::Create();
    if(!window_manager)
    {
        spdlog::error("Failed to initialize window manager.");
        return EXIT_FAILURE;
    }

    window_manager->Run();

    return EXIT_SUCCESS;
}