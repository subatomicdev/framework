#include "Pipeline.hpp"


namespace framework
{
    Pipeline::Pipeline(const string& name) : m_name(name), m_nextStageId(1)
    {
        m_nc = std::make_shared<Poco::NotificationCenter>();
    }


    Pipeline::~Pipeline()
    {
        stop(true);
    }


    bool Pipeline::initialise()
    {
        m_stagePool = std::make_unique<ctpl::thread_pool>(static_cast<int>(m_stages.size()));

        for (auto& stage : m_stages)
        {
            // initialise, pass the notification center so everything uses the same notification center,
            // allowing communication between pipeline and stages.
            stage.second->initialise(stage.first, m_nc);
        }

        return true;
    }


    void Pipeline::start()
    {
        for (auto& stage : m_stages)
        {
            // store the future of the stage worker for when we stop()
            // after this call, the stage::run() is executing in one of the pool's threads
            m_stageFutures[stage.first] = m_stagePool->push(StageWorker{ stage.second }).share();
        }
    }


    void Pipeline::stop(const bool waitForStages)
    {
        logg(m_name + ": waiting for stages to stop");

        if (m_stagePool)
        {
            m_stagePool->clear_queue();
        }        

        for (auto& stage : m_stages)
        {
            stage.second->stopStage();

            //if (waitForStages)
            //{
            //    // check for valid futures because this map is only populated when start() is called
            //    m_stageFutures[stage.first].wait();
            //}
        }

        if (waitForStages)
        {
            for (auto& stageFutures : m_stageFutures)
            {
                if (stageFutures.second.valid())
                    stageFutures.second.wait();
            }
        }
        

        if (m_stagePool)
        {
            m_stagePool->stop(waitForStages);
        }        

        m_nextStageId = 1;
    }


    void Pipeline::addStage(shared_ptr<PipelineStage> stage)
    {
        m_stages.insert(std::make_pair(m_nextStageId.load(), stage));

        ++m_nextStageId;
    }


    void Pipeline::injectData(const shared_ptr<StageData>& data)
    {
        if (!m_stages.empty())
        {
            m_stages.begin()->second->injectData(data);
        }
    }
}

