#include <sstream>
#include "StorageApi.h"
#include "SystemUtil.h"
#include "HexFrmt.h"
#include "CommonUtil.h"

#include <fileapi.h>
#include <errhandlingapi.h>

#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string>

#include "tostring.h"

using namespace std;
using namespace StorageApi;

const string PHYDRV0 = "\\\\.\\PhysicalDrive0";
const string PHYDRV1 = "\\\\.\\PhysicalDrive1";
const string PHYDRV2 = "\\\\.\\PhysicalDrive2";

const string drvname = PHYDRV2;

#define DUMPERR(msg) \
    cout << msg << ". " \
         << SystemUtil::GetLastErrorString() << endl

eRetCode UtilOpenFile(const string& name, HDL& hdl) {
    cout << "Opening drive " << name << endl;
    if (RET_OK != StorageApi::Open(name, hdl)) {
        DUMPERR("Cannot open drive"); return RET_FAIL;
    }

    if ((HANDLE) hdl == INVALID_HANDLE_VALUE) {
        DUMPERR("Invalid handle value"); return RET_FAIL;
    }
    return RET_OK;
}

eRetCode UtilSeekFile(HDL hdl, S64 offset, U8 startpos) {
    LARGE_INTEGER low, high; high.QuadPart = 0; low.QuadPart = offset;
    startpos %= 3;
    if (SetFilePointerEx((HANDLE)hdl, low, &high, startpos) == INVALID_SET_FILE_POINTER) {
        if (0 == startpos) DUMPERR("Cannot seek file pointer from start position");
        if (1 == startpos) DUMPERR("Cannot seek file pointer from current position");
        if (2 == startpos) DUMPERR("Cannot seek file pointer from end position");
        return RET_FAIL;
    }
    return RET_OK;
}

eRetCode UtilGetDriveSize_Seeking(HDL hdl, S64 &drvsize) {
    LARGE_INTEGER tmp = {{0}}, high = {{0}}, cur = {{0}};

    if (SetFilePointerEx((HANDLE)hdl, tmp, &cur, 1) == INVALID_SET_FILE_POINTER) {
        DUMPERR("Cannot seek file pointer from current position"); return RET_FAIL;
    }

    if (SetFilePointerEx((HANDLE)hdl, tmp, &high, 2) == INVALID_SET_FILE_POINTER) {
        DUMPERR("Cannot seek file pointer from end"); return RET_FAIL;
    }

    drvsize = high.QuadPart;

    if (SetFilePointerEx((HANDLE)hdl, cur, &tmp, 0) == INVALID_SET_FILE_POINTER) {
        DUMPERR("Cannot seek file pointer to current position"); return RET_FAIL;
    }

    return RET_OK;
}

eRetCode UtilGetDriveSize(HDL hdl, U64& drvsize)
{
    DISK_GEOMETRY  pdg; BOOL ret = FALSE; DWORD rsize = 0;
    drvsize = 0;
    ret = DeviceIoControl((HANDLE)hdl, IOCTL_DISK_GET_DRIVE_GEOMETRY,
                              NULL, 0, &pdg, sizeof(pdg), &rsize, NULL);
    if (!ret) { DUMPERR("Cannot read drive geometry"); return RET_FAIL; }
    drvsize = pdg.Cylinders.QuadPart * (ULONG)pdg.TracksPerCylinder *
              (ULONG)pdg.SectorsPerTrack * (ULONG)pdg.BytesPerSector;
    return RET_OK;
}

#define ToKB(b) (((b * 100) >> 10) / 100.0)
#define ToMB(b) (((b * 100) >> 20) / 100.0)
#define ToGB(b) (((b * 100) >> 30) / 100.0)
#define ToTB(b) (((b * 100) >> 40) / 100.0)

void TestSeek(HDL hdl, S64 offset, U8 pos) {

    string pstr;
    if (pos == 0) pstr = "start";
    else if (pos == 1) pstr = "current";
    else if (pos == 2) pstr = "end";
    else cout << "[ERR] Invalid pos: " << pos << endl;

    U64 offlen = (offset < 0) ? -offset : offset;
    stringstream lstr;
    if      (offlen < ((U64)1 << 10)) lstr << offlen << " (bytes)";
    else if (offlen < ((U64)1 << 20)) lstr << ToKB(offlen) << " (KB)";
    else if (offlen < ((U64)1 << 30)) lstr << ToMB(offlen) << " (MB)";
    else if (offlen < ((U64)1 << 40)) lstr << ToGB(offlen) << " (GB)";
    else                              lstr << ToTB(offlen) << " (TB)";

    cout << "Seeking " << ((offset >= 0) ? "forward" : "backward")
         << " " << lstr.str() << " from " << pstr << ": ";

    if (RET_OK != UtilSeekFile(hdl, offset, pos)) {
        cout << "FAIL" <<  endl; return;
    }

    cout << "OK" << endl;
}

