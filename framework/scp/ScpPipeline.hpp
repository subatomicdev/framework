#pragma once


#include <Pipeline.hpp>

#include "dcmtk/dcmdata/dcfilefo.h"


struct StoredData : framework::StageData
{
    std::shared_ptr<DcmDataset> dataset;
};


class Prepare : public framework::PipelineStage
{
public:
    Prepare() ;
    
    virtual void run() override;
};

class IndexDicom : public framework::PipelineStage
{
public:
    IndexDicom();

    virtual void run() override;
};