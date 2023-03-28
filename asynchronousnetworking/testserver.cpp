#include "asynctcpserver.hpp"

#include <boost/asio/error.hpp>
#include <thread>

int main (int argc, char *argv[])
{
    const unsigned int POOL_SIZE{std::thread::hardware_concurrency()?std::thread::hardware_concurrency():2};

    try
    {
        AsyncTCPServer server;

        // start server
        server.start(8080, POOL_SIZE);

        std::this_thread::sleep_for(std::chrono::seconds(10));

        server.stop();
    }
    catch (system::system_error &e) {
        std::cout << "Error occured! Error code = " << e.code()
            << ". Message: " << e.what();
    }
    return 0;
}
