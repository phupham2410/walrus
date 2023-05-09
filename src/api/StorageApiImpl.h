
#ifdef STORAGE_API_IMPL

#include "windows.h"
#include "ntddscsi.h"
#include "winioctl.h"
#include "ntddstor.h"

#include "ApiUtil.h"
#include "DeviceMgr.h"

#include "StorageApiCmn.h"

// --------------------------------------------------------------------------------

// Real implementation of StorageApi
using namespace StorageApi;

// Select function to run based on BusType
#define SELFUNC_BY_BUSTYPE(_bt, _rv, _prefix, ...) do { switch(_bt) { \
        case 0x07: _rv = _prefix##_UsbBus(__VA_ARGS__); break; \
        case 0x0B: _rv = _prefix##_SataBus(__VA_ARGS__); break; \
        case 0x11: _rv = _prefix##_NvmeBus(__VA_ARGS__); break; \
    }} while(0)

// Append log to volatile info
#define PROGINFO(p) *const_cast<std::string*>(&p->info)
static void AppendLog(volatile sProgress* p, CSTR& log) {
    if (p) PROGINFO(p) = PROGINFO(p) + "\n" + log;
}

// Dummy waiting function
static eRetCode ProcessTask(volatile sProgress* p, U32 load, eRetCode ret) {
    INIT_PROGRESS(p, load); // Prepare progress
    for (U32 i = 0; i < load; i++) {
        RETURN_IF_STOP(p, RET_ABORTED);
        Sleep(1000);
        UPDATE_PROGRESS(p, 1);
    }
    FINALIZE_PROGRESS(p, ret);
    return p->rval;
}

static void UpdateAdapterInfo(const sAdapterInfo& src, sDriveInfo& dst) {
    dst.bustype = src.BusType;
    dst.maxtfsec = src.MaxTransferSector;
}

static void CloseDevice(sPHYDRVINFO& phy) {
    return DeviceMgr::CloseDevice(phy);
}

// Bus-Specific utilities
#include "StorageApiScsi.h"
#include "StorageApiUsb.h"
#include "StorageApiSata.h"
#include "StorageApiNvme.h"

// --------------------------------------------------------------------------------




// --------------------------------------------------------------------------------

// This function is similar to DeviceMgr::ScanDriveCommon
eRetCode StorageApi::ScanDrive(tDriveArray &darr, bool rid, bool rsm, volatile sProgress *p) {
    (void) rid; darr.clear();

    eRetCode rc = RET_OK;
    U32 vldcnt = 0;  // number of openable drive
    U32 phycnt = 16; // max 16 physical drives in Windows
    U32 tskcnt = 6;  // 6 tasks per drive: Open, Identify, SmartData, Threshold, Parse, Partition

    INIT_PROGRESS(p, phycnt * tskcnt);

    for (U32 i = 0; i < phycnt; i++)
    {
        // --------------------------------------------------
        // Open physical drive

        sPHYDRVINFO phy;
        if (!DeviceMgr::OpenDevice(i, phy)) SKIP_AND_CONTINUE(p,6)
        UPDATE_AND_RETURN_IF_STOP(p, 1, CloseDevice, phy, RET_ABORTED);

        vldcnt++; // openable physical drive

        // --------------------------------------------------
        // Read Addapter Info
        sAdapterInfo adapter;
        if (GetDeviceInfo(phy.DriveHandle, adapter) != RET_OK) continue;

        std::stringstream lstr;
        lstr << "BusType: " << adapter.BusType; AppendLog(p, lstr.str());

        eRetCode ret; sDriveInfo di;
        SELFUNC_BY_BUSTYPE(adapter.BusType, ret, ScanDrive, phy, i, rsm, di, p);

        if (ret != RET_OK) SKIP_AND_CONTINUE(p,1)

        // --------------------------------------------------
        // accept drive

        UpdateAdapterInfo(adapter, di);
        ScanPartition(phy.DriveHandle, di.pi);
        UPDATE_PROGRESS(p, 1);

        darr.push_back(di); DeviceMgr::CloseDevice(phy);
    }

    if (!darr.size())
        rc = (vldcnt != 0) ? RET_SKIP_DRIVE : RET_NO_PERMISSION;

    FINALIZE_PROGRESS(p, rc); return rc;
}

eRetCode StorageApi::ScanPartition(CSTR &drvname, sPartInfo &pinf) {
    HDL hdl; pinf.parr.clear();
    if (!DeviceMgr::OpenDevice(drvname, hdl)) return RET_NO_PERMISSION;
    return ScanPartition(hdl, pinf);
}

