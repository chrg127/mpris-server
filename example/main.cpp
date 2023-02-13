#include "../src/mpris_server.hpp"
#include <iostream>

int main()
{
    mpris::Server server{"genericplayer"};
    server.start_loop();
    return 0;
}
