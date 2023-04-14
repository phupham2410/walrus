#include "DeviceMgr.h"
#include "windows.h"
#include "ntddscsi.h"

#include "AtaCmd.h"
using namespace std;


bool DeviceMgr::OpenDevice(const string &deviceName, int &handle)
{
    HANDLE handlePtr = CreateFileA(
        deviceName.c_str(),
        GENERIC_READ | GENERIC_WRITE, //IOCTL_ATA_PASS_THROUGH requires read-write
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,            //no security attributes
        OPEN_EXISTING,
        0,               //flags and attributes
        NULL             //no template file
    );

    if (handlePtr == INVALID_HANDLE_VALUE) return false;

    handle = (int)(reinterpret_cast<U64>(handlePtr));
    return true;
}

bool DeviceMgr::OpenDevice(U32 idx, sPHYDRVINFO& drive)
{
    stringstream sstr;
    sstr << "\\\\.\\PhysicalDrive" << idx;
    string driveName = sstr.str();

    int handle;
    bool rc = OpenDevice(driveName, handle);

    if (rc)
    {
        drive.DriveNumber = idx;
        drive.DriveName = driveName;
        drive.DriveHandle = (int) handle;
    }

    return rc;
}

void DeviceMgr::OpenDevice(tPHYDRVARRAY& driveList)
{
    // -------------------------------------------------------------
    // Windows implementation: Try to open all available drives
    // -------------------------------------------------------------

    driveList.clear();
    int maxDriveCount = 16;
    sPHYDRVINFO driveInfo;

    for (int i = 0; i < maxDriveCount; i++)
    {
        if (true == OpenDevice(i, driveInfo))
            driveList.push_back(driveInfo);
    }
}

void DeviceMgr::CloseDevice(int handle)
{
    CloseHandle((HANDLE) handle);
}

bool DeviceMgr::ReadIdentifyData(const string &deviceName, U8 *identifyBuffer)
{
    return ReadIdentifyDataCommon(deviceName, identifyBuffer);
}

eSCANERROR DeviceMgr::ScanDrive(list<sDRVINFO>& infoList, bool readSMART, const tSERIALSET& serialSet)
{
    return ScanDriveCommon(infoList, readSMART, serialSet);
}
