
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

    struct sDcVolInfo {
        bool valid;
        char letter; // original volume letter
        std::string shalink; // shadowcopy mount point
        std::string mntlink; // target volume mount point

        std::string shaid;  // shadow_id string
        std::string shavol; // shadow_volume string
    };

    struct sDcPartInfo {
        U32 pidx;   // 1, 2, 3
        U32 ptype;  // PARTITION_BASIC_DATA_GUID ...
        U64 start;  // in bytes
        U64 psize;  // in bytes
        U64 nsize;  // new size in bytes (expanded)

        sDcVolInfo vi; // volume info on this partition
    };

    struct sDcDriveInfo {
        U32 drvidx;
        std::vector<sDcPartInfo> parr;
    };

    typedef std::vector<sDcPartInfo> tDcPiArray;
    typedef const std::vector<sDcPartInfo> tDcPiConstArray;

    StorageApi::eRetCode GetDriveInfo(U32 srcidx, sDcDriveInfo& si);
    StorageApi::eRetCode FilterPartition(StorageApi::tConstAddrArray &parr, sDcDriveInfo& si);
    StorageApi::eRetCode GenDestRange(const sDcDriveInfo& si, U32 dstidx, sDcDriveInfo& di);
    StorageApi::eRetCode RemovePartTable(U32 dstidx);

    StorageApi::eRetCode GenPrepareScript(const sDcDriveInfo& di, std::string& script);
    StorageApi::eRetCode GenCreatePartScript(const sDcDriveInfo& di, std::string& script);
    StorageApi::eRetCode GenCreateShadowScript(const sDcDriveInfo& di, std::string& script);

    StorageApi::eRetCode ExecShCmdList(std::string& script);
    StorageApi::eRetCode ExecDpCmdList(std::string& script);
    StorageApi::eRetCode ExecPsCmdList(std::string& script);

    StorageApi::eRetCode ExecShadowCopyScript(sDcDriveInfo& di);

    StorageApi::eRetCode VerifyPartition(U32 dstidx, const sDcDriveInfo& di);
    StorageApi::eRetCode ClonePartitions(
        const sDcDriveInfo& si, const sDcDriveInfo& di,
        volatile StorageApi::sProgress* p = NULL);

    // Called from StorageApi
    StorageApi::eRetCode HandleCloneDrive_RawCopy(
        StorageApi::CSTR& dstdrv, StorageApi::CSTR& srcdrv,
        StorageApi::tConstAddrArray& parr, volatile StorageApi::sProgress* p = NULL);

    // Called from StorageApi
    StorageApi::eRetCode HandleCloneDrive_ShadowCopy(
        StorageApi::CSTR& dstdrv, StorageApi::CSTR& srcdrv,
        StorageApi::tConstAddrArray& parr, volatile StorageApi::sProgress* p = NULL);

    StorageApi::eRetCode TestCloneDrive(U32 dstidx, U32 srcidx, U64 extsize);
}

#endif
