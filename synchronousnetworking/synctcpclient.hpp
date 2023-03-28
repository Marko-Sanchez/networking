#ifndef SYNC_TCPCLIENT
#define SYNC_TCPCLIENT

#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

/*
 * A Synchronous Transmission Control Protocol Client.
 * Connects to a Synchronous TCP server, on a given port.
 *
 * @behavior: connects to server and reads or writes messages.
 */
class TCPClient {
    private:
        asio::io_service ios;
        asio::ip::tcp::endpoint ep;
        asio::ip::tcp::socket sock;

    public:

        /* Constructor, opens socket on endpoint. */
        TCPClient(std::string ip, unsigned short port)
        :ep(asio::ip::address::from_string(ip), port),
        sock(ios)
        {
            sock.open(ep.protocol());
        }

        /* Connect to endpoint. */
        void connect()
        {
            sock.connect(ep);
        }

        /* Shutsdown sockets write ability signaling server and closes socket. */
        void close()
        {
            sock.shutdown(asio::socket_base::shutdown_both);
            sock.close();
        }

        /* Sends message to server ending with a newline character. */
        void sendRequest(const std::string & request)
        {
            asio::write(sock, asio::buffer(request));
        }

        /* Reads from socket until a newline character is encountered. */
        std::string receiveRequest()
        {
            asio::streambuf buf;
            asio::read_until(sock, buf, '\n');

            std::istream is(&buf);
            std::string response;

            std::getline(is, response);

            return response;
        }
};
#endif // !SYNC_TCPCLIENT
