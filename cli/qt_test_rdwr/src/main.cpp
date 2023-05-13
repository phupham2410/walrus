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

using namespace std;
using namespace StorageApi;

const string PHYDRV1 = "\\\\.\\PhysicalDrive1";
const string PHYDRV2 = "\\\\.\\PhysicalDrive2";

const string drvname = PHYDRV2;

#define DUMPERR(msg) \
    cout << msg << ". " \
         << SystemUtil::GetLastErrorString() << endl

eRetCode UtilOpenFile(const string& name, HDL& hdl) {
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
}

eRetCode UtilInitDrive(HDL hdl) {
    CREATE_DISK dsk;
    dsk.PartitionStyle = PARTITION_STYLE_MBR;
    dsk.Mbr.Signature = 9999;

    BOOL ret; DWORD junk;
    ret = DeviceIoControl((HANDLE) hdl, IOCTL_DISK_CREATE_DISK,
                          &dsk, sizeof(dsk), NULL, 0, &junk, NULL);
    if (!ret) { DUMPERR("Cannot create disk"); return RET_FAIL; }

    ret = DeviceIoControl((HANDLE) hdl, IOCTL_DISK_UPDATE_PROPERTIES,
                              NULL, 0, NULL, 0, &junk, NULL);
    if (!ret) { DUMPERR("Cannot update properties"); return RET_FAIL; }

    return RET_OK;
}

void TestInitDrive() {
    HDL hdl;
    if (RET_OK != UtilOpenFile(drvname, hdl)) return;

    if (RET_OK != UtilInitDrive(hdl)) return;

    cout << "Init drive ok" << endl;
}

#include <diskguid.h>
#include <ntdddisk.h>

eRetCode UtilAddPartition(HDL hdl, U32 itemcnt, U64 itemsize) {
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
        p.Mbr.HiddenSectors = CAP500MB >> 9;
    }

    BOOL ret; DWORD rsize;
    ret = DeviceIoControl((HANDLE) hdl, IOCTL_DISK_SET_DRIVE_LAYOUT_EX,
                          pli, len, NULL, 0, &rsize, NULL);
    free(pli);
    if (!ret) { DUMPERR("Cannot add partitions"); return RET_FAIL; }

    return RET_OK;
}

void TestAddPartition() {
    HDL hdl;
    if (RET_OK != UtilOpenFile(drvname, hdl)) return;

    cout << "Creating partitions ... ";
    if (RET_OK != UtilAddPartition(hdl, 3, CAP030GB)) {
        cout << "FAIL" << endl; return;
    }

    cout << "OK" << endl;
}

int main(int argc, char** argv) {
    (void) argc; (void) argv;

    // TestFileSeek();
    // TestReadDriveSize();
    // ClearPartTable();
    // WriteFullDisk();
    // TestReadSector();
    // TestInitDrive();
    TestAddPartition();
    return 0;
}
