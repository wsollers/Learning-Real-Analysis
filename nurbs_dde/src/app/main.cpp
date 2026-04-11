// app/main.cpp
#include "app/Application.hpp"
#include <iostream>
#include <stdexcept>
#include <cstdlib>

int main() {
    try {
        ndde::app::Application app;
        app.run();
        return EXIT_SUCCESS;
    }
    catch (const std::exception& e) {
        std::cerr << "[FATAL] " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    catch (...) {
        std::cerr << "[FATAL] Unknown exception\n";
        return EXIT_FAILURE;
    }
}
