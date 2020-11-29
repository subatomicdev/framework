#include "ScpPipeline.hpp"

#include <Logger.hpp>

#include <dcmtk/dcmdata/dcdeftag.h>
#include <elasticlient/bulk.h>
#include <elasticlient/client.h>
#include <json/json.h>

#include <functional>
#include <vector>

using std::vector;
using std::string;


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
                dataComplete(data);
            }            
        }
    }
}


 
 // Elastic
 struct DicomTag
 {
     DicomTag(short g, short el) : group(g), element(el)
     {

     }

     DicomTag() : group(0), element(0)
     {
              
     }

     short group;
     short element;     
 };

 struct IndexDicomTag
 {
     IndexDicomTag() = default;

     DicomTag tag;
     string vrName;

     inline void toJson(Json::Value& v)
     {
         v["tagGroup"] = tag.group;
         v["tagElement"] = tag.element;
         v["vrName"] = vrName;
     }
 };

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
                 const size_t AverageTagsPerDicomInstance = 180;

                 elasticlient::SameIndexBulkData bulk("dicomtag", AverageTagsPerDicomInstance);

                 Json::StreamWriterBuilder streamBuilder;
                 streamBuilder["indentation"] = "";

                 const auto nParentTags = dataset->getRootItem()->getNumberOfValues();
                 size_t index = 0;

                 for (DcmElement* dcmEl = dataset->getRootItem()->getElement(0); dcmEl && index < nParentTags; )
                 {
                     char* value;
                     if (dcmEl->getString(value).good() && value)
                     {
                         IndexDicomTag indexTag;
                         indexTag.tag.group = dcmEl->getTag().getGroup();
                         indexTag.tag.element = dcmEl->getTag().getElement();
                         indexTag.vrName = dcmEl->getTag().getVRName();

                         Json::Value tagJson;
                         indexTag.toJson(tagJson);

                         string jsonString = Json::writeString(streamBuilder, tagJson);
                         bulk.indexDocument("docType", "", jsonString);
                     }

                     dcmEl = dataset->getRootItem()->getElement(++index);
                 }

                 std::stringstream ss;
                 ss << "Indexed " << bulk.size() << " tags";
                 framework::logg(ss.str());


                 // TODO send to elastic
                 auto client = std::make_shared<elasticlient::Client> (vector<string>({"http://localhost:9200/"}));
                 
                 elasticlient::Bulk bulkIndex(client);
                 const auto nError = bulkIndex.perform(bulk);

                 dataComplete(std::make_shared<StoredData>());
             }
         }
     }
 }
