
#include "ApiUtil.h"
#include "CoreUtil.h"
#include "SysHeader.h"

using namespace StorageApi;

void ApiUtil::ConvertSmartAttr(const sSMARTATTR& src, sSmartAttr& dst) {
    dst.reset();
    dst.type = eAttrType((src.ID >> 8) & 0xFF);
    #define MAP_ITEM(d, s) dst.d = src.s
    MAP_ITEM(worst, Worst); MAP_ITEM(threshold, Threshold);
    MAP_ITEM(id, ID); MAP_ITEM(name, Name); MAP_ITEM(value, Value);
    MAP_ITEM(loraw, LowRaw); MAP_ITEM(hiraw, HighRaw); MAP_ITEM(note, Note);
    #undef MAP_ITEM
}

void ApiUtil::ConvertFeatures(const sFEATURE& src, sFeature& dst) {
    dst.reset();
    #define MAP_ITEM(d, s) dst.d = src.s
    MAP_ITEM(ataversion,  AtaVersion);
    MAP_ITEM(dma,  IsDmaSupported);
    MAP_ITEM(lbamode,  IsLba48Supported);
    MAP_ITEM(ncq,  IsNcqSupported);
    MAP_ITEM(provision,  IsOpSupported);
    MAP_ITEM(security,  IsSecuritySupported);
    // secure-erase is mandatory in security feature set
    MAP_ITEM(erase,  IsSecuritySupported);
    MAP_ITEM(smart,  IsSmartSupported);
    MAP_ITEM(test,  IsTestSupported);
    MAP_ITEM(trim,  IsTrimSupported);
    MAP_ITEM(dlcode,  IsDlCodeSupported);
    #undef MAP_ITEM
}

void ApiUtil::ConvertIdentify(const sIDENTIFY& src, sIdentify& dst) {
    dst.reset();
    #define MAP_ITEM(d, s) dst.d = src.s
    MAP_ITEM(cap, UserCapacity);
    MAP_ITEM(model, DeviceModel);
    MAP_ITEM(serial, SerialNumber);
    MAP_ITEM(version, FirmwareVersion);
    #undef MAP_ITEM

    ApiUtil::ConvertFeatures(src.SectorInfo, dst.features);
}

bool ApiUtil::GetSmartAttr(const tAttrMap& smap, U16 id, sSmartAttr& attr) {
    tAttrConstIter iter = smap.find(id);
    if (iter == smap.end()) return false;
    attr = iter->second; return true;
}

bool ApiUtil::GetSmartRaw(const tAttrMap& smap, U16 id, U32& lo, U32& hi) {
    sSmartAttr attr;
    if (!ApiUtil::GetSmartAttr(smap, id, attr)) return false;
    lo = attr.loraw; hi = attr.hiraw; return true;
}

bool ApiUtil::GetSmartValue(const tAttrMap& smap, U16 id, U8& val) {
    sSmartAttr attr;
    if (!ApiUtil::GetSmartAttr(smap, id, attr)) return false;
    val = attr.value; return true;
}

sSmartAttr& ApiUtil::GetSmartAttrRef(tAttrMap& smap, U16 id) {
    tAttrIter iter = smap.find(id);
    if (iter == smap.end()) {
        sSmartAttr sm; sm.id = id;
        sm.type = eAttrType((id >> 8) & 0xFF);
        cCoreUtil::LookupAttributeName(sm.id, sm.name);
        smap[id] = sm;
        iter = smap.find(id);
    }
    return iter->second;
}

bool ApiUtil::SetSmartRaw(tAttrMap& smap, U16 id, U32 lo, U32 hi) {
    sSmartAttr& attr = GetSmartAttrRef(smap, id);
    attr.loraw = lo; attr.hiraw = hi; return true;
}

STR ApiUtil::ToAtaAttrString(const StorageApi::sSmartAttr& sa, const char* sep) {
    char buffer[2048];
    sprintf(buffer, "%02X%s%30s%s%3d%s%3d%s%3d%s%08d%s%08d%s%s",
            sa.id, sep, sa.name.c_str(), sep,
            sa.value, sep, sa.worst, sep, sa.threshold, sep,
            sa.loraw, sep, sa.hiraw, sep, sa.note.c_str());
    return STR(buffer);
}

