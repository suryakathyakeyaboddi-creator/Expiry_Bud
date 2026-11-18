#include "expirybud/Application.h"

#include <filesystem>
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        const std::filesystem::path dataDirectory =
            argc > 1 ? std::filesystem::path(argv[1]) : std::filesystem::path("data");
        expirybud::CsvDataStore dataStore(dataDirectory);
        expirybud::ConsoleAlertService alertService(std::cout);
        expirybud::Application application(dataStore, alertService, std::cin, std::cout);
        return application.run();
    } catch (const std::exception& error) {
        std::cerr << "Fatal error: " << error.what() << '\n';
        return 1;
    }
}
