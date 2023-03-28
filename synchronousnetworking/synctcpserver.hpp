#ifndef SYNC_TCPSERVER
#define SYNC_TCPSERVER

#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <atomic>
#include <thread>

using namespace boost;
/*
 * Service handles incoming client request.
 *
 * @param: {asio::ip::tcp::socket} &sock: refrence to client socket to process.
 *
 * @behavior: reads from socket and prints clients message to stdout.
 */
class Service {
    public:

        /* Constructor */
        Service(){}

        /* Takes socket and reads message: read_until may throw exception.
         * socket get's deallocted via destrutor from wherever it was initiated from.
         *
         * @param: {asio::ip::tcp::socket} &sock: client socket
         *
         * @behavior: reads clients message or throws Error
         */
        void HandleClient(asio::ip::tcp::socket &sock)
        {
            try
            {
                asio::streambuf buf;
                asio::read_until(sock, buf, '\n');

                std::istream is(&buf);
                std::string request;

                std::getline(is, request);

                std::cout << request << std::endl;
            }
            catch (const system::system_error &ec)
            {
                std::cerr << "Error Occured Handling client: " << ec.code()
                    << "Error Message: " << ec.what() << std::endl;
            }
        }
};

/*
 * A Iterative Transmission Control Protocol synchronous server.
 * Server class accepts clients on given port, listening on any ip4
 * address on host machine. Creates a thread and starts listening for connections,
 * once a connection is accepted class Service is invoked to handle client.
 *
 * @param: {unsigned short} port: port for server to listen on.
 *
 * @behavior: listens for connections and handles client. Due to servers synchronous
 *          behavior will block while handling client request.
 */
class TCPServer {
    private:
        asio::io_service ios;
        asio::ip::tcp::acceptor acceptor;
        const int BACKLOG_SIZE{30};

        std::atomic<bool> stopserver;
        std::unique_ptr<std::thread> thread_;

        /* Start listening for client connections, once accepted pass client socket to
         * service class for processing.
         *
         * @behavior: will block until client request is processed.
         */
        void run()
        {
            while(!stopserver)
            {
                asio::ip::tcp::socket sock(ios);

                // will hang on accept until a new connection is made.
                // when server is signaled to stop.
                acceptor.accept(sock);

                Service srv;
                srv.HandleClient(sock);
            }
        }

    public:

        /* Constructor */
        TCPServer(unsigned short port)
        :acceptor(ios, asio::ip::tcp::endpoint(asio::ip::address_v4::any(), port)),
        stopserver(false)
        {
            acceptor.listen(BACKLOG_SIZE);
        }

        /* Start thread to listen for connections */
        void start()
        {
            thread_.reset(new std::thread([this]()
                        {
                            run();
                        }));
        }

        /* Stop server */
        void stop()
        {
            stopserver.store(true);
            thread_->join();
        }
};
#endif // !SYNC_TCPSERVER
