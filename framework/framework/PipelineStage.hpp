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
    using std::mutex;

    
    typedef unsigned short StageId;


    //TODO consider std::variant
    class StageData
    {
    public:
        StageData() = default;
        virtual ~StageData() = default;        

        bool isFinalData() const { return m_finalData.load(); }
        void isFinalData(bool final) { m_finalData.store(final); }

    private:
        std::atomic_bool m_finalData;
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


    struct PipelineStageControlNotification : public Poco::Notification
    {
        enum class Control { StagePause, StageRestart };

        PipelineStageControlNotification(const Control c, const size_t stageId, const std::string& stageName) : control(c), stageSendId(stageId), stagename(stageName)
        {

        }

        virtual std::string name() const override
        {
            return stagename;
        }

        size_t stageSendId;
        Control control;
        std::string stagename;
    };


    using namespace std::chrono_literals;


    class PipelineStage
    {
    public:
        

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
            virtual size_t queueSize() const = 0;
        };


        /// We'll start with a std::queue, but later we may wish
        /// to use an array implemented as a ring buffer for more control and potentially more performance.
        struct DataQueue : public StageBuffer
        {
            DataQueue() 
            {

            }

            virtual void add(const shared_ptr<StageData>& d) override
            {
                {
                    std::scoped_lock lock(queueMux);
                    queue.push(d);
                }

                newDataCV.notify_one();
            }


            virtual shared_ptr<StageData> next(const std::chrono::milliseconds& waitMs) override
            {
                shared_ptr<StageData> data;

                std::unique_lock cvLock(newDataCvMux);
                newDataCV.wait_for(cvLock, waitMs);

                if (hasData())
                {
                    std::scoped_lock cvLock(queueMux);
                    data = queue.front();
                    queue.pop();
                }

                return data;
            }


            virtual bool hasData() override
            {
                std::scoped_lock lock(queueMux);
                return !queue.empty();
            }


            virtual size_t queueSize() const override
            {
                std::scoped_lock lock(queueMux);
                return queue.size();
            }

            std::queue<shared_ptr<StageData>> queue;
            mutable std::mutex queueMux;
            std::mutex newDataCvMux;
            std::condition_variable newDataCV;
        };


    protected:        
        enum PauseState { Requested, Paused, PauseEnd };


        PipelineStage(const string& name = "", const BufferType buffType = BufferType::Queue);
        virtual ~PipelineStage();


        bool isPauseRequested() const;
        void enterPause();


    public:

        const string& name() const { return m_name; }
        StageId id() const { return m_id; }

        virtual void initialise(const StageId id, const shared_ptr<Poco::NotificationCenter>& nc);

        virtual void run() = 0;

        virtual void stopStage();

        inline void injectData(const shared_ptr<StageData>& data);

        void handleStageCommand(const Poco::AutoPtr<StageCommand>& pNf);
        void handleStageNotification(const Poco::AutoPtr<PipelineStageControlNotification>& notification);


        size_t queueSize() const { return m_data->queueSize(); }

    protected:

        bool shouldStop();


        void dataComplete(shared_ptr<StageData>&& data)
        {
            const StageId targetStage = (m_id + 1U);    // we assume it's the next stage
            m_nc->postNotification(new StageCommand(targetStage, m_id, std::move(data))); 
        }


        template<class DataT = StageData>
        shared_ptr<DataT> nextData(const std::chrono::milliseconds& waitMs = 100ms)
        {
            return std::dynamic_pointer_cast<DataT>(m_data->next(waitMs));
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

        mutable std::mutex m_muxPause;
        std::condition_variable m_cvPause;
        std::atomic<PauseState> m_pauseState;
    };
}