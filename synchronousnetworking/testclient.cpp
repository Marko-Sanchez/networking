#include "synctcpclient.hpp"

int main()
{
    std::string raw_ip_address{"127.0.0.1"};
    unsigned short port_num{8080};
    try
    {
        TCPClient client(raw_ip_address, port_num);
        client.connect();

        std::string msg{"Hi Server\n"};
        client.sendRequest(msg);

        client.close();
    }catch(system::system_error &ec)
    {
        return ec.code().value();
    }

    return 0;
}
