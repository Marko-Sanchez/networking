#ifndef ASYNC_TCPCLIENT
#define ASYNC_TCPCLIENT

#include <boost/asio.hpp>

#include <boost/system/detail/error_code.hpp>
#include <thread>
#include <mutex>
#include <memory>
#include <iostream>
#include <map>
#include <list>

using namespace boost;

typedef void(*Callback) (unsigned int request_id, const std::string &response, const system::error_code &ec);

/*
 * Structure to hold information on client request.
 */
struct Session
{
    asio::ip::tcp::socket m_sock;
    asio::ip::tcp::endpoint m_ep;
    std::string m_request;
    unsigned int m_id;             // unique ID assigned to the request

    asio::streambuf m_response_buf;
    std::string m_response;

    system::error_code m_ec;
    Callback m_callback;
    bool m_was_cacelled;
    std::mutex m_cancel_gaurd;

    Session(asio::io_service &ios,
            const std::string &raw_ip_address,
            unsigned short port_num,
            const std::string &request,
            unsigned int id,
            Callback callback):
        m_sock(ios),
        m_ep(asio::ip::address::from_string(raw_ip_address), port_num),
        m_request(request),
        m_id(id),
        m_callback(callback),
        m_was_cacelled(false)
    {}
};

/*
 * A Multithreaded Asynchronous Transmission Protocol Client.
 * Client applicaiton creates multiple threads that wait for asynchronous operations,
 * class is noncopyable to avoid having multiple instances of the client. Recives work via
 * function method, that takes in endpoint and function pointer. Creating a object of type Session
 * to continue handling request.
 *
 * @behavior: Starts work event loop and launches multiple threads to run event loop until client signals to stop working.
 *            Uses user provided function to handle asnync callback.
 */
class AsyncTCPClient : public asio::noncopyable {
    private:
        asio::io_service m_ios;
        std::map<int, std::shared_ptr<Session>> m_active_sessions;
        std::mutex m_active_sessions_gaurd;
        std::unique_ptr<asio::io_service::work> m_work;
        std::list<std::unique_ptr<std::thread>> m_threads;

        /*
         * On async complete, function gets called and marks session request as complete.
         * If session has not epired, calls user provided callback function.
         *
         * @param: {std::shared_ptr<Session>} session: struct holds information related to request.
         *
         * @behavior: checks if request has been canceled; otherwise, calls callback function.
         */
        void onRequestComplete(std::shared_ptr<Session> session)
        {
            system::error_code ignored_ec;

            session->m_sock.shutdown(asio::ip::tcp::socket::shutdown_both, ignored_ec);

            std::unique_lock<std::mutex> lock(m_active_sessions_gaurd);

            auto it = m_active_sessions.find(session->m_id);
            if(it != m_active_sessions.end())
                m_active_sessions.erase(it);

            lock.unlock();

            system::error_code ec;

            if(session->m_ec.value() == 0 && session->m_was_cacelled)
                ec = asio::error::operation_aborted;
            else
                 ec = session->m_ec;

            // call the callback provided by user
            session->m_callback(session->m_id, session->m_response, ec);
        }

    public:

        /* Contructor */
        AsyncTCPClient(std::size_t threads)
        {
            // keeps threads running event loop from exiting when no async operation is pending.
            m_work.reset(new asio::io_service::work(m_ios));

            //loop creating all the threads and appending them to the list
            for(std::size_t i = 0; i < threads; ++i)
            {
                std::unique_ptr<std::thread> thread = std::make_unique<std::thread>(std::thread([this] () { m_ios.run();}));

                m_threads.push_back(thread);
            }
        }

        /*
         * Cancels a request session given an ID.
         *
         * @param: {unsigned int} request_id: session ID.
         *
         * @behavior: marks sessions as canceled.
         */
        void cancelrequest(unsigned int request_id)
        {
            std::unique_lock<std::mutex> lock(m_active_sessions_gaurd);

            auto it = m_active_sessions.find(request_id);
            if(it != m_active_sessions.end())
            {
                std::unique_lock<std::mutex> cancel_lock(it->second->m_cancel_gaurd);

                it->second->m_was_cacelled = true;
                it->second->m_sock.cancel();
            }
        }

        /* Closes io_service work, causing all threads to stop looping event loop and joins threads.*/
        void close()
        {
            m_work.reset(NULL);
            for(auto& thread: m_threads)
                thread->join();
        }

        /*
         * Example function meant to test connecting, writing, and reading to server. Acomplished
         * using nested callback functions.
         *
         * @param: {const std::string &} raw_ip_address: servers IP address to connect to.
         *         {unsigned short} port_num: port on which server will be listening on.
         *         {Callback} callback: user provided function pointer to handle callback.
         *         {unsigned int} request_id: request ID.
         *
         * @behavior: creats a new session for request, connects to server, writes to server, then reads from server.
         */
        void emulateLongComputationOp(const std::string &raw_ip_address,
                                      unsigned short port_num, Callback callback, unsigned int request_id)
        {

            std::string request{"Hello Server\n"};
            std::shared_ptr<Session> session = std::make_shared<Session>(m_ios, raw_ip_address,
                                                                       port_num, request, request_id, callback);

            session->m_sock.open(session->m_ep.protocol());

            // add new session
            std::unique_lock<std::mutex> lock(m_active_sessions_gaurd);
            m_active_sessions[request_id] = session;
            lock.unlock();

            // simulate reading and writing from server
            session->m_sock.async_connect(session->m_ep,
                    [this, session](const system::error_code &ec)
                    {
                        if(ec.value() != 0)
                        {
                            session->m_ec = ec;
                            onRequestComplete(session);
                            return;
                        }

                        std::unique_lock<std::mutex> cancel_lock(session->m_cancel_gaurd);

                        if(session->m_was_cacelled)
                        {
                            onRequestComplete(session);
                            return;
                        }

                        asio::async_write(session->m_sock, asio::buffer(session->m_request),
                                [this, session](const system::error_code &ec, std::size_t bytes_transferred)
                                {
                                    if(ec.value() != 0)
                                    {
                                        session->m_ec = ec;
                                        onRequestComplete(session);
                                        return;
                                    }

                                    std::unique_lock<std::mutex> cancel_lock(session->m_cancel_gaurd);
                                    if(session->m_was_cacelled)
                                    {
                                        onRequestComplete(session);
                                        return;
                                    }

                                    asio::async_read_until(session->m_sock, session->m_response_buf, '\n',
                                            [this, session](const system::error_code &ec, std::size_t bytes_transferred)
                                            {
                                                if(ec.value() != 0)
                                                {
                                                    session->m_ec = ec;
                                                }
                                                else
                                                {
                                                   std::istream is(&session->m_response_buf);
                                                   std::getline(is, session->m_response);
                                                }

                                                onRequestComplete(session);
                                            });
                                });
                    });
        }
};
 #endif // !ASYNC_TCPCLIENTTCPCLIENT
