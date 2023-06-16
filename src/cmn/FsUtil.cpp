
#include "FsUtil.h"
#include "CoreUtil.h"

#include <windows.h>
#include "winioctl.h"

using namespace std;
using namespace StorageApi;
using namespace FsUtil;

#include <cstdio>
#include <string.h>
#include <diskguid.h>
#include <ntdddisk.h>
#include <processthreadsapi.h>

#define DBGMODE 1

static eRetCode GetDriveIndex(const std::string& name, U32& index) {
    const char* p = name.c_str();
    while(*p && !INRANGE('0', '9' + 1, *p)) p++;
    if (!*p) return RET_INVALID_ARG;
    sscanf(p, "%d", &index); return RET_OK;
}

static eRetCode UtilOpenFile(const string& name, HDL& hdl) {
    if (RET_OK != StorageApi::Open(name, hdl)) return RET_FAIL;
    if ((HANDLE) hdl == INVALID_HANDLE_VALUE) return RET_FAIL;
    return RET_OK;
}

// vollist format: "C:\, D:\, E:\"
static eRetCode UtilScanVolList(vector<string>& vollist) {
    vollist.clear();
    const U32 bufsize = 1024;
    char vl[bufsize] = {0};
    DWORD ret = GetLogicalDriveStringsA(bufsize, vl);
    if (!ret) return RET_FAIL;

    for (U32 c = 0, p = 0; c < ret; c++) {
        if (!vl[c]) { p++; continue; }
        if (vl[c] == '\\') {
            string word = string(vl + p, vl + c);
            vollist.push_back(word + "\\");
            p = c + 1;
        }
    }
    return vollist.size() ? RET_OK : RET_EMPTY;
}

static eRetCode UtilGetVolLetter(const string& volume, char& letter) {
    letter = ' '; // Unknown
    const char* p = volume.c_str();
    while(*p) {
        char c = *p++;
        if (INRANGE('a', 'z', c) || INRANGE('A', 'Z', c)) {
            letter = c; return RET_OK;
        }
    }
    return RET_FAIL;
}

static eRetCode UtilGetVolSize(const string& volname, U64& totalsize, U64& usedsize, U64& freesize) {
    ULARGE_INTEGER stotal, sfree;
    BOOL ret = GetDiskFreeSpaceExA(volname.c_str(), NULL, &stotal, &sfree);
    if (TRUE != ret) return RET_FAIL;
    totalsize = stotal.QuadPart;
    freesize = sfree.QuadPart;
    usedsize = totalsize - freesize;
    return RET_OK;
}

static eRetCode UtilGetVolumeName(char letter, wstring& volname, wstring& fsname) {
    wstring shortname = wstring(1, letter) + L":";
    const U32 bufsize = 1024;
    wchar_t volbuf[bufsize] = {0}, fsbuf[bufsize] = {0};
    BOOL ret = GetVolumeInformationW(shortname.c_str(), volbuf, bufsize,
                                     NULL, NULL, NULL, fsbuf, bufsize);
    if (TRUE != ret) return RET_FAIL;
    volname = wstring(volbuf);
    fsname = wstring(fsbuf);
    return RET_OK;
}

// Input volname: C:\, D:\, ...
static eRetCode UtilGetVolInfo(const string& volname, sVolDiskExtInfo& vi) {
    // convert volname to "\\??\\C:"
    string newname = "\\??\\" + volname.substr(0, volname.size() - 1);
    HDL hdl; if (RET_OK != UtilOpenFile(newname, hdl)) return RET_FAIL;
    DWORD rsize, size = sizeof(VOLUME_DISK_EXTENTS) + 256 * sizeof(DISK_EXTENT);
    VOLUME_DISK_EXTENTS* vd = (VOLUME_DISK_EXTENTS*)malloc(size);
    BOOL ret = DeviceIoControl((HANDLE) hdl, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                               NULL, 0, (void *)vd, size, &rsize, NULL);
    if (TRUE != ret) { free(vd); return RET_FAIL; }
    Close(hdl);
    bool volstatus = (RET_OK == UtilGetVolLetter(volname, vi.letter)) &&
                     (RET_OK == UtilGetVolumeName(vi.letter, vi.name, vi.fsname)) &&
                     (RET_OK == UtilGetVolSize(volname, vi.totalsize, vi.usedsize, vi.freesize));
    if (!volstatus) { free(vd); return RET_FAIL; }

    U32 maxi = vd->NumberOfDiskExtents;
    for (U32 i = 0; i < maxi; i++) {
        DISK_EXTENT* de = &vd->Extents[i];
        sDiskExtInfo ext;
        ext.drvidx = de->DiskNumber;
        ext.offset = de->StartingOffset.QuadPart;
        ext.length = de->ExtentLength.QuadPart;
        vi.di.push_back(ext);
    }
    free(vd); return RET_OK;
}

