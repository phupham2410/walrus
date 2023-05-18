
#ifndef CloneUtil_H
#define CloneUtil_H

#include "StorageApi.h"
#include "CoreData.h"

#include "StdHeader.h"

#include <guiddef.h>

namespace DiskCloneUtil { // DC

    enum eDcPartType {
        DPT_INVALID = 0,
        DPT_PARTITION_BASIC_DATA_GUID,
        DPT_PARTITION_ENTRY_UNUSED_GUID,
        DPT_PARTITION_SYSTEM_GUID,
        DPT_PARTITION_MSFT_RESERVED_GUID,
        DPT_PARTITION_LDM_METADATA_GUID,
        DPT_PARTITION_LDM_DATA_GUID,
        DPT_PARTITION_MSFT_RECOVERY_GUID,
    };

    struct sDcPartInfo {
        U32 pidx;   // 1, 2, 3
        U32 ptype;  // PARTITION_BASIC_DATA_GUID ...
        U64 start;  // in bytes
        U64 psize;  // in bytes
        U64 nsize;  // new size in bytes (expanded)
    };

    struct sDcDriveInfo {
        U32 drvidx;
        std::vector<sDcPartInfo> parr;
    };

    typedef std::vector<sDcPartInfo> tDcPiArray;
    typedef const std::vector<sDcPartInfo> tDcPiConstArray;

    StorageApi::eRetCode GetDriveInfo(StorageApi::HDL hdl, U32 srcidx, sDcDriveInfo& si);
    StorageApi::eRetCode ValidateRange(StorageApi::tConstAddrArray &parr, sDcDriveInfo& si);
    StorageApi::eRetCode GenDestRange(const sDcDriveInfo& si, U32 dstidx, sDcDriveInfo& di);
    StorageApi::eRetCode GenCloneScript(const sDcDriveInfo& si, const sDcDriveInfo& di, std::string& script);
    StorageApi::eRetCode ExecScript(std::string& script);
    StorageApi::eRetCode ClonePartitions(const sDcDriveInfo& si, const sDcDriveInfo& di);

    // Called from StorageApi
    StorageApi::eRetCode HandleCloneDrive(
        StorageApi::CSTR& dstdrv, StorageApi::CSTR& srcdrv,
        StorageApi::tConstAddrArray& parr, volatile StorageApi::sProgress* p = NULL);
}

#endif
