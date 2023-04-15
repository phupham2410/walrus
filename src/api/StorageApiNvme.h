#ifndef StorageApiNvme_H
#define StorageApiNvme_H

#include "ApiUtil.h"
#include "DeviceMgr.h"
#include "StorageApiCmn.h"

static eRetCode ScanDrive_NvmeBus(sPHYDRVINFO& phy, U32 index, bool rsm, sDriveInfo& di, volatile sProgress *p) {
    std::stringstream sstr;
    sstr << "Scanning device " << index << " on NvmeBus";
    AppendLog(p, sstr.str());

    UPDATE_PROGRESS(p, 6);
    return RET_NOT_SUPPORT;
}

static eRetCode Read_NvmeBus(HDL handle, U64 lba, U32 count, U8 *buffer, volatile sProgress *p) {
    return RET_FAIL;
}

#endif // StorageApiNvme_H
