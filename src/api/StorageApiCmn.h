
#include <math.h>
#include <time.h>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <windows.h>

#include "HexFrmt.h"
#include "StorageApi.h"

// Fake implementation of StorageApi
using namespace StorageApi;

// ------------------------------------------------------------
// Utilities

#define FIXME(flag, msg) \
    if (flag) cout << "Unimplemented: " << msg << endl

#define ARRAY_SIZE(name) (sizeof(name) / sizeof(name[0]))

#define PACK16(B1,B0) (((B1)<<8) | (B0))
#define PACK32(B3,B2,B1,B0) (((B3)<<24) | ((B2)<<16) | ((B1)<<8) | (B0))

#define MIN2(a,b) (((a) < (b)) ? (a) : (b))
#define MAX2(a,b) (((a) > (b)) ? (a) : (b))

// ------------------------------------------------------------
// Constants

#define SMA_TEMPERATURE 194        // C2
#define SMA_TOTAL_HOST_READ 242    // F2
#define SMA_TOTAL_HOST_WRITTEN 241 // F1

#define MASK16B 0xFFFF
#define MASK32B 0xFFFFFFFF

#define ENDL std::endl
#define ENDL2 std::endl << std::endl
#define SETW(n) std::left << std::setw(n)
#define SETF(c) std::left << std::setfill(c)

// ------------------------------------------------------------
// Manage progress

#define INIT_PROGRESS(prog, load)     do { if (prog) prog->init(load); } while(0)
#define UPDATE_PROGRESS(prog, weight) do { if (prog) prog->progress += weight; } while(0)
#define CLOSE_PROGRESS(prog)          do { if (prog) prog->done = true; } while(0)
#define UPDATE_RETCODE(prog, code)    do { if (prog) prog->rval = code; } while(0)
#define UPDATE_INFO(prog, infostr)    do { if (prog) prog->info = infostr; } while(0)
#define FINALIZE_PROGRESS(prog, code) do { \
    UPDATE_RETCODE(prog, code); CLOSE_PROGRESS(prog); } while(0)
#define RETURN_IF_STOP(prog, code)    do { if (prog && prog->stop) { \
    FINALIZE_PROGRESS(prog, code); return code; }} while(0)
#define FINALIZE_IF_STOP(prog, func, param, code)    do { if (prog && prog->stop) { \
    if (func != NULL) { func(param); } FINALIZE_PROGRESS(prog, code); return code; }} while(0)

// ------------------------------------------------------------
// Common utilities

#define DEF_CONST(name) \
    name::name() { reset(); }
DEF_CONST(sFeature)
DEF_CONST(sPartition)
DEF_CONST(sPartInfo)
DEF_CONST(sIdentify)
DEF_CONST(sSmartAttr)
DEF_CONST(sSmartInfo)
DEF_CONST(sDriveInfo)
DEF_CONST(sProgress)
#undef DEF_CONST

// Note: Keep the order of features
sFeature::sFeature(const U8 *val) {
    const U8* pval = val;
    #define MAP_ITEM(name) \
        name = *(pval);
    MAP_ITEM(ataversion);
    MAP_ITEM(lbamode);
    MAP_ITEM(dma);
    MAP_ITEM(provision);
    MAP_ITEM(trim);
    MAP_ITEM(smart);
    MAP_ITEM(security);
    MAP_ITEM(ncq);
    MAP_ITEM(test);
    MAP_ITEM(erase);
    MAP_ITEM(dlcode);
    #undef MAP_ITEM
}

void sFeature::reset() {
    memset(this, 0x00, sizeof(sFeature));
}

void sPartition::reset() {
    name.clear();
    index = 0;
    cap = 0;
    addr.first = addr.second = 0;
}

void sPartInfo::reset() {
    parr.clear();
}

void sIdentify::reset() {
    model.clear();
    serial.clear();
    version.clear();
    confid.clear();
    cap = 0;
    features.reset();
}

