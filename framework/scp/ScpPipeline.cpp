#include "ScpPipeline.hpp"

#include <dcmtk/dcmdata/dcdeftag.h>

#include <functional>



// Prepare
Prepare::Prepare() : PipelineStage("Prepare")
{

}

 void Prepare::run()
{
    using namespace std::chrono_literals;

    framework::logg(name() + ": running");

    while (!shouldStop())
    {
        if (auto data = nextData<StoredData>(200ms); data)
        {
            bool good = true;
            
            // remove pixel data
            if (data->dataset->tagExists(DCM_PixelData, false) && data->dataset->findAndDeleteElement(DCM_PixelData).bad())
            {
                good = false;
            }

            if (good)
            {
                dataComplete(std::make_shared<StoredData>());
            }            
        }
    }
}


 
 // Elastic
 IndexDicom::IndexDicom() : PipelineStage("Index Dicom")
 {

 }

 void IndexDicom::run()
 {
     using namespace std::chrono_literals;

     framework::logg(name() + ": running");

     while (!shouldStop())
     {
         if (auto data = nextData<StoredData>(200ms); data)
         {
             auto& dataset = data->dataset;

             if (dataset)
             {
                 bool good = true;

                    

                 if (good)
                 {
                     dataComplete(std::make_shared<StoredData>());
                 }
             }
         }
     }
 }
