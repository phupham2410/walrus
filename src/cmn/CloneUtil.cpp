
#include "CloneUtil.h"
#include "CoreUtil.h"

#include <windows.h>
#include "winioctl.h"

using namespace std;
using namespace StorageApi;
using namespace DiskCloneUtil;

#include <diskguid.h>
#include <ntdddisk.h>

static void UtilSetGuid(GUID& g, U32 d1, U16 d2, U16 d3, U64 d4) {
    g.Data1 = d1; g.Data2 = d2; g.Data3 = d3;
    for (int i = 7; i >= 0; i--) {
        g.Data4[7-i] = (d4 >> (8*i)) & 0xFF;
    }
}

static eRetCode GetDriveIndex(const std::string& name, U32& index) {
    const char* p = name.c_str();
    while(*p && !INRANGE('0', '9' + 1, *p)) p++;
    if (!*p) return RET_INVALID_ARG;
    sscanf(p, "%d", &index); return RET_OK;
}

static string GetDriveName(U32 index) {
    stringstream sstr;
    sstr << "\\\\.\\PhysicalDrive" << index;
    return sstr.str();
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

static eRetCode UtilOpenFile(U32 drvidx, HDL& hdl) {
    string name = GetDriveName(drvidx);
    cout << "Opening drive " << name << endl;
    if (RET_OK != StorageApi::Open(name, hdl)) return RET_FAIL;
    return RET_OK;
}

static eRetCode UtilSeekFile(HDL hdl, S64 offset, U8 startpos) {
    LARGE_INTEGER low, high; high.QuadPart = 0; low.QuadPart = offset;
    startpos %= 3;
    if (SetFilePointerEx((HANDLE)hdl, low, &high, startpos) == INVALID_SET_FILE_POINTER)
        return RET_FAIL;
    return RET_OK;
}

static bool ToGptPartTypeString(U32 t, string& tstr) {
    tstr = "";
    switch(t) {
        #define MAP_ITEM(key, value)  case key: tstr = value; break;
        MAP_ITEM(DPT_PARTITION_BASIC_DATA_GUID, "primary")
        MAP_ITEM(DPT_PARTITION_SYSTEM_GUID, "efi")
        MAP_ITEM(DPT_PARTITION_MSFT_RESERVED_GUID, "msr")
        default: return false;
        #undef MAP_ITEM
    }
    return true;
}

static bool ToGptPartIdString(U32 t, string& id) {
    id = "";
    switch(t) {
        #define MAP_ITEM(key, value)  case key: id = value; break;
        MAP_ITEM(DPT_PARTITION_MSFT_RECOVERY_GUID, "de94bba4-06d1-4d40-a16a-bfd50179d6ac")
        MAP_ITEM(DPT_PARTITION_ENTRY_UNUSED_GUID, "00000000-0000-0000-0000-000000000000")
        MAP_ITEM(DPT_PARTITION_LDM_METADATA_GUID, "5808c8aa-7e8f-42e0-85d2-e1e90434cfb3")
        MAP_ITEM(DPT_PARTITION_LDM_DATA_GUID, "af9b60a0-1431-4f62-bc68-3311714a69ad")
        default: return false;
        #undef MAP_ITEM
    }
    return true;
}

eRetCode DiskCloneUtil::GetDriveInfo(U32 srcidx, sDcDriveInfo& si) {
    HDL hdl;
    if (RET_OK != UtilOpenFile(srcidx, hdl)) return RET_INVALID_ARG;

    // Read DriveLayout and Partitions
    // Fill in si

    si.drvidx = 0; si.parr.clear();

    eRetCode ret;
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

static eRetCode MatchPartition(U64 addr, U64 size, const sDcDriveInfo& si, U32& idx) {
    const vector<sDcPartInfo>& parr = si.parr;
    for (U32 i = 0, pcnt = parr.size(); i < pcnt; i++) {
        const sDcPartInfo& p = parr[i];
        if (p.start != addr) continue;
        if (p.psize > size) return RET_CONFLICT;
        idx = i; return RET_OK;
    }
    return RET_FAIL;
}

#define CAP1KB (1 << 10)
#define CAP1MB (1 << 20)

eRetCode DiskCloneUtil::FilterPartition(tConstAddrArray &parr, sDcDriveInfo& si) {
    vector<sDcPartInfo> out;
    set<U64> aset; // address map: address -> size;

    // address matching and filtering
    for (U32 i = 0, pcnt = parr.size(); i < pcnt; i++) {
        const tPartAddr& pa = parr[i];
        U64 addr = pa.first, size = pa.second; U32 idx;
        if (aset.find(addr) != aset.end()) return StorageApi::RET_CONFLICT;
        if (RET_OK != MatchPartition(addr, size, si, idx)) continue;

        sDcPartInfo info = si.parr[idx];
        size = ((size + CAP1MB - 1) / CAP1MB) * CAP1MB; // round to MB
        info.nsize = size; out.push_back(info);
    }
    si.parr = out; return out.size() ? RET_OK : RET_EMPTY;
}

// update start & index of target drive
eRetCode DiskCloneUtil::GenDestRange(const sDcDriveInfo& si, U32 dstidx, sDcDriveInfo& di) {
    U64 start = 4096; // partition table size in byte
    U64 gap = 100 * 1024; // gap 100KB between partitions
    di.drvidx = dstidx; di.parr.clear();
    for (U32 i = 0, maxi = si.parr.size(); i < maxi; i++) {
        const sDcPartInfo& s = si.parr[i];
        sDcPartInfo d = s;
        d.pidx = di.parr.size() + 1; // start from 1
        d.start = start; d.psize = s.nsize;
        di.parr.push_back(d);
        start += d.psize + gap;
    }
    return di.parr.size() ? RET_OK : RET_EMPTY;
}

eRetCode DiskCloneUtil::RemovePartTable(U32 dstidx) {
    HDL hdl;
    if (RET_OK != UtilOpenFile(dstidx, hdl)) return RET_FAIL;

    stringstream sstr;
    const int zsize = 4096; U8 zbuf[zsize] = {0};

    if (RET_OK != UtilSeekFile(hdl, 0, 0)) return RET_FAIL;
    WriteFile((HANDLE) hdl, zbuf, zsize, NULL, NULL); // Master table

    if (RET_OK != UtilSeekFile(hdl, -zsize, 2)) return RET_FAIL;
    WriteFile((HANDLE) hdl, zbuf, zsize, NULL, NULL); // Slave table

    Close(hdl);
    return RET_OK;
}

static eRetCode GenCreatePartScript(const sDcPartInfo& d, string& script) {
    stringstream sstr;
    string typestr, idstr;

    U64 size_in_mb = ((d.nsize + CAP1MB - 1) / CAP1MB);
    U64 offset_in_kb = ((d.start + CAP1KB - 1) / CAP1KB);

    if (ToGptPartTypeString(d.ptype, typestr)) {
        sstr << "create partition " << typestr
             << " size=" << size_in_mb
             << " offset=" << offset_in_kb << endl;
    }
    else if(ToGptPartIdString(d.ptype, idstr)) {
        sstr << "create partition primary"
             << " id=" << idstr
             << " size=" << size_in_mb
             << " offset=" << offset_in_kb << endl;
    }

    script = sstr.str();
    return RET_OK;
}

eRetCode DiskCloneUtil::GenCreatePartScript(const sDcDriveInfo& di, std::string& script) {
    // DiskPart script:
    // 1. create target drive: di.drvidx
    //    diskpart
    //    select disk dstidx
    //    create partition primary size=?? offset=??
    //    exit

    stringstream sstr;
    sstr << "select disk " << di.drvidx << endl;

    for (U32 i = 0; i < di.parr.size(); i++) {
        const sDcPartInfo& pi = di.parr[i]; string subscr;
        if (RET_OK != ::GenCreatePartScript(pi, subscr)) return RET_FAIL;
        sstr << subscr << endl;
    }

    sstr << "exit" << endl;
    script = sstr.str();
    return RET_OK;
}

static string GenRandomTimeString() {
    return string(tmpnam(NULL));
}

eRetCode DiskCloneUtil::ExecScript(std::string& script) {
    // Start child process
    // Execute diskpart /s script

    string fname = "./" + GenRandomTimeString();
    ofstream fstr; fstr.open (fname);
    fstr << script << endl; fstr.close();

    stringstream cstr; cstr << "diskpart /s " << fname;
    system(cstr.str().c_str());
    return RET_OK;
}

static eRetCode CopyPartition(StorageApi::HDL shdl, U64 sadd, U64 size, StorageApi::HDL dhdl, U64 dadd) {
    // Copy data block from src_drv to dst_drv
    if ((size % 512) || (sadd % 512) || (dadd % 512)) return RET_INVALID_ARG;

    U32 buf_sec = 256, buf_size = buf_sec << 9;
    U8* buf = (U8*) malloc(buf_size);
    if (!buf) return StorageApi::RET_OUT_OF_MEMORY;
    memset(buf, 0x5A, buf_size);

    U32 wrk_sec = buf_sec;
    U64 src_sec = size >> 9, rem_sec = src_sec;

    // Seeking
    if (RET_OK != UtilSeekFile(shdl, sadd, 0)) return RET_FAIL;
    if (RET_OK != UtilSeekFile(dhdl, dadd, 0)) return RET_FAIL;

    // threshold to update progress
    U64 load = 0, thr = rem_sec / 100;
    U64 progress = 0;

    while(rem_sec) {
        wrk_sec = MIN2(wrk_sec, rem_sec);

        // copy from src_lba to dst_lba
        DWORD tmp_size = 0, read_size = wrk_sec << 9;
        if (TRUE != ReadFile((HANDLE) shdl, buf, read_size, &tmp_size, NULL)) break;
        if (TRUE != WriteFile((HANDLE) dhdl, buf, tmp_size, NULL, NULL)) break;
        if (!tmp_size) break;

        wrk_sec = tmp_size >> 9; load += wrk_sec;
        if (load > thr) { load = 0; progress++; }
        rem_sec -= wrk_sec;
    }

    return rem_sec ? RET_FAIL : RET_OK;
}

eRetCode DiskCloneUtil::ClonePartitions(const sDcDriveInfo& si, const sDcDriveInfo& di) {
    U32 pcnt = si.parr.size();
    if (pcnt != di.parr.size()) return StorageApi::RET_INVALID_ARG;

    HDL shdl, dhdl;
    if (RET_OK != UtilOpenFile(si.drvidx, shdl)) return RET_FAIL;
    if (RET_OK != UtilOpenFile(di.drvidx, dhdl)) { Close(shdl); return RET_FAIL; }

    U32 i;
    for (i = 0; i < pcnt; i++) {
        const sDcPartInfo& sp = si.parr[i];
        const sDcPartInfo& dp = di.parr[i];
        if (RET_OK != CopyPartition(shdl, sp.start, sp.psize, dhdl, dp.start)) break;
    }

    Close(shdl); Close(dhdl);

    return (i == pcnt) ? RET_OK : RET_FAIL;
}

eRetCode DiskCloneUtil::HandleCloneDrive(CSTR& dstdrv, CSTR& srcdrv,
                          tConstAddrArray& parr, volatile StorageApi::sProgress* p) {
    U32 sidx, didx;
    if (RET_OK != GetDriveIndex(srcdrv, sidx)) return RET_INVALID_ARG;
    if (RET_OK != GetDriveIndex(dstdrv, didx)) return RET_INVALID_ARG;

    sDcDriveInfo si, di; string script;
    if (RET_OK != DiskCloneUtil::GetDriveInfo(sidx, si)) return RET_FAIL;
    if (RET_OK != DiskCloneUtil::FilterPartition(parr, si)) return RET_FAIL;
    if (RET_OK != DiskCloneUtil::GenDestRange(si, didx, di)) return RET_FAIL;
    if (RET_OK != DiskCloneUtil::RemovePartTable(didx)) return RET_FAIL;
    if (RET_OK != DiskCloneUtil::GenCreatePartScript(di, script)) return RET_FAIL;
    if (RET_OK != DiskCloneUtil::ExecScript(script)) return RET_FAIL;
    if (RET_OK != DiskCloneUtil::ClonePartitions(si, di)) return RET_FAIL;

    return RET_OK;
}


