STR ApiUtil::ToAttrValueString(U16 id, U32 loraw, U32 hiraw) {
    char buffer[1024];
    switch(id) {
        case SMA_DATA_UNIT_READ   :
        case SMA_DATA_UNIT_WRITTEN: {
            U64 lba = (((uint64_t)hiraw << 32) | loraw) * 1000;
            F64 lba_gb = ((lba * 100) >> 21) / 100.0;
            sprintf(buffer, "%0.2f GB", lba_gb);

            if (lba_gb >= 1024) {
                lba = lba >> 10;
                F64 lba_tb = ((lba * 100) >> 21) / 100.0;
                sprintf(buffer, "%0.2f TB", lba_tb);
            }
        } break;

        case SMA_COMPOSITE_TEMP   : {
            U32 celsius = loraw - 273; sprintf(buffer, "%d C", celsius);
        } break;

        case SMA_CTRL_BUSY_TIME   : sprintf(buffer, "%d m", loraw); break;

        case SMA_POWER_CYCLE        :
        case SMA_NVME_POWER_ON_HOURS:
        case SMA_UNSAFE_SHUTDOWNS   :
        case SMA_SPARE_PERCENTAGE   :
        case SMA_SPARE_THRESHOLD    :
        case SMA_PERCENTAGE_USED    :
        case SMA_TEMP_SENSOR_1      :
        case SMA_TEMP_SENSOR_2      :
        case SMA_TEMP_SENSOR_3      :
        case SMA_TEMP_SENSOR_4      :
        case SMA_TEMP_SENSOR_5      :
        case SMA_TEMP_SENSOR_6      :
        case SMA_TEMP_SENSOR_7      :
        case SMA_TEMP_SENSOR_8      :
        case SMA_TEMP_WARNING       :
        case SMA_TEMP_CRITICAL      :
        case SMA_MEDIA_ERRORS       :
        case SMA_ERROR_LOG_COUNT    :
        case SMA_HOST_CMD_READ      :
        case SMA_HOST_CMD_WRITTEN   : sprintf(buffer, "%d", loraw); break;

        default: sprintf(buffer, "%d", loraw); break;
    }

        return STR(buffer);
}

STR ApiUtil::ToNvmeAttrString(const StorageApi::sSmartAttr& sa, const char* sep) {
    STR vstr = ApiUtil::ToAttrValueString(sa.id, sa.loraw, sa.hiraw);
    char buffer[2048]; sprintf(buffer, "%30s%s%16s%s%10d",
                               sa.name.c_str(), sep, vstr.c_str(), sep, sa.loraw);
    return STR(buffer);
}

void ApiUtil::UpdateDriveInfo(const sDRVINFO& src, sDriveInfo& dst) {

    dst.name = src.IdentifyInfo.DriveName;

    if (1) {
        ApiUtil::ConvertIdentify(src.IdentifyInfo, dst.id);
    }

    if (1) {
        // Clone SMART info
        tAttrMap& dm = dst.si.amap; sSmartAttr attr;
        const tATTRMAP& sm = src.SmartInfo.AttrMap;
        for (tATTRMAPCITR it = sm.begin(); it != sm.end(); it++) {
            ApiUtil::ConvertSmartAttr(it->second, attr); dm[it->first] = attr;
        }
    }

    if (1) { // Others info
        #define MAP_ITEM(name, attrid) do { \
            sSmartAttr attr; if (ApiUtil::GetSmartAttr(dst.si.amap, attrid, attr)) { \
                dst.name = ((U64)attr.hiraw << 32) | attr.loraw; }} while(0)
        MAP_ITEM(temp, SMA_TEMPERATURE_CELSIUS);
        MAP_ITEM(tread, SMA_TOTAL_LBA_READ);
        MAP_ITEM(twrtn, SMA_TOTAL_LBA_WRITTEN);
        MAP_ITEM(health, SMA_REMAINING_LIFE_LEFT);
        #undef MAP_ITEM
    }
}

bool ApiUtil::IsTrimSupported(HDL hdl) {
    DEVICE_TRIM_DESCRIPTOR tdesc = { 0 };
    DWORD retlen = 0;

    STORAGE_PROPERTY_QUERY query;
    query.PropertyId = StorageDeviceTrimProperty;
    query.QueryType = PropertyStandardQuery;

    BOOL result = DeviceIoControl((HANDLE) hdl, IOCTL_STORAGE_QUERY_PROPERTY,
                                  &query, sizeof(query), &tdesc, sizeof(tdesc),
                                  &retlen, NULL);

    return (!result || (retlen != sizeof(tdesc))) ? false : tdesc.TrimEnabled;
}