eRetCode FsUtil::ScanVolumeInfo(tVolArray& va) {
    va.clear();
    eRetCode ret; vector<string> vollist;
    ret = UtilScanVolList(vollist);
    if (RET_OK != ret) return ret;
    for(auto name : vollist) {
        sVolDiskExtInfo vi;
        UtilGetVolInfo(name, vi);
        va.push_back(vi);
    }
    return va.size() ? RET_OK : RET_EMPTY;
}

void FsUtil::GetLetterSet(const tVolArray& va, set<char>& used, set<char>& unused) {
    used.clear(); unused.clear();
    for(auto& v : va) used.insert(v.letter);
    for (char c = 'C'; c <= 'Z'; c++)
        if (used.find(c) == used.cend()) unused.insert(c);
}

// Update information in pi:
static void UtilUpdatePartInfo(const tVolArray& va, U32 drvidx, sPartInfo& pi) {
    // Fill partition with volume info

    for(auto& p: pi.parr) {
        U64 minpos = p.addr.first;
        U64 maxpos = minpos + (p.addr.second);

        // search volumes in this range
        bool found = false;
        for (auto& v: va) {
            for(auto& e: v.di) {
                if (e.drvidx != drvidx) continue;

                U64 start = e.offset;
                U64 end = start + e.length;
                if ((start >= minpos) && (end <= maxpos)) {
                    sExtInfo& ext = p.fsinfo;
                    // found volume extent
                    ext.valid = true;
                    ext.letter = v.letter;
                    ext.fsname = v.fsname;
                    ext.name = v.name;
                    ext.freesize = v.freesize;
                    ext.usedsize = v.usedsize;
                    ext.cap = v.totalsize;
                    break;
                }
            }
            if (found) break;
        }
    }
}

eRetCode FsUtil::UpdateVolumeInfo(tDriveArray& da) {
    tVolArray va; ScanVolumeInfo(va);
    for(auto& d:da) {
        U32 drvidx;
        GetDriveIndex(d.name, drvidx);
        UtilUpdatePartInfo(va, drvidx, d.pi);
    }
    return RET_OK;
}

eRetCode FsUtil::TestUpdateVolumeInfo() {
    tDriveArray da; ScanDrive(da);
    UpdateVolumeInfo(da);

    #define DUMP(t, w, v) wcout << t << "(" << setw(w) << (v) << ") "

    // dump partition info
    for(auto& d:da) {
        cout << endl << "Drive: " << d.name;
        for(auto& p : d.pi.parr) {
            wcout << endl << "  ";
            DUMP("Name",  28, p.name);            // Partition name
            DUMP("Index",  1, p.index);            // Partition index
            DUMP("CapMB",  7, p.cap >> 11);              // Capacity in sector
            DUMP("StartSec", 12, p.addr.first);       // Partition address (start lba, sector count)
            DUMP("CountSec", 12, p.addr.second);       // Partition address (start lba, sector count)

            if (p.fsinfo.valid) {
                wcout << endl << "    ";
                DUMP("Letter", 1, p.fsinfo.letter); // drive letter C/D/E/F ..
                DUMP("VlName",10, p.fsinfo.name);   // drive name "My Data"
                DUMP("FsName", 4, p.fsinfo.fsname); // file system name NTFS
                DUMP("Total", 12, p.fsinfo.cap);     //
                DUMP("Used",  12, p.fsinfo.usedsize); // used space in bytes
                DUMP("Free",  12, p.fsinfo.freesize); // free space in bytes
                wcout << endl;
            }
        }
    }
    #undef DUMP
    return RET_OK;
}
