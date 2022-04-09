#include "../server/server.hpp"
#include "../core/config.hpp"

int main(int argc, char **argv) {  
    srand(time(0));
    json config = getConfig(argc, argv);
    BambooServer* server = new BambooServer();
    server->run(config);
}

