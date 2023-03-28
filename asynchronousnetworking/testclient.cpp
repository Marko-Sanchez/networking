#include "asynctcpclient.hpp"

#include <boost/asio/error.hpp>
#include <thread>


void handler(unsigned int request_id, const std::string &response, const system::error_code &ec)
{
    if(ec.value() == 0)
    {
        std::cout << "Request #" << request_id << "has completed. Response "
            << response << std::endl;
    }
    else if(ec == asio::error::operation_aborted)
    {
        std::cout << "Request #" << request_id
            << " has been cancelled by user" << std::endl;
    }

    return;
}


int main (int argc, char *argv[])
{

    try
    {
        AsyncTCPClient client(2);

        client.emulateLongComputationOp("127.0.0.1",  8080, handler, 1);
        std::this_thread::sleep_for(std::chrono::seconds(5));


        client.emulateLongComputationOp("127.0.0.1",  8080, handler, 2);
        std::this_thread::sleep_for(std::chrono::seconds(5));
        client.cancelrequest(2);

        client.emulateLongComputationOp("127.0.0.1",  8080, handler, 3);
        std::this_thread::sleep_for(std::chrono::seconds(5));

        client.close();
    }
    catch (system::system_error &e) {
        std::cout << "Error occured! Error code = " << e.code()
            << ". Message: " << e.what();
    }
    return 0;
}
