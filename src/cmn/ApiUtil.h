
#ifndef ApiUtil_H
#define ApiUtil_H

#include "StorageApi.h"
#include "CoreData.h"

// Converter from original data structures to StorageApi's

namespace ApiUtil {

    void ConvertSmartAttr(const sSMARTATTR& src, StorageApi::sSmartAttr& dst);
    void ConvertFeatures(const sFEATURE& src, StorageApi::sFeature& dst);
    void ConvertIdentify(const sIDENTIFY& src, StorageApi::sIdentify& dst);
    void UpdateDriveInfo(const sDRVINFO& src, StorageApi::sDriveInfo& dst);

    enum eSmartKey {
        SMA_TEMPERATURE = 194,        // C2
        SMA_TOTAL_HOST_READ = 242,    // F2
        SMA_TOTAL_HOST_WRITTEN = 241, // F1
    };

    bool SetSmartRaw(StorageApi::tAttrMap& smap, U8 id, U32 lo, U32 hi);
    bool GetSmartValue(const StorageApi::tAttrMap& smap, U8 id, U8& val);
    bool GetSmartRaw(const StorageApi::tAttrMap& smap, U8 id, U32& lo, U32& hi);
    bool GetSmartAttr(const StorageApi::tAttrMap& smap, U8 id, StorageApi::sSmartAttr& attr);
}

#endif