#define P100B 100
#define P10KB ((U64)100 << 10)
#define P10MB ((U64)100 << 20)
#define P10GB ((U64)100 << 30)
#define P10TB ((U64)100 << 40)

#define N100B ((S64)(-P100B))
#define N10KB ((S64)(-P10KB))
#define N10MB ((S64)(-P10MB))
#define N10GB ((S64)(-P10GB))
#define N10TB ((S64)(-P10TB))

void TestFileSeek() { // --> ok
    HDL hdl;
    if (RET_OK != UtilOpenFile(drvname, hdl)) return;

    S64 offset; U8 pos;

    pos = 2;
    for (U32 i = 0; i < 3; i++) {
        pos = i;
        offset = 0; TestSeek(hdl, offset, pos);
        offset = P100B; TestSeek(hdl, offset, pos);
        offset = P10KB; TestSeek(hdl, offset, pos);
        offset = P10MB; TestSeek(hdl, offset, pos);
        offset = P10GB; TestSeek(hdl, offset, pos);
        offset = P10TB; TestSeek(hdl, offset, pos);
        offset = N100B; TestSeek(hdl, offset, pos);
        offset = N10KB; TestSeek(hdl, offset, pos);
        offset = N10MB; TestSeek(hdl, offset, pos);
        offset = N10GB; TestSeek(hdl, offset, pos);
        offset = N10TB; TestSeek(hdl, offset, pos);
    }
    Close(hdl);
}

BOOL GetDriveGeometry(HDL hdl, DISK_GEOMETRY *pdg)
{
    BOOL bResult   = FALSE;                 // results flag
    DWORD junk     = 0;                     // discard results

    bResult = DeviceIoControl((HANDLE)hdl,                   // device to be queried
                              IOCTL_DISK_GET_DRIVE_GEOMETRY, // operation to perform
                              NULL, 0,                       // no input buffer
                              pdg, sizeof(*pdg),             // output buffer
                              &junk,                         // # bytes returned
                              (LPOVERLAPPED) NULL);          // synchronous I/O
    return (bResult);
}

void TestReadDriveSize()
{
    HDL hdl;
    if (RET_OK != UtilOpenFile(drvname, hdl)) return;

    U64 drvsize = 0;
    if (RET_OK != UtilGetDriveSize(hdl, drvsize)) return;

    cout << "Drive size: " << drvsize << " bytes" << endl
         << " (" << ToGB(drvsize) << " GBs)" << endl
         << " (" << ToTB(drvsize) << " TBs)" << endl;
    Close(hdl);
}

void ClearPartTable() { // --> ok
    HDL hdl;
    if (RET_OK != UtilOpenFile(drvname, hdl)) return;

    stringstream sstr;
    const int zsize = 4096; U8 zbuf[zsize] = {0};

    if (RET_OK != UtilSeekFile(hdl, 0, 0)) return;
    WriteFile((HANDLE) hdl, zbuf, zsize, NULL, NULL);

    if (RET_OK != UtilSeekFile(hdl, -zsize, 2)) return;
    WriteFile((HANDLE) hdl, zbuf, zsize, NULL, NULL);
}

void WriteFullDisk() {
    HDL hdl;
    if (RET_OK != UtilOpenFile(drvname, hdl)) return;

    U64 drvsize, cap_sec, rem_sec, wrt_byte, ttl_sec = 0, ttl_gb = 0;
    if (RET_OK != UtilGetDriveSize(hdl, drvsize)) return;

    U32 buf_sec = 65536, wrt_sec = buf_sec; // 32M buffer
    void* buf = (void*) malloc(buf_sec * 512);
    if (!buf) { DUMPERR("Cannot alloc buffer"); return; }

    CommonUtil::FormatBuffer((U8*) buf, buf_sec, 0, wrt_sec, 2);

    cap_sec = drvsize >> 9; rem_sec = cap_sec;
    wrt_byte = wrt_sec * 512;

    for (U64 i = 0; rem_sec; i += wrt_sec) {
        if (wrt_sec > rem_sec) wrt_sec = rem_sec;
        wrt_byte = wrt_sec * 512;
        CommonUtil::FormatHeader((U8*) buf, buf_sec, i, wrt_sec, 3);
        if (!WriteFile((HANDLE) hdl, buf, wrt_byte, NULL, NULL)) {
            stringstream sstr; sstr << "Writing fail at LBA " << i;
            DUMPERR(sstr.str());
        }

        ttl_sec += wrt_sec;
        rem_sec = rem_sec - wrt_sec;

        if (ttl_sec >= (1 << 21)) {
            ttl_sec = 0; ttl_gb++;
            cout << "Written: " << ttl_gb << " GB" << endl;
        }
    }

    cout << "RemSec: " << rem_sec << endl;
}