void sSmartAttr::reset() {
    name.clear();
    note.clear();
    loraw = hiraw = 0;
    id = value = worst = threshold = 0;
}

void sSmartInfo::reset() {
    amap.clear();
}

void sDriveInfo::reset() {
    name.clear();
    id.reset();
    si.reset();
    pi.reset();
    temp = 0;
    tread = twrtn = 0;
    bustype = maxtfsec = 0;
}

void sProgress::reset() volatile {
    ready = false;
    stop = done = false;
    workload = progress = 0;
    rval = RET_OK; const_cast<std::string*>(&info)->clear();
    memset(const_cast<void*>((volatile void*)&priv), 0x00, sizeof(priv));
}

void sProgress::init(U32 load) volatile {
    reset(); workload = load; ready = true;
}

static STR ToBusTypeString(U32 bustype) {
    std::stringstream sstr;
    #define MAP_ITEM(name, val) \
        case val: sstr << #name << "(" << val << ")"; break

    switch(bustype) {
        MAP_ITEM(BusTypeUnknown          , 0x00);
        MAP_ITEM(BusTypeScsi             , 0x1 );
        MAP_ITEM(BusTypeAtapi            , 0x2 );
        MAP_ITEM(BusTypeAta              , 0x3 );
        MAP_ITEM(BusType1394             , 0x4 );
        MAP_ITEM(BusTypeSsa              , 0x5 );
        MAP_ITEM(BusTypeFibre            , 0x6 );
        MAP_ITEM(BusTypeUsb              , 0x7 );
        MAP_ITEM(BusTypeRAID             , 0x8 );
        MAP_ITEM(BusTypeiScsi            , 0x9 );
        MAP_ITEM(BusTypeSas              , 0xA );
        MAP_ITEM(BusTypeSata             , 0xB );
        MAP_ITEM(BusTypeSd               , 0xC );
        MAP_ITEM(BusTypeMmc              , 0xD );
        MAP_ITEM(BusTypeVirtual          , 0xE );
        MAP_ITEM(BusTypeFileBackedVirtual, 0xF );
        MAP_ITEM(BusTypeMaxReserved      , 0x7F);
    }
    #undef MAP_ITEM
    return sstr.str();
}

STR StorageApi::ToString(const StorageApi::sFeature& ft, U32 indent) {
    std::stringstream sstr; STR prefix (indent, ' ');
    #define MAP_FLAG(e, name, flag) \
        sstr << prefix << name << ": " << (flag ? "Yes" : "No"); if (e) sstr << ENDL
    #define MAP_ITEM(e, name, value) \
        sstr << prefix << name << ": " << (U32) value; if (e) sstr << ENDL
    MAP_ITEM(1, "ATA Version", ft.ataversion);
    MAP_FLAG(1, "LBA 48B", ft.lbamode);
    MAP_FLAG(1, "DMA", ft.dma);
    MAP_FLAG(1, "Over Provision", ft.provision);
    MAP_FLAG(1, "TRIM Command", ft.trim);
    MAP_FLAG(1, "S.M.A.R.T.", ft.smart);
    MAP_FLAG(1, "Security", ft.security);
    MAP_FLAG(1, "NCQ", ft.ncq);
    MAP_FLAG(1, "Self Test", ft.test);
    MAP_FLAG(1, "Secure Wipe", ft.erase);
    MAP_FLAG(0, "Firmware Upgrade", ft.dlcode);
    #undef MAP_ITEM
    #undef MAP_FLAG
    return sstr.str();
}

