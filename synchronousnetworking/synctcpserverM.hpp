#pragma once

#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <atomic>
#include <thread>

using namespace boost;

/*
 * Service handles incoming client request.
 *
 * @behavior: creates detached thread, reads from socket and prints clients message to stdout.
 */
class Service_M {
    private:

        /* Takes socket and reads message: read_until may throw exception.
         * socket get's deallocted via destrutor from wherever it was initiated from.
         *
         * @param: {shared_ptr<asio::ip::tcp::socket>} sock: shared pointer to client socket.
         *
         * @behavior: reads clients message or throws Error
         */
        void HandleClient(std::shared_ptr<asio::ip::tcp::socket> sock) {

            try {
                asio::streambuf buf;
                asio::read_until(*sock.get(), buf, '\n');

                std::istream is(&buf);
                std::string request;

                std::getline(is, request);

                std::cout << request << std::endl;
            } catch(system::system_error& ec){

            }
            delete this;
        }

    public:

        /* Constructor */
        Service_M(){}

        /*
         * Takes shared pointer to socket, invokes thread and detaches thread. Thread is called
         * with function to handle client.
         *
         * @param: {shared_ptr<asio::ip::tcp::socket>} sock: shared pointer to client socket.
         *
         * @behavior: creates thread and detaches.
         */
        void StartHandlingClient(std::shared_ptr<asio::ip::tcp::socket> sock) {
            std::thread thread_(([this, sock]() { HandleClient(sock);}));

            thread_.detach();
        }
};

/*
 * A Multithreaded Transmission Control Protocol synchronous server.
 * Server class accepts clients on given port, listening on any ip4
 * address on host machine. Creates a thread and starts listening for connections,
 * once a connection is accepted class Service is invoked in a new thread to handle client.
 *
 * @param: {unsigned short} port: port for server to listen on.
 *
 * @behavior: listens for connections and handles client in new thread.
 *            Although synchronous in nature, due to multithreading the server
 *            can continue to process clients.
 */
class TCPServer_M {
    private:
        asio::io_service ios;
        asio::ip::tcp::acceptor acceptor;
        const int BACKLOG_SIZE{30};

        std::atomic<bool> stopserver;
        std::unique_ptr<std::thread> thread_;

        /* Start listening for client connections, once accepted pass client socket to
         * service class for processing.
         *
         * @behavior: accepts connection and launches a new thread to handle client.
         */
        void run()
        {
            while(!stopserver)
            {
                std::shared_ptr<asio::ip::tcp::socket> sock(new asio::ip::tcp::socket(ios));
                acceptor.accept(*sock.get());

                (new  Service_M)->StartHandlingClient(sock);
            }
        }

    public:

        /* Constructor */
        TCPServer_M(unsigned short port)
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
