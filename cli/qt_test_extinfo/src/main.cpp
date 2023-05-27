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

const string PHYDRV0 = "\\\\.\\PhysicalDrive0";
const string PHYDRV1 = "\\\\.\\PhysicalDrive1";
const string PHYDRV2 = "\\\\.\\PhysicalDrive2";

const string drvname = PHYDRV2;

#define DUMPERR(msg) \
    cout << msg << ". " \
         << SystemUtil::GetLastErrorString() << endl

eRetCode UtilOpenFile(const string& name, HDL& hdl) {

    cout << "Opening drive " << name << endl;
    // hdl = (HDL) CreateFileA(name.c_str(), FILE_READ_ATTRIBUTES | SYNCHRONIZE | FILE_TRAVERSE,
    //             FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);

    if (RET_OK != StorageApi::Open(name, hdl)) {
        DUMPERR("Cannot open drive"); return RET_FAIL;
    }

    if ((HANDLE) hdl == INVALID_HANDLE_VALUE) {
        DUMPERR("Invalid handle value"); return RET_FAIL;
    }
    return RET_OK;
}

void Util_GetVolumeExtents(const string& volname) {
    HDL hdl;
    if (RET_OK != UtilOpenFile(volname, hdl)) {
        DUMPERR("Cannot open volume"); return;
    }

    DWORD rsize;
    DWORD size = sizeof(VOLUME_DISK_EXTENTS) + 256 * sizeof(DISK_EXTENT);
    VOLUME_DISK_EXTENTS* vde = (VOLUME_DISK_EXTENTS*)malloc(size);
    BOOL ret = DeviceIoControl((HANDLE) hdl, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                               NULL, 0, (void *)vde, size, &rsize, NULL);
    if (TRUE != ret) {
        DUMPERR("Cannot get volume ext info"); return;
    }

    for (U32 i = 0, maxi = vde->NumberOfDiskExtents; i < maxi; i++) {
        DISK_EXTENT* de = &vde->Extents[i];
        U64 lba = de->StartingOffset.QuadPart >> 9;
        U64 sec = de->ExtentLength.QuadPart >> 9;
        cout << "Disk: " << de->DiskNumber << " ";
        cout << "LBA: " << lba << " Size: " << sec << " (sectors)";
        cout << endl;
    }
}

void Util_GetVolumeSpace(const string& volname) {
    // Get free spaces
    ULARGE_INTEGER callercnt, totalcnt, freecnt;
    BOOL ret = GetDiskFreeSpaceExA(volname.c_str(), &callercnt, &totalcnt, &freecnt);
    if (TRUE != ret) {
        DUMPERR("Cannot get free info"); return;
    }

    double total_gb = ((totalcnt.QuadPart * 100) >> 30) / 100.0;
    double used_gb  = (((totalcnt.QuadPart - freecnt.QuadPart) * 100) >> 30) / 100.0;
    double free_gb  = ((freecnt.QuadPart * 100) >> 30) / 100.0;

    cout << "Total: " << total_gb << " (gb)";
    cout << "Used: " << used_gb << " (gb)";
    cout << "Free: " << free_gb << " (gb)";
    cout << endl;
}

void Util_GetVolumeName(const string& volname) {
    // Get free spaces
    ULARGE_INTEGER callercnt, totalcnt, freecnt;

    char volbuf[MAX_PATH] = {0};
    char fsbuf[MAX_PATH] = {0};

    BOOL ret = GetVolumeInformationA(volname.c_str(), volbuf, MAX_PATH,
                                     NULL, NULL, NULL, fsbuf, MAX_PATH);
    if (TRUE != ret) {
        DUMPERR("Cannot get volume & fs name"); return;
    }

    cout << "VolumeName: " << volbuf << " ";
    cout << "FileSystem: " << fsbuf <<  " ";
    cout << endl;
}

