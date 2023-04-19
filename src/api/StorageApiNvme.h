#ifndef StorageApiNvme_H
#define StorageApiNvme_H

#include "CoreUtil.h"
#include "ApiUtil.h"
#include "DeviceMgr.h"
#include "StorageApiCmn.h"
#include "NvmeUtil.h"

#define TO16(p, n) (((U16) p[n]) << (n * 8))
#define TO32(p, n) (((U32) p[n]) << (n * 8))
#define TO64(p, n) (((U64) p[n]) << (n * 8))

#define CASTBUF16(val, p) val = TO16(p,1) | TO16(p,0)
#define CASTBUF32(val, p) val = TO32(p,3) | TO32(p,2) | TO32(p,1) | TO32(p,0)
#define CASTBUF64(val, p) val = TO64(p,7) | TO64(p,6) | TO64(p,5) | TO64(p,4) | \
                                  TO64(p,3) | TO64(p,2) | TO64(p,1) | TO64(p,0)
#define CASTBUF128(hval, lval, buf) do { \
    U8* _lbuf = buf, *_hbuf = buf+ 8; \
    CASTBUF64(hval, _hbuf); CASTBUF64(lval, _lbuf); } while(0)

static eRetCode ScanDrive_NvmeBus(sPHYDRVINFO& phy, U32 index, bool rsm, sDriveInfo& di, volatile sProgress *p) {
    bool status;
    NVME_IDENTIFY_CONTROLLER_DATA ctrl;
    NVME_HEALTH_INFO_LOG hlog;

    status = NvmeUtil::IdentifyController((HANDLE) phy.DriveHandle, &ctrl);
    if (!status) return RET_FAIL;

    status = NvmeUtil::GetSMARTHealthInfoLog((HANDLE) phy.DriveHandle, &hlog);
    if (!status) return RET_FAIL;

    StorageApi::sIdentify& id = di.id;
    #define MAP_ITEM(sname, dname) do { \
        TMPBUFFER(buf); \
        memcpy(buf, ctrl.sname, sizeof(ctrl.sname)); \
        id.dname = buf; } while(0)
    MAP_ITEM(MN, model);
    MAP_ITEM(SN, serial);
    MAP_ITEM(FR, version);
    #undef MAP_ITEM
    id.confid = "N/A-9999.99";
    id.cap = 9999.99;

    StorageApi::sFeature& ftr = di.id.features;
    ftr.smart = ctrl.LPA.SmartPagePerNamespace;

    StorageApi::sSmartInfo& si = di.si;
    if (1) {
        uint32_t val = ((uint32_t) hlog.Temperature[1] << 8) | ((uint32_t) hlog.Temperature[0]);
        ApiUtil::SetSmartRaw(si.amap, SMA_TEMPERATURE_CELSIUS, val, 0);
    }

    ApiUtil::SetSmartRaw(si.amap, SMA_REMAINING_SPARE_COUNT, hlog.AvailableSpare, 0, hlog.AvailableSpareThreshold);
    ApiUtil::SetSmartRaw(si.amap, SMA_REMAINING_LIFE_LEFT,   hlog.PercentageUsed, 0);

    uint64_t dl, dh; // Data Unit Read (sector)
    #define MAP_ITEM(name, attrid) \
        CASTBUF128(dh, dl, hlog.name); if (dh) return RET_OUT_OF_SPACE; \
        ApiUtil::SetSmartRaw(si.amap, attrid, dl & MASK32B, (dl >> 32) & MASK32B)
    MAP_ITEM(DataUnitRead, SMA_TOTAL_LBA_READ);
    MAP_ITEM(DataUnitWritten, SMA_TOTAL_LBA_WRITTEN);
    MAP_ITEM(PowerCycle, SMA_POWER_CYCLE_COUNT);
    MAP_ITEM(PowerOnHours, SMA_POWER_ON_HOURS);
    MAP_ITEM(UnsafeShutdowns, SMA_SUDDEN_POWER_LOST_COUNT);

    MAP_ITEM(MediaErrors, SMA_RESERVED21);
    MAP_ITEM(ErrorInfoLogEntryCount, SMA_RESERVED22);
    MAP_ITEM(ControllerBusyTime, SMA_RESERVED23);
    MAP_ITEM(HostReadCommands, SMA_RESERVED24);
    MAP_ITEM(HostWrittenCommands, SMA_RESERVED25);
    #undef MAP_ITEM

    #define MAP_ITEM(name, attrid) \
        ApiUtil::SetSmartRaw(si.amap, attrid, hlog.name, 0)
    MAP_ITEM(TemperatureSensor1, SMA_RESERVED01);
    MAP_ITEM(TemperatureSensor2, SMA_RESERVED02);
    MAP_ITEM(TemperatureSensor3, SMA_RESERVED03);
    MAP_ITEM(TemperatureSensor4, SMA_RESERVED04);
    MAP_ITEM(TemperatureSensor5, SMA_RESERVED05);
    MAP_ITEM(TemperatureSensor6, SMA_RESERVED06);
    MAP_ITEM(TemperatureSensor7, SMA_RESERVED07);
    MAP_ITEM(TemperatureSensor8, SMA_RESERVED08);

    MAP_ITEM(WarningCompositeTemperatureTime, SMA_RESERVED11);
    MAP_ITEM(CriticalCompositeTemperatureTime, SMA_RESERVED12);
    #undef MAP_ITEM

    UPDATE_PROGRESS(p, 6);
    return RET_OK;
}

static eRetCode Read_NvmeBus(HDL handle, U64 lba, U32 count, U8 *buffer, volatile sProgress *p) {
    return RET_FAIL;
}

#endif // StorageApiNvme_H
