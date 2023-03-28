#ifndef ASYNC_TCPSERVER
#define ASYNC_TCPSERVER

#include <boost/asio.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/write.hpp>
#include <boost/system/detail/error_code.hpp>
#include <string>
#include <thread>
#include <atomic>
#include <memory>
#include <iostream>

using namespace boost;

class Service
{
    private:
        std::shared_ptr<asio::ip::tcp::socket> m_sock;
        std::string m_response;
        asio::streambuf m_request;

        void onRequestRecieved(const boost::system::error_code &ec, std::size_t bytes_transferred)
        {
            if(ec.value() != 0)
            {
                std::cout << "Error code in Service class ! Error code = " << ec.value()
                    << ". Message: " << ec.message() << std::endl;

                onFinish();
                return;
            }

            // process request
            m_response = processRequest(m_request);

            //write operation
            asio::async_write(*m_sock.get(), asio::buffer(m_response),
                    [this](const system::error_code &ec, std::size_t bytes_transferred)
                    {
                        if(ec.value() != 0)
                        {
                            std::cout << "Error code! Error code = " << ec.value()
                                << ". Message: " << ec.message() << std::endl;
                        }
                    });
        }

        void onResponseSent(const boost::system::error_code &ec, std::size_t bytes_transferred)
        {
            if(ec.value() != 0)
            {
                std::cout << "Error code! Error code = " << ec.value()
                    << ". Message: " << ec.message() << std::endl;
            }

            onFinish();
        }

        std::string processRequest(asio::streambuf &request)
        {
            // parse request and process it
            // emulate operations that block the thread
            std::istream is(&request);
            std::string msg;

            std::getline(is, msg);
            std::cout << msg << std::endl;

            std::string response{"Hello Client\n"};
            return response;
        }

        void onFinish()
        {
            delete this;
        }

    public:

        Service(std::shared_ptr<asio::ip::tcp::socket> sock)
            :m_sock(sock)
        {}

        void startHandling()
        {
            // read from Client
            asio::async_read_until(*m_sock.get(), m_request, '\n',
                    [this](const system::error_code &ec, std::size_t bytes_transferred)
                    {
                        onRequestRecieved(ec, bytes_transferred);
                    });
        }
};

class Acceptor
{
    private:
        asio::io_service &m_ios;
        asio::ip::tcp::acceptor m_acceptor;
        std::atomic<bool> m_isStopped;

        void InitAccept()
        {
            std::shared_ptr<asio::ip::tcp::socket> sock = std::make_shared<asio::ip::tcp::socket>(asio::ip::tcp::socket(m_ios));

            m_acceptor.async_accept(*sock.get(),
                    [this, sock](const system::error_code &ec)
                    {
                        onAccept(ec, sock);
                    });
        }

        void onAccept(const system::error_code &ec, std::shared_ptr<asio::ip::tcp::socket> sock)
        {
            if(ec.value() == 0)
            {
                (new Service(sock)) -> startHandling();
            }
            else
            {
                std::cout << "Error occured! Error code = " << ec.value()
                    << ". Message: " << ec.message() << std::endl;
            }

            // Init next socket to accept, unless stopped
            if(!m_isStopped.load())
            {
                InitAccept();
            }
            else
            {
                m_acceptor.close();
            }
        }

    public:

        Acceptor(asio::io_service &ios, unsigned short port_num):
            m_ios(ios),
            m_acceptor(m_ios, asio::ip::tcp::endpoint(asio::ip::address_v4::any(), port_num)),
            m_isStopped(false)
    {}

        void start()
        {
            m_acceptor.listen(30);
            InitAccept();
        }

        void stop()
        {
            m_isStopped.store(true);
        }
};

class AsyncTCPServer
{
    private:
        asio::io_service m_ios;
        std::unique_ptr<asio::io_service::work> m_work;
        std::unique_ptr<Acceptor> acc;
        std::vector<std::unique_ptr<std::thread>> m_thread_pool;

    public:

        AsyncTCPServer()
        {
            m_work.reset(new asio::io_service::work(m_ios));
        }

        void start(unsigned short port_num, unsigned int thread_pool_size)
        {
            // make sure thread pool size is greater then 0
            if(thread_pool_size == 0 || thread_pool_size > 2 * std::thread::hardware_concurrency())
                thread_pool_size = 2;

            acc.reset(new Acceptor(m_ios, port_num));
            acc->start();

            for(unsigned int i{0}; i < thread_pool_size; ++i)
            {
                std::unique_ptr<std::thread> process(new std::thread([this](){m_ios.run();}));

                m_thread_pool.push_back(std::move(process));
            }
        }

        void stop()
        {
            acc->stop();
            m_ios.stop();

            for(auto &process: m_thread_pool)
            {
                process->join();
            }
        }
};

#endif // !ASYNC_TCPSERVER