void Util_GetVolumeList() {
    // Get free spaces
    char letters[MAX_PATH] = {0};

    DWORD ret = GetLogicalDriveStringsA(MAX_PATH, letters);
    if (!ret) {
        DUMPERR("Cannot get list of drive letters"); return;
    }

    cout << "RetVal: " << ret << endl;

    for (U32 i = 0; i < ret; i++) {
        cout << letters[i];
    }
    cout << endl;
}

void Test_GetVolumeExtents() {
    string volnames[] = {
        "\\??\\C:", "\\??\\D:", "\\??\\E:", "\\??\\H:", "\\??\\V:"
    };

    Util_GetVolumeList();

    for (auto s : volnames) {
        Util_GetVolumeExtents(s);
    }

    Util_GetVolumeSpace("C:");
    Util_GetVolumeName("H:");
}

// -------------------------------------------------------------------
// -------------------------------------------------------------------
// -------------------------------------------------------------------
// -------------------------------------------------------------------

struct sDiskExtInfo {
    U32 drvidx;
    U64 offset; // offset in byte unit
    U64 length; // length in byte unit
};

struct sVolDiskExtInfo {
    char letter;     // D
    wstring name;    // My Storage"
    wstring fsname;  // NTFS
    U64 usedsize;    // size in byte unit
    U64 freesize;    // size in byte unit
    U64 totalsize;   // size in byte unit
    vector<sDiskExtInfo> di;
};