// read size bytes from offset, store in buffer
U32 UtilReadSector(HDL hdl, U64 lba, U32 sec_cnt, U8* buffer, U32 bufsize) {
    U32 buf_sec = bufsize / 512;
    if (!buf_sec) return 0;

    U32 read_sec = MIN2(sec_cnt, buf_sec);
    U32 read_size = read_sec * 512;
    S64 read_pos = lba * 512;

    memset(buffer, 0x5A, bufsize);

    if (RET_OK != UtilSeekFile(hdl, read_pos, 0)) return 0;

    DWORD ret_size = 0;
    if (TRUE != ReadFile((HANDLE) hdl, buffer, read_size, &ret_size, NULL)) {
        DUMPERR("Cannot read file"); return 0;
    }

    return ret_size;
}

#define CAP0B    0
#define CAP100KB ((U64)100 << 10)
#define CAP500KB ((U64)500 << 10)
#define CAP100MB ((U64)100 << 20)
#define CAP500MB ((U64)500 << 20)
#define CAP005GB ((U64)  5 << 30)
#define CAP010GB ((U64) 10 << 30)
#define CAP020GB ((U64) 20 << 30)
#define CAP030GB ((U64) 30 << 30)
#define CAP100GB ((U64)100 << 30)
#define CAP200GB ((U64)200 << 30)
#define CAP100TB ((U64)100 << 40)

#define LBA0B    (CAP0B    >> 9)
#define LBA100KB (CAP100KB >> 9)
#define LBA100MB (CAP100MB >> 9)
#define LBA030GB (CAP030GB >> 9)
#define LBA100GB (CAP100GB >> 9)
#define LBA200GB (CAP200GB >> 9)
#define LBA100TB (CAP100TB >> 9)

void TestReadSector() {
    HDL hdl;
    if (RET_OK != UtilOpenFile(drvname, hdl)) return;

    U64 drvsize;
    if (RET_OK != UtilGetDriveSize(hdl, drvsize)) return;

    U64 drv_sec = drvsize >> 9;

    U64 lba[] = { LBA0B, LBA100KB, LBA100MB, LBA100GB, LBA200GB, LBA100TB };
    U32 cnt = sizeof(lba) / sizeof(lba[0]);

    U32 buf_sec = 32, buf_size = buf_sec * 512;
    U8* buf = (U8*) malloc(buf_size); memset(buf, 0x5A, buf_size);
    if (!buf) { DUMPERR("Cannot alloc memory"); return; }

    for (U64 i = 0; i < cnt; i++) {
        U64 addr = lba[i];
        if (addr >= drv_sec) continue; // Need to check boundary here !!!

        U32 rsize = UtilReadSector(hdl, addr, 1, buf, buf_size);
        cout << "Reading LBA 0x" << hex << addr << ": ";
        if (!rsize) cout << "Fail" << endl;
        else cout << endl << HexFrmt::ToHexString(buf, rsize) << endl;
    }
    Close(hdl);
}

// 0x87c068b6b72699c7
static void UtilSetGuid(GUID& g, U32 d1, U16 d2, U16 d3, U64 d4) {
    g.Data1 = d1;
    g.Data2 = d2;
    g.Data3 = d3;
    for (int i = 7; i >= 0; i--) {
        g.Data4[7-i] = (d4 >> (8*i)) & 0xFF;
    }
}

static void UtilSetGuid(GUID& g, GUID src) {
    g = src;
}

#include <combaseapi.h>

static void UtilNewGuid(GUID& g) {
    // HRESULT ret = CoCreateGuid(&g);
    // if (ret != S_OK) {
    //     cout << "Fail to create new guid" << endl;
    // }
}

eRetCode UtilInitDrive(HDL hdl, PARTITION_STYLE style) {
    CREATE_DISK dsk;
    dsk.PartitionStyle = style;
    if (style == PARTITION_STYLE_MBR) dsk.Mbr.Signature = 9999;
    else {
        GUID gid; UtilNewGuid(gid);
        dsk.Gpt.MaxPartitionCount = 128;
        dsk.Gpt.DiskId = gid;
    }

    BOOL ret; DWORD junk;
    ret = DeviceIoControl((HANDLE) hdl, IOCTL_DISK_CREATE_DISK,
                          &dsk, sizeof(dsk), NULL, 0, &junk, NULL);
    if (!ret) { DUMPERR("Cannot create disk"); return RET_FAIL; }

    ret = DeviceIoControl((HANDLE) hdl, IOCTL_DISK_UPDATE_PROPERTIES,
                              NULL, 0, NULL, 0, &junk, NULL);
    if (!ret) { DUMPERR("Cannot update properties"); return RET_FAIL; }

    return RET_OK;
}

