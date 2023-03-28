#include "synctcpserver.hpp"
#include <thread>

int main (int argc, char *argv[])
{

    unsigned short port_num{8080};
    try
    {
        TCPServer server(port_num);
        server.start();

        // sleep for 5 seconds to emulate work
        std::this_thread::sleep_for(std::chrono::seconds(5));

        // if using iterative version, will hang until another connection is made.
        server.stop();

    }catch(system::system_error &ec)
    {
        return ec.code().value();
    }
    return 0;
}