// vollist format: "C:\, D:\, E:\"
eRetCode UtilScanVolList(vector<string>& vollist) {
    vollist.clear();
    char vl[MAX_PATH] = {0};
    DWORD ret = GetLogicalDriveStringsA(MAX_PATH, vl);
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

eRetCode UtilGetVolLetter(const string& volume, char& letter) {
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

eRetCode UtilGetVolSize(const string& volname, U64& totalsize, U64& usedsize, U64& freesize) {
    ULARGE_INTEGER stotal, sfree;
    BOOL ret = GetDiskFreeSpaceExA(volname.c_str(), NULL, &stotal, &sfree);
    if (TRUE != ret) return RET_FAIL;
    totalsize = stotal.QuadPart;
    freesize = sfree.QuadPart;
    usedsize = totalsize - freesize;
    return RET_OK;
}

eRetCode UtilGetVolumeName(char letter, wstring& volname, wstring& fsname) {
    wstring shortname = wstring(1, letter) + L":";
    wchar_t volbuf[MAX_PATH] = {0}, fsbuf[MAX_PATH] = {0};
    BOOL ret = GetVolumeInformationW(shortname.c_str(), volbuf, MAX_PATH,
                                NULL, NULL, NULL, fsbuf, MAX_PATH);
    if (TRUE != ret) {
        DUMPERR("Cannot get name info"); return RET_FAIL;
    }
    volname = wstring(volbuf); fsname = wstring(fsbuf);
    return RET_OK;
}

// Input volname: C:\, D:\, ...
eRetCode UtilGetVolInfo(const string& volname, sVolDiskExtInfo& vi) {
    // convert volname to "\\??\\C:"
    string newname = "\\??\\" + volname.substr(0, volname.size() - 1);

    HDL hdl;
    if (RET_OK != UtilOpenFile(newname, hdl)) return RET_FAIL;

    DWORD rsize;
    DWORD size = sizeof(VOLUME_DISK_EXTENTS) + 256 * sizeof(DISK_EXTENT);
    VOLUME_DISK_EXTENTS* vd = (VOLUME_DISK_EXTENTS*)malloc(size);
    BOOL ret = DeviceIoControl((HANDLE) hdl, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                               NULL, 0, (void *)vd, size, &rsize, NULL);
    if (TRUE != ret) {
        free(vd);
        DUMPERR("Cannot get volume ext info"); return RET_FAIL;
    }

    Close(hdl);

    U32 i, maxi = vd->NumberOfDiskExtents;

    UtilGetVolLetter(volname, vi.letter);
    UtilGetVolumeName(vi.letter, vi.name, vi.fsname);
    UtilGetVolSize(volname, vi.totalsize, vi.usedsize, vi.freesize);

    cout << "Scanning Volume " << newname << " ";
    cout << "Number of Exts: " << maxi << endl;

    for (i = 0; i < maxi; i++) {
        DISK_EXTENT* de = &vd->Extents[i];
        sDiskExtInfo ext;
        ext.drvidx = de->DiskNumber;
        ext.offset = de->StartingOffset.QuadPart;
        ext.length = de->ExtentLength.QuadPart;
        vi.di.push_back(ext);
    }
    return RET_OK;
}

typedef vector<sVolDiskExtInfo> tVolArray;
typedef map<U32, vector<sVolDiskExtInfo>> tVolMap;
typedef tVolMap::iterator tVolMapIter;
typedef tVolMap::const_iterator tVolMapConstIter;

eRetCode UtilScanVolumeInfo(tVolArray& va) {
    va.clear();
    eRetCode ret;
    vector<string> vollist;
    ret = UtilScanVolList(vollist);
    if (RET_OK != ret) {
        DUMPERR("Cannot scan vollist"); return ret; }

    cout << "List of Volumes: " << endl;
    for(auto s : vollist) cout << s << "(" << s.length() << ") "; cout << endl;

    for(auto name : vollist) {
        sVolDiskExtInfo vi;
        UtilGetVolInfo(name, vi);
        va.push_back(vi);

        // dump:
        // cout << "Letter: " << vi.letter << " ";
        // cout << "Name: " << vi.name<< " ";
        // cout << "FS: " << vi.fsname<< " ";
        // for (U32 i = 0, maxi = vi.di.size(); i < maxi; i++) {
        //     sDiskExtInfo &di = vi.di[i];
        //     cout << "Disk: " << di.drvidx << " ";
        //     cout << "Start: " << (di.offset >> 9) << " (LBA)";
        //     cout << "Size: " << (di.length >> 30) << " (GB)";
        //     cout << endl << endl;
        // }
    }
    return va.size() ? RET_OK : RET_EMPTY;
}

// Update information in pi:
void UtilUpdatePartInfo(const vector<sVolDiskExtInfo>& va, U32 drvidx, sPartInfo& pi) {
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

eRetCode GetDriveIndex(const std::string& name, U32& index) {
    const char* p = name.c_str();
    while(*p && !INRANGE('0', '9' + 1, *p)) p++;
    if (!*p) return RET_INVALID_ARG;
    sscanf(p, "%d", &index); return RET_OK;
}

void Test_UtilScanVolumeInfo() {
    tVolArray va;
    UtilScanVolumeInfo(va);
}

#define DUMP(t, w, v) wcout << t << "(" << setw(w) << (v) << ") "

void Test_ScanPartitionExt()  {
    tVolArray va; UtilScanVolumeInfo(va);
    for (auto& v : va) {
        DUMP("Letter", 1, v.letter);    // D
        DUMP("VlName",10, v.name);      // My Storage"
        DUMP("FsName", 4, v.fsname);    // NTFS
        DUMP("Total", 12, v.totalsize);  // size in byte unit
        DUMP("Used",  12, v.usedsize);    // size in byte unit
        DUMP("Free",  12, v.freesize);    // size in byte unit

        wcout << endl;

        for(auto& d: v.di) {
            wcout << "    ";
            DUMP("Drive",      1, d.drvidx);
            DUMP("OffsetSec", 10, d.offset>>9); // offset in byte unit
            DUMP("LengthSec", 10, d.length>>9); // length in byte unit
            wcout << endl;
        }

        wcout << endl;
    }

    tDriveArray da; ScanDrive(da);

    for(auto& d:da) {
        U32 drvidx;
        GetDriveIndex(d.name, drvidx);
        UtilUpdatePartInfo(va, drvidx, d.pi);
    }


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
}

int main(int argc, char** argv) {
    (void) argc; (void) argv;

    // Util_GetVolumeList();
    // Test_UtilScanVolumeInfo();
    Test_ScanPartitionExt();

    return 0;
}
