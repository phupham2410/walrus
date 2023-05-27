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
    U64 offset;
    U64 length;
};

struct sVolDiskExtInfo {
    string name;
    char letter;
    string fsname;
    U32 count;
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

eRetCode UtilGetVolumeName(char letter, string& volname, string& fsname) {

    string shortname = string(1, letter) + ":";
    char volbuf[MAX_PATH] = {0}, fsbuf[MAX_PATH] = {0};
    BOOL ret = GetVolumeInformationA(shortname.c_str(), volbuf, MAX_PATH,
                                NULL, NULL, NULL, fsbuf, MAX_PATH);
    if (TRUE != ret) {
        DUMPERR("Cannot get free info"); return RET_FAIL;
    }
    volname = string(volbuf); fsname = string(fsbuf);
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
    vi.count = maxi;

    UtilGetVolLetter(volname, vi.letter);
    UtilGetVolumeName(vi.letter, vi.name, vi.fsname);

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

eRetCode UtilScanVolumeInfo() {
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

        // dump:
        cout << "Letter: " << vi.letter << " ";
        cout << "Name: " << vi.name<< " ";
        cout << "FS: " << vi.fsname<< " ";
        for (U32 i = 0; i < vi.count; i++) {
            sDiskExtInfo &di = vi.di[i];
            cout << "Disk: " << di.drvidx << " ";
            cout << "Start: " << (di.offset >> 9) << " (LBA)";
            cout << "Size: " << (di.length >> 30) << " (GB)";
            cout << endl;
        }
    }
    return RET_OK;
}

int main(int argc, char** argv) {
    (void) argc; (void) argv;

    // Util_GetVolumeList();
    UtilScanVolumeInfo();

    return 0;
}
