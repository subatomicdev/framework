#pragma once

#define ASIO_STANDALONE
#include <asio.hpp>

#include <future>
#include <functional>
#include <optional>
#include <vector>

namespace framework
{
    using std::shared_future;
    using std::promise;
    using std::unique_ptr;
    using std::shared_ptr;
    using std::function;
    using std::vector;


    class TcpServer
    {
    public:
        enum class ServerState {Stopped, Stopping, Running, StartFailed};

        TcpServer();
        ~TcpServer() ;

        
        void start(promise<TcpServer::ServerState>& promise, std::function<void(asio::ip::tcp::socket&&)>& connectionHandler, const char* ip, short port);
        void stop();

    private:
        TcpServer(TcpServer&&) = delete;
        TcpServer(TcpServer&) = delete;
        TcpServer& operator=(const TcpServer&) = delete;

        void accept();

    private:
        asio::io_context m_context;
        unique_ptr<asio::ip::tcp::acceptor> m_acceptor;
        shared_future<TcpServer::ServerState> m_contextFuture;
        std::optional<asio::ip::tcp::socket> m_socket;
        TcpServer::ServerState m_state;
        unique_ptr<std::thread> m_serviceThread;
        function<void(asio::ip::tcp::socket&&)> m_connectionHandler;
    };
}