STR StorageApi::ToString(eRetCode code) {
    #define MAP_ITEM(val) case RET_##val: return #val
    switch(code) {
        MAP_ITEM(OK            );
        MAP_ITEM(FAIL          );
        MAP_ITEM(OUT_OF_MEMORY );
        MAP_ITEM(LOGIC         );
        MAP_ITEM(INVALID_ARG   );
        MAP_ITEM(XDATA         );
        MAP_ITEM(NOT_SUPPORT   );
        MAP_ITEM(OUT_OF_SPACE  );
        MAP_ITEM(CONFLICT      );
        MAP_ITEM(TIME_OUT      );
        MAP_ITEM(ABANDONED     );
        MAP_ITEM(BUSY          );
        MAP_ITEM(EMPTY         );
        MAP_ITEM(NOT_IMPLEMENT );
        MAP_ITEM(NOT_FOUND     );
        MAP_ITEM(NO_PERMISSION );
        MAP_ITEM(ABORTED       );
    }
    return "UNKNOWN_CODE";
    #undef MAP_ITEM
}

STR StorageApi::ToString(const StorageApi::sIdentify& id, U32 indent) {
    std::stringstream sstr; STR prefix (indent, ' '); U32  sub = indent + 2;
    #define MAP_ITEM(nlmid, nlend, text, val) do { \
        sstr << prefix << text << ": "; if (nlmid) { sstr << ENDL; } \
        sstr << val; if (nlend) { sstr << ENDL; }} while(0)
    MAP_ITEM(0,1, "Model"           , id.model);
    MAP_ITEM(0,1, "Serial"          , id.serial);
    MAP_ITEM(0,1, "Firmware Version", id.version);
    MAP_ITEM(0,1, "Configuration ID", id.confid);
    MAP_ITEM(0,1, "Capacity(GB)"    , id.cap);
    MAP_ITEM(1,1, "Features"        , ToString(id.features, sub));
    #undef MAP_ITEM
    return sstr.str();
}

STR StorageApi::ToString(const StorageApi::sSmartAttr& sa, U32 indent) {
    char buffer[2048]; STR prefix (indent, ' ');
    sprintf(buffer, "%02X %30s %3d %3d %06d %06d %3d %s",
            sa.id, sa.name.c_str(), sa.value, sa.threshold,
            sa.loraw, sa.hiraw, sa.worst, sa.note.c_str());
    return prefix + std::string(buffer);
}

STR StorageApi::ToString(const StorageApi::sSmartInfo& sm, U32 indent) {
    std::stringstream sstr; STR prefix (indent, ' ');
    for (tAttrConstIter iter = sm.amap.begin(); iter != sm.amap.end(); iter++) {
        sstr << prefix << ToString(iter->second, 0) << ENDL;
    }
    return sstr.str();
}

STR StorageApi::ToString(const StorageApi::sPartition& pt, U32 indent) {
    std::stringstream sstr; STR prefix (indent, ' ');
    #define MAP_ITEM(w, text, val, unit) do {\
        sstr << text << "("; if (w) sstr << SETW(w); sstr << std::right << val; \
        if (strlen(unit)) { sstr << " " << unit; } sstr << ") "; } while(0)
    sstr << prefix << "Partition: ";
    // MAP_ITEM(20, "Name" , pt.name, "");
    MAP_ITEM( 0, "Idx"  , pt.index, "");
    MAP_ITEM(14, "LBA"  , pt.addr.first, "");
    MAP_ITEM(14, "Count", pt.addr.second, "sectors");
    MAP_ITEM( 8, "Size" , pt.cap, "GB");
    #undef MAP_ITEM
    return sstr.str();
}

STR StorageApi::ToString(const StorageApi::sPartInfo& pi, U32 indent) {
    std::stringstream sstr; STR prefix (indent, ' '); U32  sub = indent + 2;
    for (tPartConstIter iter = pi.parr.begin(); iter != pi.parr.end(); iter++) {
        sstr << prefix << ToString(*iter, sub) << ENDL;
    }
    return sstr.str();
}