void TestInitDriveMRB() {
    HDL hdl;
    if (RET_OK != UtilOpenFile(drvname, hdl)) return;

    if (RET_OK != UtilInitDrive(hdl, PARTITION_STYLE_MBR)) return;

    cout << "Init drive ok" << endl;
    Close(hdl);
}

void TestInitDriveGPT() {
    HDL hdl;
    if (RET_OK != UtilOpenFile(drvname, hdl)) return;

    if (RET_OK != UtilInitDrive(hdl, PARTITION_STYLE_GPT)) return;

    cout << "Init drive ok" << endl;
    Close(hdl);
}

#include <diskguid.h>
#include <ntdddisk.h>

eRetCode UtilGetDriveLayout(HDL hdl, U8* buffer, U32 bufsize) {
    if (!buffer) return StorageApi::RET_INVALID_ARG;
    memset(buffer, 0x00, bufsize);

    BOOL ret; DWORD rsize;
    ret = DeviceIoControl((HANDLE) hdl, IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                          NULL, 0, buffer, bufsize, &rsize, NULL);

    if (!ret) { DUMPERR("Cannot read partitions info ex"); return RET_FAIL; }
    return RET_OK;
}

void TestGetDriveLayout() {
    HDL hdl;
    if (RET_OK != UtilOpenFile(drvname, hdl)) return;

    const U32 bufsize = 4096; U8 buffer[bufsize];
    memset(buffer, 0x00, bufsize);

    cout << "Get layout information: ";
    if (RET_OK != UtilGetDriveLayout(hdl, buffer, bufsize)) {
        cout << "FAIL" << endl; return;
    }
    cout << endl;

    DRIVE_LAYOUT_INFORMATION_EX* dli = (DRIVE_LAYOUT_INFORMATION_EX*) buffer;
    U32 count = dli->PartitionCount;
    PARTITION_INFORMATION_EX* pi = (PARTITION_INFORMATION_EX*) &dli->PartitionEntry[0];

    cout << ToString(*dli) << endl;

    for (U32 i = 0; i < count; i++, pi++) {
        if (!pi->PartitionNumber) continue;
        cout << "Entry " << i << endl << ToString(*pi) << endl;
    }

    Close(hdl);
}

eRetCode UtilSetDriveLayoutMRB(HDL hdl, U32 itemcnt, U64 itemsize) {
    LARGE_INTEGER psize; psize.QuadPart = itemsize;

    DWORD lisize = sizeof(DRIVE_LAYOUT_INFORMATION_EX);
    DWORD pisize = sizeof(PARTITION_INFORMATION_EX);

    DWORD len = lisize + itemcnt * pisize;
    DRIVE_LAYOUT_INFORMATION_EX *pli = (DRIVE_LAYOUT_INFORMATION_EX*) malloc(len);
    if (pli == NULL) { DUMPERR("Cannot allocate memory"); return RET_OUT_OF_MEMORY; }

    cout << "Layout size: " << lisize << endl;
    cout << "Partition size: " << pisize << endl;

    memset((void*)pli, 0x00, len);

    pli->PartitionStyle = PARTITION_STYLE_MBR;
    pli->PartitionCount = itemcnt;
    pli->Mbr.Signature = 99999;

    U32 hdr_sec = 2048;
    U32 gap_size = CAP500KB;
    U64 hdr_size = hdr_sec * 512;
    U64 start = hdr_size;

    for (U32 i = 0; i < itemcnt; i++) {

        PARTITION_INFORMATION_EX& p = pli->PartitionEntry[i];
        p.PartitionStyle = PARTITION_STYLE_MBR;
        p.StartingOffset.QuadPart = start + i * (psize.QuadPart + gap_size);
        p.PartitionLength.QuadPart = psize.QuadPart;
        p.PartitionNumber = i;
        p.RewritePartition = TRUE;
        p.Mbr.PartitionType = PARTITION_NTFT;
        p.Mbr.BootIndicator = TRUE;
        p.Mbr.RecognizedPartition = TRUE;
        p.Mbr.HiddenSectors = CAP500MB >> 9; // Unknown meaning
    }

    BOOL ret; DWORD rsize;
    ret = DeviceIoControl((HANDLE) hdl, IOCTL_DISK_SET_DRIVE_LAYOUT_EX,
                          pli, len, NULL, 0, &rsize, NULL);
    free(pli);
    if (!ret) { DUMPERR("Cannot add partitions"); return RET_FAIL; }

    return RET_OK;
}

