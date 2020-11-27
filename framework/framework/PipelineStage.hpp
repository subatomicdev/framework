#pragma once

#include <functional>
#include <future>
#include <string>
#include <map>
#include <atomic>
#include <queue>
#include <condition_variable>
#include <chrono>

#include <Poco/NotificationCenter.h>
#include <Poco/Notification.h>
#include <Poco/NObserver.h>
#include <Poco/AutoPtr.h>


namespace framework
{
    using std::shared_ptr;
    using std::string;
    using std::map;
    using std::queue;
    

    //TODO consider std::variant
    class StageData
    {
    public:
        StageData() = default;
        virtual ~StageData() = default;        
    };


    using namespace std::chrono_literals;


    class PipelineStage
    {
    public:
        typedef unsigned short StageId;

    private:

        // prefer this to templating PipelineStage, i.e. with the buffer as template argument
        enum class BufferType { Queue };

        

        /// Interface which provides a FIFO buffer for stage data. 
        /// Implementions must be thread safe.
        struct StageBuffer
        {
            virtual void initialise() {};
            virtual void add(const shared_ptr<StageData>& d) = 0;
            virtual shared_ptr<StageData> next(const std::chrono::milliseconds& waitMs) = 0;
            virtual bool hasData() = 0;
        };


        /// We'll start with a std::queue, but later we may wish
        /// to use an array implemented as a ring buffer for more control and potentially more performance.
        struct DataQueue : public StageBuffer
        {
            virtual void add(const shared_ptr<StageData>& d) override
            {
                {
                    std::scoped_lock lock(mux);
                    queue.push(d);
                }

                cv.notify_one();
            }


            virtual shared_ptr<StageData> next(const std::chrono::milliseconds& waitMs) override
            {
                shared_ptr<StageData> data;

                std::unique_lock cvLock(cvMux);
                cv.wait_for(cvLock, waitMs);

                if (hasData())
                {
                    std::scoped_lock cvLock(mux);
                    data = queue.front();
                    queue.pop();
                }

                return data;
            }


            virtual bool hasData() override
            {
                std::scoped_lock lock(mux);
                return !queue.empty();
            }


            std::queue<shared_ptr<StageData>> queue;
            std::mutex mux, cvMux;
            std::condition_variable cv;
        };


        struct StageCommand : public Poco::Notification
        {
            enum class Command { CommandNone, CommandDataAvailable };

            StageCommand(const Command c = Command::CommandNone) : cmd(c), senderStageId(0), targetStageId(0), data(nullptr)
            {

            }

            StageCommand(const StageId targetId, const StageId senderId, const shared_ptr<StageData>& d)
                :   cmd(Command::CommandDataAvailable),
                    targetStageId(targetId),
                    senderStageId(senderId),
                    data(d)
            {

            }

            Command cmd;
            StageId senderStageId;
            StageId targetStageId;
            shared_ptr<StageData> data;
        };


    protected:
        PipelineStage(const string& name = "", const BufferType buffType = BufferType::Queue);

        virtual ~PipelineStage();


    public:
        

        const string& name() const { return m_name; }

        virtual void initialise(const StageId id, const shared_ptr<Poco::NotificationCenter>& nc);

        virtual void run() = 0;

        virtual void stopStage();

        inline void injectData(const shared_ptr<StageData>& data);

        void handleStageCommand(const Poco::AutoPtr<StageCommand>& pNf);


    protected:

        bool shouldStop();


        void dataComplete(const shared_ptr<StageData>& data)
        {
            auto future = std::async(std::launch::async, [this, &data]
            {
                const StageId targetStage = (m_id + 1U);    // we assume it's the next stage
                m_nc->postNotification(new StageCommand (targetStage, m_id, data));
            });
        }


        template<class DataT = StageData>
        shared_ptr<DataT> nextData(const std::chrono::milliseconds& waitMs = 100ms)
        {
            shared_ptr<DataT> data;

            if (m_data->hasData())
            {
                data = std::dynamic_pointer_cast<DataT>(m_data->next(waitMs));
            }

            return data;
        }


        shared_ptr<StageData> nextData(const std::chrono::milliseconds& waitMs = 100ms)
        {
            return nextData<>(waitMs);
        }




    private:
        string m_name;
        StageId m_id;
        std::atomic_bool m_stopRequest;
        std::unique_ptr<StageBuffer> m_data;
        shared_ptr<Poco::NotificationCenter> m_nc;
    };
}