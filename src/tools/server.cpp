#include "../server/server.hpp"
#include "../core/config.hpp"
#include "spdlog/spdlog.h"

int main(int argc, char **argv) {  
    srand(time(0));
    spdlog::set_level(spdlog::level::debug); // Set global log level to debug
    json config = getConfig(argc, argv);
    spdlog::info("Starting server");
    PandaniteServer* server = new PandaniteServer();
    server->run(config);
}

