#pragma once

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#endif


#include <functional>
#include <future>
#include <string>
#include <map>
#include <atomic>

#include "ctpl_threadpool.hpp"
#include "PipelineStage.hpp"
#include "Logger.hpp"


namespace framework
{
    using std::shared_ptr;
    using std::unique_ptr;
    using std::string;
    using std::map;
    using std::shared_future;


    class Pipeline
    {
        struct StageWorker
        {
            StageWorker(const shared_ptr<PipelineStage>& stg) : stage(stg)
            {

            }

            void operator()(int threadId) const
            {
                stage->run();
            }

            shared_ptr<PipelineStage> stage;
        };


    public:
        Pipeline(const string& name = "");
        ~Pipeline();

        void start();
        void stop(const bool waitForStages = true);

        void addStage(shared_ptr<PipelineStage> stage);

        void injectData(const shared_ptr<StageData>& data);

    private:
        std::atomic<PipelineStage::StageId> m_nextStageId;
        string m_name; 
        unique_ptr<ctpl::thread_pool> m_stagePool;
        map<PipelineStage::StageId, shared_ptr<PipelineStage>> m_stages;
        map<PipelineStage::StageId, shared_future<void>> m_stageFutures;
        shared_ptr<Poco::NotificationCenter> m_nc;
    };
}