WSTR StorageApi::ToWideString(const StorageApi::sPartition& pt, U32 indent) {
    std::wstringstream sstr; WSTR prefix (indent, ' ');
    #define MAP_ITEM(w, text, val, unit) do { \
        sstr << text << "("; if (w) { sstr << SETW(w); } sstr << std::right << val; \
        if (strlen(unit)) { sstr << L" " << unit; } sstr << L") "; } while(0)
    sstr << prefix << L"Partition: ";
    MAP_ITEM(30, L"Name" , pt.name, "");
    MAP_ITEM( 0, L"Idx"  , pt.index, "");
    MAP_ITEM(14, L"LBA"  , pt.addr.first, "");
    MAP_ITEM(14, L"Count", pt.addr.second, "sectors");
    MAP_ITEM( 8, L"Size" , pt.cap, "GB");
    #undef MAP_ITEM
    return sstr.str();
}

WSTR StorageApi::ToWideString(const StorageApi::sPartInfo& pi, U32 indent) {
    std::wstringstream sstr; WSTR prefix (indent, ' '); U32  sub = indent + 2;
    for (tPartConstIter iter = pi.parr.begin(); iter != pi.parr.end(); iter++) {
        sstr << prefix << ToWideString(*iter, sub) << ENDL;
    }
    return sstr.str();
}

STR StorageApi::ToString(const StorageApi::sDriveInfo &drv, U32 indent) {
    std::stringstream sstr; STR prefix (indent, ' '); U32  sub = indent + 2;
    #define MAP_ITEM(nlmid, nlend, text, val) do { \
        sstr << prefix << text << ": "; \
        if (nlmid) { sstr << ENDL; } sstr << val; if (nlend) { sstr << ENDL; } } while(0)

    sstr << ToShortString(drv, indent);
    MAP_ITEM(1,1, "Partitions"        , ToString(drv.pi, sub));
    MAP_ITEM(1,1, "SMART Attributes"  , ToString(drv.si, sub));
    #undef MAP_ITEM
    return sstr.str();
}

STR StorageApi::ToShortString(const StorageApi::sDriveInfo &drv, U32 indent) {
    // Show drive info without Partitions and SMART
    std::stringstream sstr; STR prefix (indent, ' '); U32  sub = indent + 2;
    #define MAP_ITEM(nlmid, nlend, text, val) do { \
        sstr << prefix << text << ": "; \
        if (nlmid) { sstr << ENDL; } sstr << val; if (nlend) { sstr << ENDL; }} while(0)
    MAP_ITEM(0,1, "Physical Drive"    , drv.name);
    MAP_ITEM(1,1, "Identify"          , ToString(drv.id, sub));
    MAP_ITEM(0,1, "Temperature"       , drv.temp);
    MAP_ITEM(0,1, "Total Host Read"   , drv.tread);
    MAP_ITEM(0,1, "Total Host Written", drv.twrtn);
    MAP_ITEM(0,1, "Bus Type"          , ToBusTypeString(drv.bustype));
    MAP_ITEM(0,1, "MaxTransferSector" , drv.maxtfsec);
    #undef MAP_ITEM
    return sstr.str();
}

STR StorageApi::ToString(const U8 *bufptr, U32 bufsize) {
    return HexFrmt::ToHexString(bufptr, bufsize);
}

// --------------------------------------------------------------------------------
// Common static functions

static bool GetSmartAttr(const tAttrMap& smap, U8 id, sSmartAttr& attr) {
    tAttrConstIter iter = smap.find(id);
    if (iter == smap.end()) return false;
    attr = iter->second; return true;
}

static bool GetSmartRaw(const tAttrMap& smap, U8 id, U32& lo, U32& hi) {
    sSmartAttr attr;
    if (!GetSmartAttr(smap, id, attr)) return false;
    lo = attr.loraw; hi = attr.hiraw; return true;
}

static bool GetSmartValue(const tAttrMap& smap, U8 id, U8& val) {
    sSmartAttr attr;
    if (!GetSmartAttr(smap, id, attr)) return false;
    val = attr.value; return true;
}

static bool SetSmartRaw(tAttrMap& smap, U8 id, U32 lo, U32 hi) {
    tAttrIter iter = smap.find(id);
    if (iter == smap.end()) return false;
    sSmartAttr& attr = iter->second;
    attr.loraw = lo; attr.hiraw = hi;
    return true;
}
