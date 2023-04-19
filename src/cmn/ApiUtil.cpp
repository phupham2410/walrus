
#include "ApiUtil.h"

using namespace StorageApi;

void ApiUtil::ConvertSmartAttr(const sSMARTATTR& src, sSmartAttr& dst) {
    dst.reset();
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

bool ApiUtil::GetSmartAttr(const tAttrMap& smap, U8 id, sSmartAttr& attr) {
    tAttrConstIter iter = smap.find(id);
    if (iter == smap.end()) return false;
    attr = iter->second; return true;
}

bool ApiUtil::GetSmartRaw(const tAttrMap& smap, U8 id, U32& lo, U32& hi) {
    sSmartAttr attr;
    if (!ApiUtil::GetSmartAttr(smap, id, attr)) return false;
    lo = attr.loraw; hi = attr.hiraw; return true;
}

bool ApiUtil::GetSmartValue(const tAttrMap& smap, U8 id, U8& val) {
    sSmartAttr attr;
    if (!ApiUtil::GetSmartAttr(smap, id, attr)) return false;
    val = attr.value; return true;
}

sSmartAttr& ApiUtil::GetSmartAttrRef(tAttrMap& smap, U8 id) {
    tAttrIter iter = smap.find(id);
    if (iter == smap.end()) {
        smap[id] = sSmartAttr();
        iter = smap.find(id);
    }
    return iter->second;
}

bool ApiUtil::SetSmartRaw(tAttrMap& smap, U8 id, U32 lo, U32 hi) {
    sSmartAttr& attr = GetSmartAttrRef(smap, id);
    attr.loraw = lo; attr.hiraw = hi; return true;
}

bool ApiUtil::SetSmartRaw(tAttrMap& smap, U8 id, U32 lo, U32 hi, U32 thr) {
    sSmartAttr& attr = GetSmartAttrRef(smap, id);
    attr.loraw = lo; attr.hiraw = hi; attr.threshold = thr; return true;
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

    if (1) {
        // Others
        sSmartAttr attr;
        if (ApiUtil::GetSmartAttr(dst.si.amap, SMA_TEMPERATURE_CELSIUS, attr)) {
            dst.temp = ((U64)attr.hiraw << 32) | attr.loraw;
        }

        if (ApiUtil::GetSmartAttr(dst.si.amap, SMA_TOTAL_LBA_READ, attr)) {
            dst.tread = ((U64)attr.hiraw << 32) | attr.loraw;
        }

        if (ApiUtil::GetSmartAttr(dst.si.amap, SMA_TOTAL_LBA_WRITTEN, attr)) {
            dst.twrtn = ((U64)attr.hiraw << 32) | attr.loraw;
        }
    }
}
