
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

    bool SetSmartRaw(StorageApi::tAttrMap& smap, U16 id, U32 lo, U32 hi);
    bool GetSmartValue(const StorageApi::tAttrMap& smap, U16 id, U8& val);
    bool GetSmartRaw(const StorageApi::tAttrMap& smap, U16 id, U32& lo, U32& hi);
    bool GetSmartAttr(const StorageApi::tAttrMap& smap, U16 id, StorageApi::sSmartAttr& attr);
    StorageApi::sSmartAttr& GetSmartAttrRef(StorageApi::tAttrMap&, U16 id);

    StorageApi::STR ToAttrValueString(U16 id, U32 loraw, U32 hiraw);
    StorageApi::STR ToNvmeAttrString̣̣̣̣̣̣(const StorageApi::sSmartAttr& sa, const char* sep = " ");
    StorageApi::STR ToAtaAttrString̣̣̣̣̣̣(const StorageApi::sSmartAttr& sa, const char* sep = " ");

    bool IsTrimSupported(StorageApi::HDL hdl);
}

#endif
