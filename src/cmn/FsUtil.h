
#ifndef FsUtil_H
#define FsUtil_H

#include "StorageApi.h"
#include "CoreData.h"

#include "StdHeader.h"

#include <guiddef.h>

namespace FsUtil { // FileSystem Util
    struct sDiskExtInfo {
        U32 drvidx;
        U64 offset; // offset in byte unit
        U64 length; // length in byte unit
    };

    struct sVolDiskExtInfo {
        char letter;          // D
        std::wstring name;    // My Storage"
        std::wstring fsname;  // NTFS
        U64 usedsize;         // size in byte unit
        U64 freesize;         // size in byte unit
        U64 totalsize;        // size in byte unit
        std::vector<sDiskExtInfo> di;
    };

    typedef std::vector<sVolDiskExtInfo> tVolArray;

    StorageApi::eRetCode ScanVolumeInfo(tVolArray& va);
    void GetLetterSet(const tVolArray& va,
        std::set<char>& used, std::set<char>& unused);

    // Called from StorageApi
    StorageApi::eRetCode UpdateVolumeInfo(StorageApi::tDriveArray& da);
    StorageApi::eRetCode TestUpdateVolumeInfo();
}

#endif
