#include "PipelineStage.hpp"


namespace framework
{
    PipelineStage::PipelineStage(const string& name, const BufferType buffType) : m_name(name), m_id(0), m_stopRequest(false)
    {
        if (buffType == BufferType::Queue)
        {
            m_data = std::make_unique<DataQueue>();
        }
        else
        {
            throw std::runtime_error("Invalid stage buffer type");
        }
    }


    PipelineStage::~PipelineStage()
    {
        if (m_nc)
        {
            m_nc->removeObserver(Poco::NObserver<PipelineStage, StageCommand>(*this, &PipelineStage::handleStageCommand));
        }        
    }

    
    void PipelineStage::initialise(const StageId id, const shared_ptr<Poco::NotificationCenter>& nc)
    {
        m_id = id;

        m_nc = nc;
        m_nc->addObserver(Poco::NObserver<PipelineStage, StageCommand>(*this, &PipelineStage::handleStageCommand));
    }


    void PipelineStage::injectData(const shared_ptr<StageData>& data)
    {
        m_data->add(data);
    }
    

    void PipelineStage::handleStageCommand(const Poco::AutoPtr<StageCommand>& pCommand)
    {
        if (pCommand->targetStageId == m_id)
        {
            switch (pCommand->cmd)
            {
            case StageCommand::Command::CommandDataAvailable:
                injectData(pCommand->data);
                break;

            default:
                // ignore
                break;
            }
        }
    }


    void PipelineStage::stopStage()
    {
        m_stopRequest.store(true);
    }


    bool PipelineStage::shouldStop()
    {
        return m_stopRequest.load();
    }
}