eRetCode StorageApi::ScanPartition(HDL handle, sPartInfo &pinf) {
    pinf.parr.clear();
    const uint32_t bufsize = 8192; uint8_t buffer[bufsize]; DWORD retcnt;
    if (!DeviceIoControl(*(HANDLE*) &handle, IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
             NULL, 0, buffer, bufsize, &retcnt, NULL)) return RET_NOT_SUPPORT;

    if (!retcnt) return RET_EMPTY;

    DRIVE_LAYOUT_INFORMATION_EX* layout = (DRIVE_LAYOUT_INFORMATION_EX*)buffer;
    for (int i = 0, maxi = layout->PartitionCount; i < maxi; i++) {
        PARTITION_INFORMATION_EX& item = layout->PartitionEntry[i];

        sPartition part;
        part.index = item.PartitionNumber;
        part.addr.first = item.StartingOffset.QuadPart;   // byte unit
        part.addr.second = item.PartitionLength.QuadPart; // byte unit
        part.cap = part.addr.second >> 9;                 // cap in sector
        part.name = (item.PartitionStyle == 1) ? item.Gpt.Name : L"Unknown Name";
        pinf.parr.push_back(part);
    }

    return pinf.parr.size() ? RET_OK : RET_NOT_FOUND;
}

eRetCode StorageApi::UpdateFirmware(CSTR& drvname, const U8* bufptr, U32 bufsize, volatile sProgress *p) {
    (void) drvname; (void) bufptr; (void) bufsize; (void) p;
    return ProcessTask(p, 3, RET_NOT_IMPLEMENT);
}

eRetCode StorageApi::OptimizeDrive(CSTR &drvname, volatile sProgress *p) {
    (void) drvname; (void) p;
    return ProcessTask(p, 3, RET_NOT_IMPLEMENT);
}

eRetCode StorageApi::SetOverProvision(CSTR &drvname, U32 factor, volatile sProgress *p) {
    (void) drvname; (void) factor; (void) p;
    return ProcessTask(p, 3, RET_NOT_IMPLEMENT);
}

eRetCode StorageApi::SecureErase(CSTR &drvname, volatile sProgress *p) {
    (void) drvname; (void) p;
    return ProcessTask(p, 3, RET_NOT_IMPLEMENT);
}

eRetCode StorageApi::SelfTest(CSTR &drvname, U32 param, volatile sProgress *p) {
    (void) drvname; (void) param; (void) p;
    return ProcessTask(p, 3, RET_NOT_IMPLEMENT);
}

eRetCode StorageApi::CloneDrive(CSTR &dstdrv, CSTR &srcdrv, tConstAddrArray &parr, volatile sProgress *p) {
    (void) dstdrv; (void) srcdrv; (void) parr; (void) p;
    return ProcessTask(p, 3, RET_NOT_IMPLEMENT);
}

// --------------------------------------------------------------------------------

eRetCode StorageApi::Open(CSTR &drvname, HDL &handle) {
    return DeviceMgr::OpenDevice(drvname, handle) ? RET_OK : RET_NO_PERMISSION;
}

void StorageApi::Delay(U32 mlsec) {
    Sleep(mlsec);
}

void StorageApi::Close(HDL handle) {
    DeviceMgr::CloseDevice(handle);
}

eRetCode StorageApi::Write(HDL handle, U64 lba, U32 count, const U8 *buffer, volatile sProgress *p) {
    (void) handle; (void) lba; (void) count; (void) buffer; (void) p;
    return ProcessTask(p, 2, RET_NOT_IMPLEMENT);
}

eRetCode StorageApi::Read(HDL handle, U64 lba, U32 count, U8 *buffer, U32 bustype, volatile sProgress *p) {
    eRetCode ret;
    SELFUNC_BY_BUSTYPE(bustype, ret, Read, handle, lba, count, buffer, p);
    return ret;
}

eRetCode StorageApi::GetDeviceInfo(HDL hdl, sAdapterInfo &info) {
    HANDLE handle = (HANDLE) hdl;
    STORAGE_PROPERTY_QUERY prop;
    prop.PropertyId = StorageAdapterProperty;
    prop.QueryType  = PropertyStandardQuery;

    STORAGE_DESCRIPTOR_HEADER hdr;
    DWORD retcnt, ret;
    ret = DeviceIoControl(handle, IOCTL_STORAGE_QUERY_PROPERTY,
              &prop, sizeof(prop), &hdr, sizeof(hdr), &retcnt, NULL);

    if (!ret) return RET_NO_PERMISSION;
    if (!hdr.Size) return RET_NOT_SUPPORT;

    STORAGE_ADAPTER_DESCRIPTOR* ad =
            (STORAGE_ADAPTER_DESCRIPTOR*)malloc(hdr.Size);

    ret = DeviceIoControl( handle, IOCTL_STORAGE_QUERY_PROPERTY,
               &prop, sizeof(prop), ad, hdr.Size, &retcnt, NULL);
    if (!ret) { free(ad); return RET_NOT_SUPPORT; }

    info.BusType = ad->BusType; // Windows' STORAGE_BUS_TYPE
    info.MaxTransferSector = ad->MaximumTransferLength / 512;
    free( ad );
    return RET_OK;
}

#endif // STORAGE_API_IMPL
