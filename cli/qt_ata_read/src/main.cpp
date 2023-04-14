#include "StdHeader.h"
#include "DeviceMgr.h"

#include "AtaCmd.h"
#include "HexFrmt.h"
#include <windows.h>

#define DRIVENAME "\\\\.\\PhysicalDrive0"
#define CMDTYPE AtaCmd
#define CMDCODE_READ CMD_READ_DMA
#define CMDCODE_WRITE CMD_WRITE_DMA
#define CMDCODE_IDENTIFY CMD_IDENTIFY_DEVICE
#define CMDCODE_SMARTDATA CMD_SMART_READ_DATA
#define CMDCODE_THRESHOLD CMD_SMART_READ_THRESHOLD

using namespace std;

#include "StorageApi.h"
void testStorageApi_ScanDrive() {
    StorageApi::tDriveArray drvlst;
    StorageApi::eRetCode ret = StorageApi::ScanDrive(drvlst, true, true);
    if (ret == StorageApi::RET_OK) {
        for (U32 i = 0; i < drvlst.size(); i++) {
            StorageApi::sDriveInfo& drv = drvlst[i];
            cout << StorageApi::ToString(drv) << endl << endl;
        }
    }
    else {
        cout << "ScanDrive error: " << StorageApi::ToString(ret) << endl;
    }
}

void handleCommandSet03_ReadIdentify()
{
    stringstream sstr;
    string driveName = DRIVENAME;
    sstr << "## Read Identify on drive " << driveName << endl;

    int handle;
    bool status;

    do {
        status = DeviceMgr::OpenDevice(driveName.c_str(), handle);
        if (false == status)
        {
            sstr << "Cannot open device handle." << endl;
            break;
        }

        CMDTYPE cmd;
        cmd.setCommand(0, 1, CMDCODE_IDENTIFY);
        status = cmd.executeCommand(handle);

        if (false == status)
        {
            int errorCode = GetLastError();

            sstr << "Cannot execute ioctl command " << cmd.toString() << endl;
            sstr << "Error code: " << errorCode << endl;
            break;
        }
        else
        {
            U8* dataBuffer = cmd.getBufferPtr();
            U32 dataSize = cmd.SecCount * 512;

            sstr << "Identify sector: " << endl;
            sstr << HexFrmt::ToHexString(dataBuffer, dataSize);

            // Parse Identify info
            sIDENTIFY idInfo;
            DeviceMgr::ParseIdentifyInfo(dataBuffer, driveName, idInfo);
            sstr << "Identify info: " << endl << idInfo.toString() << endl;
            sstr << "Features: " << endl << idInfo.SectorInfo.toString() << endl;
        }

    } while(0);

    DeviceMgr::CloseDevice(handle);

    cout << sstr.str();
}

void handleCommandSet03_ReadSMART()
{
    stringstream sstr;

    string driveName = DRIVENAME;
    sstr << "## Read SMART info on drive " << driveName << endl;

    int handle;
    bool status;

    do {
        status = DeviceMgr::OpenDevice(driveName.c_str(), handle);
        if (false == status)
        {
            sstr << "Cannot open device handle." << endl;
            break;
        }

        CMDTYPE cmd0;
        cmd0.setCommand(0, 1, CMDCODE_SMARTDATA);
        status = cmd0.executeCommand(handle);

        if (false == status)
        {
            int errorCode = GetLastError();

            sstr << "Cannot execute ioctl command " << cmd0.toString() << endl;
            sstr << "Error code: " << errorCode << endl;
            break;
        }

        CMDTYPE cmd1;
        cmd1.setCommand(0, 1, CMDCODE_THRESHOLD);
        status = cmd1.executeCommand(handle);

        if (false == status)
        {
            int errorCode = GetLastError();

            sstr << "Cannot execute ioctl command " << cmd1.toString() << endl;
            sstr << "Error code: " << errorCode << endl;
            break;
        }

        if(1) {
            // dump smart information
            U8* dataPtr = cmd0.getBufferPtr();
            U8* thrsPtr = cmd1.getBufferPtr();

            sstr << "SMART Data sector" << endl << HexFrmt::ToHexString(dataPtr, 512);
            sstr << "SMART Threshold sector" << endl << HexFrmt::ToHexString(thrsPtr, 512);

            sSMARTINFO smartInfo;
            DeviceMgr::ParseSmartData(dataPtr, thrsPtr, smartInfo);

            sstr << "List of SMART attribute: " << endl << ::ToVerboseString(smartInfo) << endl;
        }
    } while(0);

    DeviceMgr::CloseDevice(handle);

    cout << sstr.str();
}

void handleCommandSet03_Read48b()
{
    stringstream sstr;

    string driveName = DRIVENAME;
    sstr << "## Read sectors on drive " << driveName << endl;

    int handle;
    bool status;

    do {
        status = DeviceMgr::OpenDevice(driveName.c_str(), handle);
        if (false == status)
        {
            sstr << "Cannot open device handle." << endl;
            break;
        }

        U32 maxc = 4;
        U64 GB = 1024*1024*1024 / 512;

        U64 start = 513 * GB;
        // U64 start = 128*1024*1024/512;
        for(U64 i = start, c = 0; c < maxc; c++, i++) {
            CMDTYPE cmd;
            cmd.setCommand(i, 1, CMD_READ_DMA_48B);
            status = cmd.executeCommand(handle);

            if (false == status)
            {
                int errorCode = GetLastError();

                sstr << "Cannot execute ioctl command " << cmd.toString() << endl;
                sstr << "Error code: " << errorCode << endl;
                break;
            }

            U8* dataBuffer = cmd.getBufferPtr();
            U32 dataSize = cmd.SecCount * 512;

            sstr << "Data at sector: " << i << endl;
            sstr << HexFrmt::ToHexString(dataBuffer, dataSize/2);
        }
    } while(0);

    DeviceMgr::CloseDevice(handle);

    cout << sstr.str();
}

int main(int argc, char** argv) {
    testStorageApi_ScanDrive();
    // handleCommandSet03_Read48b();
    // handleCommandSet03_ReadIdentify();
    // handleCommandSet03_ReadSMART();
    cout << "Hello SSD" << endl;
    return 0;
}
