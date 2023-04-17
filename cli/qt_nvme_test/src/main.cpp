#include "StdHeader.h"
#include "DeviceMgr.h"
#include "NvmeUtil.h"

#define DRIVENAME "\\\\.\\PhysicalDrive1"
#include<iostream>


int main(int argc, char** argv) {
    int iHandle;
    HANDLE hHandle;
    bool status;

    status = DeviceMgr::OpenDevice(DRIVENAME, iHandle);
    if (!status) {
        goto exit_error;
    }

    hHandle = (HANDLE)iHandle;

    std::cout << "-----Identify Namespace:------\n";
    NVME_IDENTIFY_NAMESPACE_DATA namespaceData;
    
    if (NvmeUtil::IdentifyNamespace(hHandle, 1, &namespaceData)) {
        std::cout << "NSZE " << namespaceData.NSZE << std::endl;
        std::cout << "NCAP " << namespaceData.NCAP << std::endl;
    } else {
        std::cout << "Failure to read namespace identify" << std::endl;
    }

    std::cout << "\n-----Identify Controller:------\n";
    NVME_IDENTIFY_CONTROLLER_DATA controllerData;
    if (NvmeUtil::IdentifyController(hHandle, &controllerData)) {
        std::cout << "VIN: " << controllerData.VID << std::endl;
        std::cout << "SSVID: " << controllerData.SSVID << std::endl;
        // These fields are common among revisions...
        {
            char buf[21];
            ZeroMemory(buf, 21);
            strncpy_s(buf, (const char*)(controllerData.SN), 20);
            buf[20] = '\0';
            std::cout << "SN: " << buf << std::endl;
        }

        {
            char buf[41];
            ZeroMemory(buf, 41);
            strncpy_s(buf, (const char*)(controllerData.MN), 40);
            buf[40] = '\0';
            std::cout << "MN: " << buf << std::endl;
        }

        {
            char buf[9];
            ZeroMemory(buf, 9);
            strncpy_s(buf, (const char*)(controllerData.FR), 8);
            buf[8] = '\0';
            std::cout << "FW Revision:" << buf << std::endl;
        }
    }
    else {
        std::cout << "Failure to read controller identify" << std::endl;
    }

    std::cout << "\n-----Get Health Info:------\n";
    NVME_HEALTH_INFO_LOG healthInfoLog;
    if (NvmeUtil::GetSMARTHealthInfoLog(hHandle, &healthInfoLog)) {
        std::cout << "CriticalWarning.AvailableSpaceLow: " << healthInfoLog.CriticalWarning.AvailableSpaceLow << std::endl;
        std::cout << "CriticalWarning.TemperatureThreshold: " << healthInfoLog.CriticalWarning.TemperatureThreshold << std::endl;
        std::cout << "CriticalWarning.ReliabilityDegraded: " << healthInfoLog.CriticalWarning.ReliabilityDegraded << std::endl;
        std::cout << "CriticalWarning.ReadOnly: " << healthInfoLog.CriticalWarning.ReadOnly << std::endl;
        std::cout << "CriticalWarning.VolatileMemoryBackupDeviceFailed: " << healthInfoLog.CriticalWarning.VolatileMemoryBackupDeviceFailed << std::endl;

        std::cout << "AvailableSpare(%): " << (unsigned int)healthInfoLog.AvailableSpare << std::endl;
        std::cout << "AvailableSpareThreshold(%): " << (unsigned int)healthInfoLog.AvailableSpareThreshold << std::endl;
        std::cout << "PercentageUsed(%): " << (unsigned int)healthInfoLog.PercentageUsed << std::endl;
    }
    else {
        std::cout << "Failure to read health log" << std::endl;
    }

    if (hHandle != INVALID_HANDLE_VALUE) {
        DeviceMgr::CloseDevice(iHandle);
    }
    return 0;
exit_error:
    return -1;
}
