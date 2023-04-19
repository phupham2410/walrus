#include "StdHeader.h"
#include "DeviceMgr.h"
#include "NvmeUtil.h"

#define DRIVENAME "\\\\.\\PhysicalDrive0"
#include<iostream>

void identifyNamespace(HANDLE hHandle) {
    std::cout << "-----Identify Namespace:------\n";
    NVME_IDENTIFY_NAMESPACE_DATA namespaceData;

    if (NvmeUtil::IdentifyNamespace(hHandle, 1, &namespaceData)) {
        std::cout << "NSZE " << namespaceData.NSZE << std::endl;
        std::cout << "NCAP " << namespaceData.NCAP << std::endl;
    } else {
        std::cout << "Failure to read namespace identify" << std::endl;
    }
}

void identifyController(HANDLE hHandle) {
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
}


void getHealthInfoLog(HANDLE hHandle) {
    std::cout << "\n-----Get Health Info:------\n";
    NVME_HEALTH_INFO_LOG healthInfoLog;
    if (NvmeUtil::GetSMARTHealthInfoLog(hHandle, &healthInfoLog)) {
        std::cout << "CriticalWarning.AvailableSpaceLow: " << (uint32_t) healthInfoLog.CriticalWarning.AvailableSpaceLow << std::endl;
        std::cout << "CriticalWarning.TemperatureThreshold: " << (uint32_t) healthInfoLog.CriticalWarning.TemperatureThreshold << std::endl;
        std::cout << "CriticalWarning.ReliabilityDegraded: " << (uint32_t) healthInfoLog.CriticalWarning.ReliabilityDegraded << std::endl;
        std::cout << "CriticalWarning.ReadOnly: " << (uint32_t) healthInfoLog.CriticalWarning.ReadOnly << std::endl;
        std::cout << "CriticalWarning.VolatileMemoryBackupDeviceFailed: " << (uint32_t) healthInfoLog.CriticalWarning.VolatileMemoryBackupDeviceFailed << std::endl;

        std::cout << "AvailableSpare(%): " << (unsigned int)healthInfoLog.AvailableSpare << std::endl;
        std::cout << "AvailableSpareThreshold(%): " << (unsigned int)healthInfoLog.AvailableSpareThreshold << std::endl;
        std::cout << "PercentageUsed(%): " << (unsigned int)healthInfoLog.PercentageUsed << std::endl;
    }
    else {
        std::cout << "Failure to read health log" << std::endl;
    }
}


void doSelftest(HANDLE hHandle) {
    std::cout << "\n-----Do selftest with extended test:------\n";

    // Check self test status first
    NVME_DEVICE_SELF_TEST_LOG selfTestLog;
    std::cout << " - Current status:" << std::endl;
    if(NvmeUtil::GetSelfTestLog(hHandle, &selfTestLog)) {
        std::cout << "CurrentOperation.Status: " << (unsigned int)selfTestLog.CurrentOperation.Status << std::endl;
        std::cout << "CurrentCompletion: " << (unsigned int)selfTestLog.CurrentCompletion.CompletePercent << std::endl;
        std::cout << "Newest.Result[0].Status.Result: " << (unsigned int)selfTestLog.ResultData[0].Status.Result << std::endl;
        std::cout << "Newest.Result[0].Status.CodeValue: " << (unsigned int)selfTestLog.ResultData[0].Status.CodeValue << std::endl;
    } else {
        std::cout << " ===> Fail to read intial selftest log" << std::endl;
        return;
    }

    if (selfTestLog.CurrentOperation.Status !=0) {
        std::cout << "===> Another self-test is in-progress" << std::endl;
        return;
    }

    std::cout << " - Start extended test " << std::endl;
    if(!NvmeUtil::DeviceSelfTest(hHandle, DEVICE_SELFTEST_CODE_EXTENDED_TEST)) {
        std::cout << "===> Fail to start sefl test" << std::endl;
        return;
    }

     std::cout << " - Get new status:" << std::endl;
    if(NvmeUtil::GetSelfTestLog(hHandle, &selfTestLog)) {
        std::cout << "CurrentOperation.Status: " << (unsigned int)selfTestLog.CurrentOperation.Status << std::endl;
         std::cout << "CurrentCompletion: " << (unsigned int)selfTestLog.CurrentCompletion.CompletePercent << std::endl;
        std::cout << "Newest.Result[0].Status.Result: " << (unsigned int)selfTestLog.ResultData[0].Status.Result << std::endl;
        std::cout << "Newest.Result[0].Status.CodeValue: " << (unsigned int)selfTestLog.ResultData[0].Status.CodeValue << std::endl;
    } else {
        std::cout << "===> Fail to read selftest log for re-check" << std::endl;
        return;
    }
}


int main(int argc, char** argv) {
    tHdl iHandle;
    HANDLE hHandle;
    bool status;

    status = DeviceMgr::OpenDevice(DRIVENAME, iHandle);
    if (!status) {
        goto exit_error;
    }

    hHandle = (HANDLE)iHandle;

    identifyNamespace(hHandle);
    identifyController(hHandle);
    getHealthInfoLog(hHandle);
    doSelftest(hHandle);


    if (hHandle != INVALID_HANDLE_VALUE) {
        DeviceMgr::CloseDevice(iHandle);
    }
    return 0;
exit_error:
    std::cout << "Error exit" << std::endl;
    return -1;
}
