
#include "CloneUtil.h"
#include "CoreUtil.h"

#include <windows.h>
#include "winioctl.h"

using namespace std;
using namespace StorageApi;
using namespace DiskCloneUtil;

#include <diskguid.h>
#include <ntdddisk.h>

static inline void UtilSetGuid(GUID& g, U32 d1, U16 d2, U16 d3, U64 d4) {
    g.Data1 = d1; g.Data2 = d2; g.Data3 = d3;
    for (int i = 7; i >= 0; i--) {
        g.Data4[7-i] = (d4 >> (8*i)) & 0xFF;
    }
}

static inline bool GetDriveIndex(const std::string& name, U32& index) {
    const char* p = name.c_str();
    while(*p && !INRANGE('0', '9' + 1, *p)) p++;
    if (!*p) return false;
    sscanf(p, "%d", &index); return true;
}

static eDcPartType ConvertGptPartitionType(GUID& src) {
    #define MAP_ITEM(d1, d2, d3, d4, val) \
    { GUID g; UtilSetGuid(g, d1, d2, d3, d4); if (src == g) return val; }
    MAP_ITEM(0xebd0a0a2, 0xb9e5, 0x4433, 0x87c068b6b72699c7, DPT_PARTITION_BASIC_DATA_GUID)
    MAP_ITEM(0x00000000, 0x0000, 0x0000, 0x0000000000000000, DPT_PARTITION_ENTRY_UNUSED_GUID)
    MAP_ITEM(0xc12a7328, 0xf81f, 0x11d2, 0xba4b00a0c93ec93b, DPT_PARTITION_SYSTEM_GUID)
    MAP_ITEM(0xe3c9e316, 0x0b5c, 0x4db8, 0x817df92df00215ae, DPT_PARTITION_MSFT_RESERVED_GUID)
    MAP_ITEM(0x5808c8aa, 0x7e8f, 0x42e0, 0x85d2e1e90434cfb3, DPT_PARTITION_LDM_METADATA_GUID)
    MAP_ITEM(0xaf9b60a0, 0x1431, 0x4f62, 0xbc683311714a69ad, DPT_PARTITION_LDM_DATA_GUID)
    MAP_ITEM(0xde94bba4, 0x06d1, 0x4d40, 0xa16abfd50179d6ac, DPT_PARTITION_MSFT_RECOVERY_GUID)
    #undef MAP_ITEM
    return DPT_INVALID;
}

static eRetCode UtilGetDriveLayout(HDL hdl, U8* buffer, U32 bufsize) {
    if (!buffer) return StorageApi::RET_INVALID_ARG;
    memset(buffer, 0x00, bufsize);

    BOOL ret; DWORD rsize;
    ret = DeviceIoControl((HANDLE) hdl, IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                          NULL, 0, buffer, bufsize, &rsize, NULL);

    return ret ? RET_OK : RET_FAIL;
}

static void ConvertPartInfo(PARTITION_INFORMATION_EX& p, sDcPartInfo& dpi) {
    dpi.pidx = p.PartitionNumber;
    dpi.start = p.StartingOffset.QuadPart;
    dpi.psize = p.PartitionLength.QuadPart;
    dpi.nsize = dpi.psize;

    switch(p.PartitionStyle) {
    case PARTITION_STYLE_RAW:
    case PARTITION_STYLE_MBR: dpi.ptype = DPT_PARTITION_BASIC_DATA_GUID; break;
    case PARTITION_STYLE_GPT: dpi.ptype = ConvertGptPartitionType(p.Gpt.PartitionType); break;
    default: break;
    }
}

eRetCode GetDriveInfo(HDL hdl, U32 srcidx, sDcDriveInfo& si) {
    si.drvidx = 0; si.parr.clear();

    eRetCode ret;

    // Read DriveLayout and Partitions
    // Fill in si

    const U32 bufsize = 4096; U8 buffer[bufsize];
    memset(buffer, 0x00, bufsize);

    ret = UtilGetDriveLayout(hdl, buffer, bufsize);
    if (ret != RET_OK) return ret;

    DRIVE_LAYOUT_INFORMATION_EX* dli = (DRIVE_LAYOUT_INFORMATION_EX*) buffer;
    U32 count = dli->PartitionCount;
    PARTITION_INFORMATION_EX* pi = (PARTITION_INFORMATION_EX*) &dli->PartitionEntry[0];

    si.drvidx = srcidx;

    for (U32 i = 0; i < count; i++, pi++) {
        PARTITION_INFORMATION_EX& p = *pi;
        if (!p.PartitionNumber) continue;
        sDcPartInfo dpi;
        ConvertPartInfo(p, dpi);
        si.parr.push_back(dpi);
    }

    return RET_OK;
}