void TestSetDriveLayoutMRB() {
    HDL hdl;
    if (RET_OK != UtilOpenFile(drvname, hdl)) return;

    cout << "Creating partitions ... ";
    if (RET_OK != UtilSetDriveLayoutMRB(hdl, 3, CAP030GB)) {
        cout << "FAIL" << endl; return;
    }

    cout << "OK" << endl;
    Close(hdl);
}

eRetCode UtilSetDriveLayoutGPT(HDL hdl, U32 itemcnt, U64 itemsize) {
    LARGE_INTEGER psize; psize.QuadPart = itemsize;

    DWORD lisize = sizeof(DRIVE_LAYOUT_INFORMATION_EX);
    DWORD pisize = sizeof(PARTITION_INFORMATION_EX);

    DWORD len = lisize + itemcnt * pisize;
    DRIVE_LAYOUT_INFORMATION_EX *pli = (DRIVE_LAYOUT_INFORMATION_EX*) malloc(len);
    if (pli == NULL) { DUMPERR("Cannot allocate memory"); return RET_OUT_OF_MEMORY; }

    cout << "Layout size: " << lisize << endl;
    cout << "Partition size: " << pisize << endl;

    memset((void*)pli, 0x00, len);

    pli->PartitionStyle = PARTITION_STYLE_GPT;
    pli->PartitionCount = itemcnt;
    pli->Mbr.Signature = 99999;

    U32 hdr_sec = 2048;
    U32 gap_size = CAP100KB;
    U64 hdr_size = hdr_sec * 512;
    U64 start = hdr_size;

    GUID gid; UtilSetGuid(gid, 0xebd0a0a2, 0xb9e5, 0x4433, 0x87c068b6b72699c7);

    for (U32 i = 0; i < itemcnt; i++) {
        wstring name = L"Basic data partition";
        PARTITION_INFORMATION_EX& p = pli->PartitionEntry[i];

        p.PartitionStyle = PARTITION_STYLE_GPT;
        p.StartingOffset.QuadPart = start + i * (psize.QuadPart + gap_size);
        p.PartitionLength.QuadPart = psize.QuadPart;
        p.PartitionNumber = i + 1; // index from 1
        p.RewritePartition = TRUE;

        p.Gpt.PartitionType = gid;
        p.Gpt.PartitionId = p.Gpt.PartitionType;
        p.Gpt.Attributes = 0;
        for (U32 i = 0; i < name.length(); i++) p.Gpt.Name[i] = name[i];
    }

    BOOL ret; DWORD rsize;
    ret = DeviceIoControl((HANDLE) hdl, IOCTL_DISK_SET_DRIVE_LAYOUT_EX,
                          pli, len, NULL, 0, &rsize, NULL);
    free(pli);
    if (!ret) { DUMPERR("Cannot set drive layout"); return RET_FAIL; }

    return RET_OK;
}

void TestSetDriveLayoutGPT() {
    HDL hdl;
    if (RET_OK != UtilOpenFile(drvname, hdl)) return;

    cout << "Creating partitions ... ";
    if (RET_OK != UtilSetDriveLayoutGPT(hdl, 4, CAP020GB)) {
        cout << "FAIL" << endl; return;
    }

    cout << "OK" << endl;
    Close(hdl);
}

enum ePartType {
    DPT_INVALID = 0,
    DPT_PARTITION_BASIC_DATA_GUID,
    DPT_PARTITION_ENTRY_UNUSED_GUID,
    DPT_PARTITION_SYSTEM_GUID,
    DPT_PARTITION_MSFT_RESERVED_GUID,
    DPT_PARTITION_LDM_METADATA_GUID,
    DPT_PARTITION_LDM_DATA_GUID,
    DPT_PARTITION_MSFT_RECOVERY_GUID,
};

struct sDiskPartInfo {
    U32 pidx;   // 1, 2, 3
    U32 ptype;  // PARTITION_BASIC_DATA_GUID ...
    U64 start;  // in bytes
    U64 psize;  // in bytes
    U64 nsize;
};

struct sSrcDriveInfo {
    U32 drvidx;
    U32 pcnt;
    sDiskPartInfo pinfo[128];
};

