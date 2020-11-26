#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>

#include <framework/Pipeline.hpp>
#include <framework/Logger.hpp>
#include <framework/TcpServer.hpp>
#include <framework/IntervalTimer.hpp>


using namespace framework;

using std::cout;
using std::endl;
using std::vector;

static size_t dataIndex = 1;


struct Data : public StageData
{
    Data()
    {
        index = ++dataIndex;

        v2.resize(100);
        for (auto& x : v2)
        {
            x = std::make_shared<vector<char>>();
            x->resize(5242880);
        }
    }


    size_t index;
    vector<shared_ptr<vector<char>>> v2;
};


class Stage1 : public PipelineStage
{
public:
    Stage1() : PipelineStage("Stage 1")
    {

    }

    virtual void run() override
    {
        using namespace std::chrono_literals;

        logg(name() + ": running");

        while (!shouldStop())
        {
            dataComplete(std::make_shared<Data>());

            std::this_thread::sleep_for(100ms);            
        }       
    }
};


class Stage2 : public PipelineStage
{
public:
    Stage2() : PipelineStage("Stage 2")
    {

    }

    virtual void run() override
    {
        using namespace std::chrono_literals;

        logg(name() + ": running");

        while (!shouldStop())
        {
            auto data = nextData<Data>(200ms);

            if (data)
            {
                std::stringstream ss;
                ss << name() + ": Data received: " << data->index;

                logg(ss.str());

                dataComplete(data);
            }
        }
    }
};


class Stage3 : public PipelineStage
{
public:
    Stage3() : PipelineStage("Stage 3")
    {

    }

    virtual void run() override
    {
        using namespace std::chrono_literals;

        logg(name() + ": running");

        while (!shouldStop())
        {
            auto data = nextData<Data>(200ms);

            if (data)
            {
                std::stringstream ss;
                ss << name() + ": Data received: " << data->index;

                logg(ss.str());
            }
        }
    }
};



struct Client 
{
    Client(asio::ip::tcp::socket&& sock, const string& clientId) : m_run(false), m_socket(std::move(sock)), m_clientId(clientId)
    {

    }

    ~Client()
    {
        stop();
    }

    bool stopped() const { return m_run.load() == false; }
    string id() const { return m_clientId; }

    void newSession() 
    {
        m_run = true;

        doRead();
    }


    void stop() 
    {
        logg("Client::stop()");

        m_run = false;

        if (m_socket.is_open())
        {
            m_socket.close();
        }

        logg("Client::stop() - done");
    }


    void doRead()
    {
        asio::async_read_until(m_socket, streambuf, '\n', [this](asio::error_code error, std::size_t bytes_transferred)
        {
            if (error)
            {
                logg("Client disconnect");
                stop();
            }
            else
            {
                std::cout << std::istream(&streambuf).rdbuf();
                doRead();
            }
        });
    }

    asio::streambuf streambuf;
    std::atomic_bool m_run;
    asio::ip::tcp::socket m_socket;
    string m_clientId;
};


map<string, shared_ptr<Client>> clients;


void handleNewSession(asio::ip::tcp::socket&& socket)
{
    std::stringstream id;
    id << socket.remote_endpoint().address().to_string() << ":" << socket.remote_endpoint().port();

    auto newClient = clients.emplace(std::make_pair<string, shared_ptr<Client>>(id.str(), std::make_shared<Client>(std::move(socket), id.str())));
    newClient.first->second->newSession();
}


void clientMonitorTick()
{
    for (map<string, shared_ptr<Client>>::iterator it = clients.begin(); it != clients.end() ; )
    {
        if (it->second)
        {
            if (it->second->stopped())
            {
                it = clients.erase(it);
            }
            else
            {
                ++it;
            }
        }        
    }
}


int main(int argc, char ** argv)
{
    using namespace std::chrono_literals;

    //Pipeline p1("Pipe 1");
    //p1.addStage(std::make_shared<Stage1>());
    //p1.addStage(std::make_shared<Stage2>());
    //p1.addStage(std::make_shared<Stage3>());

    //p1.start();
    

    // TCP Server
    string ip = "127.0.0.1";
    if (argc >= 2)
    {
        ip.assign(argv[1]);
    }

    promise<TcpServer::ServerState> serverPromise;
    auto serverStartFuture = serverPromise.get_future();
    
    IntervalTimer clientMonitor(200ms);
    
    TcpServer server;
    
    function<void(asio::ip::tcp::socket&&)> newClientHandler = handleNewSession;
    server.start(serverPromise, newClientHandler, ip.c_str(), 16474);

    if (serverStartFuture.valid() && serverStartFuture.get() == TcpServer::ServerState::Running)
    {
        function<void()> sessionEndHandler = clientMonitorTick;
        clientMonitor.start(sessionEndHandler);

        auto cmdFuture = std::async(std::launch::async, []
        {
            bool run = true;

            while (run)
            {
                string s;
                std::getline(std::cin, s);

                if (s == "stop")
                {
                    run = false;
                }
            }
        });


        if (cmdFuture.valid())
            cmdFuture.wait();
    }
    else
    {
        logg("FATAL server failed ot start");
    }

    
    logg("Stopping Client Monitor");
    clientMonitor.stop();

    logg("Stopping clients");
    for (auto& client : clients)
        client.second->stop();

    clients.clear();

    logg("Stopping TCP Server");
    server.stop();

    return 0;
}
