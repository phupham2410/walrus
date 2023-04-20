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
    const U8* _lbuf = buf, *_hbuf = buf + 8; \
    CASTBUF64(hval, _hbuf); CASTBUF64(lval, _lbuf); } while(0)

typedef NVME_HEALTH_INFO_LOG tHealthLog;
typedef NVME_IDENTIFY_CONTROLLER_DATA tCtrlData;
typedef NVME_IDENTIFY_NAMESPACE_DATA tNsData;
typedef std::vector<tNsData> tNsDataArr;

static eRetCode ConvertIdentify(const tCtrlData& ctrl, const tNsDataArr& nsarr, sIdentify& id) {
    uint64_t dl, dh; // temporary values

    if (1) {
        #define MAP_ITEM(sname, dname) do { \
            TMPBUFFER(buf); \
            memcpy(buf, ctrl.sname, sizeof(ctrl.sname)); \
            id.dname = buf; } while(0)
        MAP_ITEM(MN, model);
        MAP_ITEM(SN, serial);
        MAP_ITEM(FR, version);
        #undef MAP_ITEM
        id.confid = "N/A";
    }

    if (1) {
        F64 cap = 0;
        for (U32 i = 0, maxi = nsarr.size(); i < maxi; i++) {
            cap += (nsarr[i].NSZE * 100 >> 21) / 100.0; // Capacity in GB
        }
        id.cap = cap;
    }

    return RET_OK;
}

static eRetCode ConvertFeatures(const tCtrlData& ctrl, sFeature& ftr) {
    ftr.smart = 1; // Always 1 for NVMe device
    ftr.test = ctrl.OACS.DeviceSelfTest;
    ftr.erase = (ctrl.SANICAP.CryptoErase << 2) |
                (ctrl.SANICAP.BlockErase << 1) |
                (ctrl.SANICAP.Overwrite << 0);
    ftr.security = ctrl.OACS.SecurityCommands;
    ftr.dlcode = ctrl.OACS.FirmwareCommands;
    return RET_OK;
}

static eRetCode ConvertSmartInfo(const NVME_HEALTH_INFO_LOG& hlog, sSmartInfo& si) {
    uint64_t dl, dh; // temporary values

    if (1) {
        uint32_t val; CASTBUF16(val, hlog.Temperature);
        ApiUtil::SetSmartRaw(si.amap, SMA_COMPOSITE_TEMP, val, 0);
    }

    ApiUtil::SetSmartRaw(si.amap, SMA_SPARE_PERCENTAGE, hlog.AvailableSpare, 0);
    ApiUtil::SetSmartRaw(si.amap, SMA_SPARE_THRESHOLD, hlog.AvailableSpareThreshold, 0);
    ApiUtil::SetSmartRaw(si.amap, SMA_PERCENTAGE_USED, hlog.PercentageUsed, 0);

    #define MAP_ITEM(name, attrid) \
        CASTBUF128(dh, dl, hlog.name); if (dh) return RET_OUT_OF_SPACE; \
        ApiUtil::SetSmartRaw(si.amap, attrid, dl & MASK32B, (dl >> 32) & MASK32B)
    MAP_ITEM(DataUnitRead, SMA_DATA_UNIT_READ);
    MAP_ITEM(DataUnitWritten, SMA_DATA_UNIT_WRITTEN);
    MAP_ITEM(PowerCycle, SMA_POWER_CYCLE);
    MAP_ITEM(PowerOnHours, SMA_NVME_POWER_ON_HOURS);
    MAP_ITEM(UnsafeShutdowns, SMA_UNSAFE_SHUTDOWNS);

    MAP_ITEM(MediaErrors, SMA_MEDIA_ERRORS);
    MAP_ITEM(ErrorInfoLogEntryCount, SMA_ERROR_LOG_COUNT);
    MAP_ITEM(ControllerBusyTime, SMA_CTRL_BUSY_TIME);
    MAP_ITEM(HostReadCommands, SMA_HOST_CMD_READ);
    MAP_ITEM(HostWrittenCommands, SMA_HOST_CMD_WRITTEN);
    #undef MAP_ITEM

    #define MAP_ITEM(name, attrid) \
        ApiUtil::SetSmartRaw(si.amap, attrid, hlog.name, 0)
    MAP_ITEM(TemperatureSensor1, SMA_TEMP_SENSOR_1);
    MAP_ITEM(TemperatureSensor2, SMA_TEMP_SENSOR_2);
    MAP_ITEM(TemperatureSensor3, SMA_TEMP_SENSOR_3);
    MAP_ITEM(TemperatureSensor4, SMA_TEMP_SENSOR_4);
    MAP_ITEM(TemperatureSensor5, SMA_TEMP_SENSOR_5);
    MAP_ITEM(TemperatureSensor6, SMA_TEMP_SENSOR_6);
    MAP_ITEM(TemperatureSensor7, SMA_TEMP_SENSOR_7);
    MAP_ITEM(TemperatureSensor8, SMA_TEMP_SENSOR_8);

    MAP_ITEM(WarningCompositeTemperatureTime, SMA_TEMP_WARNING);
    MAP_ITEM(CriticalCompositeTemperatureTime, SMA_TEMP_CRITICAL);
    #undef MAP_ITEM

    return RET_OK;
}

static eRetCode ScanDrive_NvmeBus(sPHYDRVINFO& phy, U32 index, bool rsm, sDriveInfo& di, volatile sProgress *p) {
    bool status; (void) rsm; (void) index;
    tCtrlData ctrl; tHealthLog hlog; tNsDataArr nsarr;

    status = NvmeUtil::IdentifyController((HANDLE) phy.DriveHandle, &ctrl);
    if (!status) return RET_FAIL;

    for (U32 i = 0, maxi = ctrl.NN; i < maxi; i++) {
        tNsData nmsp; memset(&nmsp, 0x00, sizeof(nmsp));
        status = NvmeUtil::IdentifyNamespace((HANDLE) phy.DriveHandle, i + 1, &nmsp);
        if (status) nsarr.push_back(nmsp); else return RET_FAIL;
    }

    status = NvmeUtil::GetSMARTHealthInfoLog((HANDLE) phy.DriveHandle, &hlog);
    if (!status) return RET_FAIL;

    di.name = phy.DriveName;
    ConvertIdentify(ctrl, nsarr, di.id);
    ConvertFeatures(ctrl, di.id.features);
    ConvertSmartInfo(hlog, di.si);

    if (1) { // Others info
        #define MAP_ITEM(name, attrid) do { \
            sSmartAttr attr; if (ApiUtil::GetSmartAttr(di.si.amap, attrid, attr)) { \
                di.name = ((U64)attr.hiraw << 32) | attr.loraw; }} while(0)
        MAP_ITEM(temp, SMA_COMPOSITE_TEMP);
        MAP_ITEM(tread, SMA_DATA_UNIT_READ);
        MAP_ITEM(twrtn, SMA_DATA_UNIT_WRITTEN);
        #undef MAP_ITEM
    }

    UPDATE_PROGRESS(p, 6);
    return RET_OK;
}

static eRetCode Read_NvmeBus(HDL handle, U64 lba, U32 count, U8 *buffer, volatile sProgress *p) {
    return RET_FAIL;
}

#endif // StorageApiNvme_H