ePartType ConvertGptPartitionType(GUID& src) {
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

void ConvertPartInfo(PARTITION_INFORMATION_EX& p, sDiskPartInfo& dpi) {
    dpi.pidx = p.PartitionNumber;
    dpi.start = p.StartingOffset.QuadPart;
    dpi.psize = p.PartitionLength.QuadPart;
    dpi.nsize = dpi.psize + CAP005GB; // round up 10 GB

    switch(p.PartitionStyle) {
    case PARTITION_STYLE_RAW: dpi.ptype = DPT_PARTITION_BASIC_DATA_GUID; break;
    case PARTITION_STYLE_MBR: dpi.ptype = DPT_PARTITION_BASIC_DATA_GUID; break;
    case PARTITION_STYLE_GPT: dpi.ptype = ConvertGptPartitionType(p.Gpt.PartitionType); break;
    default: break;
    }
}

bool GetDriveIndex(const string& name, U32& index) {
    const char* p = name.c_str();
    while(*p && !INRANGE('0', '9' + 1, *p)) p++;
    if (!*p) return false;
    sscanf(p, "%d", &index); return true;
}

string GenScript_CleanDrive(sSrcDriveInfo& sdi) {
    stringstream sstr;
    sstr << "select disk " << sdi.drvidx << endl;
    for (U32 i = 0; i < sdi.pcnt; i++) {
        sDiskPartInfo& dpi = sdi.pinfo[i];
        sstr << "select partition " << dpi.pidx << endl;
        sstr << "delete partition" << endl;
    }
    return sstr.str();
}

static bool ToGptTypeString(U32 t, string& tstr) {
    tstr = "";
    #define MAP_ITEM(key, value)  case key: tstr = value; break;
    switch(t) {
        MAP_ITEM(DPT_PARTITION_BASIC_DATA_GUID, "primary")
        MAP_ITEM(DPT_PARTITION_SYSTEM_GUID, "efi")
        MAP_ITEM(DPT_PARTITION_MSFT_RESERVED_GUID, "msr")
        default: return false;
    }
    #undef MAP_ITEM
    return true;
}

static bool ToGptIdString(U32 t, string& id) {
    id = "";
    #define MAP_ITEM(key, value)  case key: id = value; break;
    switch(t) {
        MAP_ITEM(DPT_PARTITION_MSFT_RECOVERY_GUID, "de94bba4-06d1-4d40-a16a-bfd50179d6ac")
        MAP_ITEM(DPT_PARTITION_ENTRY_UNUSED_GUID, "00000000-0000-0000-0000-000000000000")
        MAP_ITEM(DPT_PARTITION_LDM_METADATA_GUID, "5808c8aa-7e8f-42e0-85d2-e1e90434cfb3")
        MAP_ITEM(DPT_PARTITION_LDM_DATA_GUID, "af9b60a0-1431-4f62-bc68-3311714a69ad")
        default: return false;
    }
    #undef MAP_ITEM
    return true;
}

string GenScript_GenPart(sSrcDriveInfo& s, U32 tidx) {
    stringstream sstr;
    sstr << "select disk " << tidx << endl;

    U64 offset_in_kb = 4, gap = CAP100KB;

    for (U32 i = 0; i < s.pcnt; i++) {
        sDiskPartInfo& d = s.pinfo[i];
        // create partition in target drive

        U64 size_in_mb = ((d.nsize + 1024 * 1024 - 1) >> 20); // in MB
        string typestr, idstr;
        if (ToGptTypeString(d.ptype, typestr)) {
            sstr << "create partition " << typestr
                 << " size=" << size_in_mb
                 << " offset=" << offset_in_kb << endl;
        }
        else if(ToGptIdString(d.ptype, idstr)) {
            sstr << "create partition primary"
                 << " id=" << idstr
                 << " size=" << size_in_mb
                 << " offset=" << offset_in_kb << endl;
        }

        offset_in_kb += ((size_in_mb << 20) + gap) >> 10; // unit: KB
    }
    return sstr.str();
}

void TestDiskPart_GenScript() {
    HDL hdl;
    if (RET_OK != UtilOpenFile(drvname, hdl)) return;

    const U32 bufsize = 4096; U8 buffer[bufsize];
    memset(buffer, 0x00, bufsize);

    cout << "Get layout information: ";
    if (RET_OK != UtilGetDriveLayout(hdl, buffer, bufsize)) {
        cout << "FAIL" << endl; return;
    }
    cout << endl;

    DRIVE_LAYOUT_INFORMATION_EX* dli = (DRIVE_LAYOUT_INFORMATION_EX*) buffer;
    U32 count = dli->PartitionCount;
    PARTITION_INFORMATION_EX* pi = (PARTITION_INFORMATION_EX*) &dli->PartitionEntry[0];

    cout << ToString(*dli) << endl;

    sSrcDriveInfo sdi;
    if (!GetDriveIndex(drvname, sdi.drvidx)) {
        cout << "Unknown drive index" << endl; return;
    }

    for (U32 i = 0; i < count; i++, pi++) {
        PARTITION_INFORMATION_EX& p = *pi;
        if (!p.PartitionNumber) continue;
        sDiskPartInfo& dpi = sdi.pinfo[sdi.pcnt++];
        ConvertPartInfo(p, dpi);
    }

    // Build DiskPart script
    string scr = GenScript_CleanDrive(sdi);
    cout << "DiskPart script: " << endl << scr << endl;

    if (1) {
        string frmt = GenScript_GenPart(sdi, 2);
        cout << "Format script: " << endl << frmt << endl;
    }

    Close(hdl);
}

void TestDiskClone() {
    const string src_drvname = PHYDRV0;
    const string dst_drvname = PHYDRV1;

    // clone Disk1::Part2 to Disk2::Part3 (PartNumber)

    // Open Disk1_PartTable
    // Read offset/size of Part2

    // Open Disk2 PartTable
    // Read offset/size of Part3
    // Do read/write

    U32 src_idx = 2, dst_idx = 2;

    // ------------------------------------------------------------
    // Get src/dst partition info

    StorageApi::sPartInfo spi, dpi;
    if (RET_OK != StorageApi::ScanPartition(src_drvname, spi)) return;
    if (RET_OK != StorageApi::ScanPartition(dst_drvname, dpi)) return;

    int si = -1, di = -1;
    for (int i = 0; i < spi.parr.size(); i++)
        if (spi.parr[i].index == src_idx) { si = i; break; }
    if (si < 0) { cout << "src part not found" << endl; return; }

    for (int i = 0; i < dpi.parr.size(); i++)
        if (dpi.parr[i].index == dst_idx) { di = i; break; }
    if (di < 0) { cout << "dst part not found" << endl; return; }

    sPartition& sp = spi.parr[si]; sPartition& dp = dpi.parr[di];

    // ------------------------------------------------------------
    // Validation

    U64 srcsize = sp.addr.second, dstsize = dp.addr.second;
    if (dstsize < srcsize) {
        cout << "dst not enough space" << endl; return;
    }

    // ------------------------------------------------------------
    // Open Src/Dst drives

    HDL sh, dh;
    if (RET_OK != UtilOpenFile(src_drvname, sh)) return;
    if (RET_OK != UtilOpenFile(dst_drvname, dh)) return;

    U32 buf_sec = 256, buf_size = buf_sec * 512;
    U8* buf = (U8*) malloc(buf_size);
    if (!buf) { DUMPERR("Cannot alloc memory"); return; }
    memset(buf, 0x5A, buf_size);

    U32 wrk_sec = buf_sec;
    U64 src_sec = srcsize >> 9, rem_sec = src_sec;
    U64 src_lba = sp.addr.first >> 9;
    U64 dst_lba = dp.addr.first >> 9;

    // Seeking
    if (RET_OK != UtilSeekFile(sh, sp.addr.first, 0)) {
        DUMPERR("Cannot seek src drive"); return;
    }
    if (RET_OK != UtilSeekFile(dh, dp.addr.first, 0)) {
        DUMPERR("Cannot seek dst drive"); return;
    }

    U64 progress = 0, load = 0, thr = rem_sec / 100;

    for (U64 i = 0; rem_sec; i++) {
        wrk_sec = MIN2(wrk_sec, rem_sec);

        // copy from src_lba to dst_lba
        DWORD tmp_size = 0, read_size = wrk_sec * 512;

        if (TRUE != ReadFile((HANDLE) sh, buf, read_size, &tmp_size, NULL)) {
            DUMPERR("");
            cout << "Cannot read lba " << src_lba << endl; break;
        }

        if (TRUE != WriteFile((HANDLE) dh, buf, tmp_size, NULL, NULL)) {
            DUMPERR("");
            cout << "Cannot write lba " << dst_lba << endl; break;
        }

        load += wrk_sec;
        if (load > thr) {
            load = 0; progress++;
            cout << "\rProgress: " << progress << "%";
        }

        src_lba += wrk_sec;
        dst_lba += wrk_sec;
        rem_sec -= wrk_sec;
    }

    cout << endl << "Done!!!";

    Close(sh); Close(dh);
}

void TestReadEfi() {

    const string src_drvname = PHYDRV0;
    const string dst_drvname = PHYDRV1;

    // Read & compare EFI partition of 2 drives

    U32 src_idx = 1, dst_idx = 1;

    // ------------------------------------------------------------
    // Get src/dst partition info

    StorageApi::sPartInfo spi, dpi;
    if (RET_OK != StorageApi::ScanPartition(src_drvname, spi)) return;
    if (RET_OK != StorageApi::ScanPartition(dst_drvname, dpi)) return;

    int si = -1, di = -1;
    for (int i = 0; i < spi.parr.size(); i++)
        if (spi.parr[i].index == src_idx) { si = i; break; }
    if (si < 0) { cout << "src part not found" << endl; return; }

    for (int i = 0; i < dpi.parr.size(); i++)
        if (dpi.parr[i].index == dst_idx) { di = i; break; }
    if (di < 0) { cout << "dst part not found" << endl; return; }

    sPartition& sp = spi.parr[si]; sPartition& dp = dpi.parr[di];

    // ------------------------------------------------------------
    // Validation

    U64 srcsize = sp.addr.second, dstsize = dp.addr.second;
    U64 minsize = MIN2(srcsize, dstsize);

    // ------------------------------------------------------------
    // Open Src/Dst drives

    HDL sh, dh;
    if (RET_OK != UtilOpenFile(src_drvname, sh)) return;
    if (RET_OK != UtilOpenFile(dst_drvname, dh)) return;

    U32 buf_sec = 1, buf_size = buf_sec * 512;
    U8* sbuf = (U8*) malloc(buf_size);
    if (!sbuf) { DUMPERR("Cannot alloc memory"); return; }
    memset(sbuf, 0x5A, buf_size);

    U8* dbuf = (U8*) malloc(buf_size);
    if (!dbuf) { DUMPERR("Cannot alloc memory"); return; }
    memset(dbuf, 0x5A, buf_size);

    U32 wrk_sec = buf_sec;
    U64 min_sec = minsize >> 9, rem_sec = min_sec;
    U64 src_lba = sp.addr.first >> 9;
    U64 dst_lba = dp.addr.first >> 9;

    // Seeking
    if (RET_OK != UtilSeekFile(sh, sp.addr.first, 0)) {
        DUMPERR("Cannot seek src drive"); return;
    }
    if (RET_OK != UtilSeekFile(dh, dp.addr.first, 0)) {
        DUMPERR("Cannot seek dst drive"); return;
    }

    U64 progress = 0, load = 0, thr = rem_sec / 100;

    for (U64 i = 0; rem_sec; i++) {
        wrk_sec = MIN2(wrk_sec, rem_sec);

        // copy from src_lba to dst_lba
        DWORD tmp_src_size = 0, tmp_dst_size = 0, read_size = wrk_sec * 512;

        if (TRUE != ReadFile((HANDLE) sh, sbuf, read_size, &tmp_src_size, NULL)) {
            DUMPERR("");
            cout << "Cannot read src lba " << src_lba << endl; break;
        }

        if (TRUE != ReadFile((HANDLE) dh, dbuf, read_size, &tmp_dst_size, NULL)) {
            DUMPERR("");
            cout << "Cannot read dst lba " << dst_lba << endl; break;
        }

        if (tmp_src_size != tmp_dst_size) {
            cout << "Datasize mismatched: "
                << "src_size(" << tmp_src_size << ")"
                << "dst_size(" << tmp_dst_size << ")"
                << endl; break;
        }

        U32 tmp_size = tmp_src_size;

        if (memcmp(sbuf, dbuf, tmp_size)) {
            cout << "Data mismatched: " << endl;
            cout << "Src data: " << endl
                 << HexFrmt::ToHexString(sbuf, tmp_size) << endl;
            cout << "Dst data: " << endl
                 << HexFrmt::ToHexString(dbuf, tmp_size) << endl;
            break;
        }

        load += wrk_sec;
        if (load > thr) {
            load = 0; progress++;
            cout << "\rComparing: " << progress << "%";
        }

        src_lba += wrk_sec;
        dst_lba += wrk_sec;
        rem_sec -= wrk_sec;
    }

    if (!rem_sec) cout << endl << "Done!!!";
    else cout << endl << "Compare fail at block src_lba " << src_lba << endl;

    Close(sh); Close(dh);
}

int main(int argc, char** argv) {
    (void) argc; (void) argv;

    // TestFileSeek();
    // TestReadDriveSize();
    // ClearPartTable();
    // WriteFullDisk();
    // TestReadSector();
    // TestInitDriveMRB();
    // TestInitDriveGPT();
    // TestGetDriveLayout();
    // TestSetDriveLayoutMRB();
    // TestSetDriveLayoutGPT();
    // TestDiskPart_GenScript();

    // TestDiskClone();

    TestReadEfi();

    return 0;
}
