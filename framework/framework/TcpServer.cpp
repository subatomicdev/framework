#include "TcpServer.hpp"
#include "Logger.hpp"
#include <optional>

namespace framework
{
    TcpServer::TcpServer() : m_state(TcpServer::ServerState::Stopped)
    {

    }


    TcpServer::~TcpServer()
    {
        stop();
    }


    void TcpServer::start(promise< TcpServer::ServerState>& promise, std::function<void(asio::ip::tcp::socket&&)>& connectionHandler, const char * ip, short port)
    {
        try
        {            
            m_connectionHandler = connectionHandler;

            m_acceptor = std::make_unique<asio::ip::tcp::acceptor> (m_context, asio::ip::tcp::endpoint(asio::ip::address::from_string(ip), port));
            
            asio::error_code ec;

            m_serviceThread = std::make_unique<std::thread>([&]()
            {
                try
                {
                    m_state = TcpServer::ServerState::Running;
                    promise.set_value(m_state);

                    m_context.run(ec); // sync call

                    if (ec)
                    {
                        m_state = TcpServer::ServerState::StartFailed;
                        promise.set_value(m_state);
                        
                        std::stringstream ss;
                        ss << "code: " << ec.value() << " : " << ec.message();

                        logg(ss.str());
                    }
                    else
                    {
                        // if run() was ok, it'll block, so we'll only arrive here when the server shuts down
                        m_state = TcpServer::ServerState::Stopping;
                        // set to stopping, don't consider fully stopped until stop() is called
                    }
                }
                catch (...)
                {
                    m_state = TcpServer::ServerState::StartFailed;
                    promise.set_value(m_state);

                    std::stringstream ss;
                    ss << ip << ":" << port;
                    
                    logg("TCP Server failed during start(), server on: " + ss.str());
                }
            });


            if (!ec)
            {
                // this is non-blocking: it'll asynchronously wait for a connection
                accept();
            }
        }
        catch (...)
        {
            m_state = TcpServer::ServerState::Stopped;
            promise.set_value(m_state);
        }
    }


    void TcpServer::stop()
    {
        logg("TcpServer::stop()");
        
        m_state = TcpServer::ServerState::Stopping;

        if (!m_context.stopped())
        {
            m_context.stop();
        }

        if (m_serviceThread && m_serviceThread->joinable())
        {
            m_serviceThread->join();
        }

        if (m_acceptor && m_acceptor->is_open())
        {           
            m_acceptor->cancel();
            m_acceptor->close();
        }
        
        if (m_socket && m_socket->is_open())
        {
            m_socket->close();
        }

        m_state = TcpServer::ServerState::Stopped;

        logg("TcpServer::stop() - done");
    }


    void TcpServer::accept()
    {
        m_socket.emplace(m_context);

        m_acceptor->async_accept(*m_socket, [&](asio::error_code error)
        {            
            if (error)
            {
                std::stringstream ss;
                ss << "code: " << error.value() << " : " << error.message();

                logg(ss.str());
            }
            else
            {
                logg("New connection");

                // offload the session to the handler
                m_connectionHandler(std::move(*m_socket));

                // go again for the next connection
                accept(); 
            }            
        });
    }
}