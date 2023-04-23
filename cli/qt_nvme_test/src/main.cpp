#include "StdHeader.h"
#include "DeviceMgr.h"
#include "NvmeUtil.h"
#include<iostream>
#include <stdlib.h>
#include <stddef.h>
#include <tchar.h>
#include <strsafe.h>
#include <io.h>
#include <lmcons.h>
#include <intsafe.h>

#define DRIVENAME "\\\\.\\PhysicalDrive0"


void *calloc_aligned(size_t num, size_t size, size_t alignment)
{
    //call malloc aligned and memset
    void *zeroedMem = NULL;
    size_t numSize = num * size;
    if (numSize)
    {
        zeroedMem = _aligned_malloc(numSize, alignment);

        if (zeroedMem)
        {
            memset(zeroedMem, 0, numSize);
        }
    }
    return zeroedMem;
}

void calloc_free(void * zeroedMem ) {
    _aligned_free(zeroedMem);
}


BOOL QueryPropertyForDevice(
        HANDLE DeviceHandle,
        PULONG AlignmentMask,
        PUCHAR SrbType){
    PSTORAGE_ADAPTER_DESCRIPTOR adapterDescriptor = NULL;
    PSTORAGE_DEVICE_DESCRIPTOR deviceDescriptor = NULL;
    STORAGE_DESCRIPTOR_HEADER header = {0};

    BOOL ok = TRUE;
    BOOL failed = TRUE;
    ULONG i;

    *AlignmentMask = 0; // default to no alignment
    *SrbType = 0; // default to SCSI_REQUEST_BLOCK

    // Loop twice:
    //  First, get size required for storage adapter descriptor
    //  Second, allocate and retrieve storage adapter descriptor
    //  Third, get size required for storage device descriptor
    //  Fourth, allocate and retrieve storage device descriptor
    for (i=0;i<4;i++) {

        PVOID buffer = NULL;
        ULONG bufferSize = 0;
        ULONG returnedData;

        STORAGE_PROPERTY_QUERY query{};

        switch(i) {
        case 0: {
            query.QueryType = PropertyStandardQuery;
            query.PropertyId = StorageAdapterProperty;
            bufferSize = sizeof(STORAGE_DESCRIPTOR_HEADER);
            buffer = &header;
            break;
        }
        case 1: {
            query.QueryType = PropertyStandardQuery;
            query.PropertyId = StorageAdapterProperty;
            bufferSize = header.Size;
            if (bufferSize != 0) {
                adapterDescriptor = (PSTORAGE_ADAPTER_DESCRIPTOR) LocalAlloc(LPTR, bufferSize);
                if (adapterDescriptor == NULL) {
                    goto Cleanup;
                }
            }
            buffer = adapterDescriptor;
            break;
        }
        case 2: {
            query.QueryType = PropertyStandardQuery;
            query.PropertyId = StorageDeviceProperty;
            bufferSize = sizeof(STORAGE_DESCRIPTOR_HEADER);
            buffer = &header;
            break;
        }
        case 3: {
            query.QueryType = PropertyStandardQuery;
            query.PropertyId = StorageDeviceProperty;
            bufferSize = header.Size;

            if (bufferSize != 0) {
                deviceDescriptor = (PSTORAGE_DEVICE_DESCRIPTOR) LocalAlloc(LPTR, bufferSize);
                if (deviceDescriptor == NULL) {
                    goto Cleanup;
                }
            }
            buffer = deviceDescriptor;
            break;
        }
        }

        // buffer can be NULL if the property queried DNE.
        if (buffer != NULL) {
            RtlZeroMemory(buffer, bufferSize);

            // all setup, do the ioctl
            ok = DeviceIoControl(DeviceHandle,
                                 IOCTL_STORAGE_QUERY_PROPERTY,
                                 &query,
                                 sizeof(STORAGE_PROPERTY_QUERY),
                                 buffer,
                                 bufferSize,
                                 &returnedData,
                                 FALSE);
            if (!ok) {
                if (GetLastError() == ERROR_MORE_DATA) {
                    // this is ok, we'll ignore it here
                } else if (GetLastError() == ERROR_INVALID_FUNCTION) {
                    // this is also ok, the property DNE
                } else if (GetLastError() == ERROR_NOT_SUPPORTED) {
                    // this is also ok, the property DNE
                } else {
                    // some unexpected error -- exit out
                    goto Cleanup;
                }
                // zero it out, just in case it was partially filled in.
                RtlZeroMemory(buffer, bufferSize);
            }
        }
    } // end i loop

    // adapterDescriptor is now allocated and full of data.
    // deviceDescriptor is now allocated and full of data.

    if (adapterDescriptor == NULL) {
        printf("   ***** No adapter descriptor supported on the device *****\n");
    } else {
        *AlignmentMask = adapterDescriptor->AlignmentMask;
        *SrbType = adapterDescriptor->SrbType;
    }

    if (deviceDescriptor == NULL) {
        printf("   ***** No device descriptor supported on the device  *****\n");
    } else {
        // PrintDeviceDescriptor(deviceDescriptor);
    }

    failed = FALSE;

Cleanup:
    if (adapterDescriptor != NULL) {
        LocalFree( adapterDescriptor );
    }
    if (deviceDescriptor != NULL) {
        LocalFree( deviceDescriptor );
    }
    return (!failed);

}



void print_Windows_Error_To_Screen(unsigned int windowsError) {
    TCHAR *windowsErrorString = NULL;
    //Originally used: MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)
    //switched to MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US) to keep output consistent with all other verbose output.-TJE
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL, windowsError, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), C_CAST(TCHAR*, &windowsErrorString), 0, NULL);
    //This cast is necessary to tell the Windows API to allocate the string, but it is necessary. Without it, this will not work.
    _tprintf_s(TEXT("%u - %s\n"), windowsError, windowsErrorString);
    LocalFree(windowsErrorString);
}

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



        std::cout << "fromat.FormatApplyToAll:" << (unsigned int)controllerData.FNA.FormatApplyToAll << std::endl;
        std::cout << "fromat.SecureEraseApplyToAll:" << (unsigned int)controllerData.FNA.SecureEraseApplyToAll << std::endl;
        std::cout << "fromat.CryptographicEraseSupported:" << (unsigned int)controllerData.FNA.CryptographicEraseSupported << std::endl;
        std::cout << "fromat.FormatSupportNSIDAllF:" << (unsigned int)controllerData.FNA.FormatSupportNSIDAllF << std::endl;

        std::cout << "SANICAP.CryptoErase:" << (unsigned int)controllerData.SANICAP.CryptoErase << std::endl;
        std::cout << "SANICAP.BlockErase:" << (unsigned int)controllerData.SANICAP.BlockErase << std::endl;
        std::cout << "SANICAP.Overwrite:" << (unsigned int)controllerData.SANICAP.Overwrite << std::endl;
        std::cout << "SANICAP.NDI:" << (unsigned int)controllerData.SANICAP.NDI << std::endl;

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

// NOTE: Not work now - it always print same NSZE of controller identify data
void identifyNSIDList(HANDLE hHandle) {
    std::cout << "\n-----List all NSID listt:------\n";
    NVME_IDENTIFY_ACTIVE_NAMESPACE_LIST nsidList;
    ZeroMemory(&nsidList, sizeof(NVME_IDENTIFY_ACTIVE_NAMESPACE_LIST));
    if(NvmeUtil::IdentifyActiveNSIDList(hHandle, &nsidList)) {

        for (int i = 0; i < 1024; i++) {
            if (nsidList.NSID[i]) {
                printf("%d\n", nsidList.NSID[i]);
            } else {
                break;
            }
        }
        printf("[I] === Active Namespace ID list end ===\n");

    } else {
        std::cout << "===> Fail to read NSID List" << std::endl;
        return;
    }
}

// same identifyNSIDList() - list all active NSIDs via SCSI passthrough
void reportLuns(HANDLE hHandle) {
    uint32_t reportLunsDataSize = 16 + 50*8; // Be carefully with this value ,
                                            // not sure why if we increase or reduce this will break the DeviceIOControl Function
    uint8_t* reportLunsData =  C_CAST(uint8_t*, calloc_aligned(reportLunsDataSize, sizeof(uint8_t), 8)); //malloc(reportLunsDataSize));
    uint32_t dataLength = 40;
    uint8_t* ptrData =  C_CAST(uint8_t*,  calloc_aligned(dataLength, sizeof(uint8_t), 8));

    std::cout << "\n-----List all NSID listt:------\n";

    tScsiContext * scsiContext =  (tScsiContext *) malloc(sizeof(tScsiContext));
    scsiContext->hDevice = hHandle;
    ZeroMemory(ptrData,  dataLength * 4);
    if (!QueryPropertyForDevice(hHandle, &scsiContext->alignmentMask, &scsiContext->srbType)) {
        std::cout << "Failed to QueryPropertyForDevice() \n";
        return;
    }
    std::cout << "alignmentMask:" << (ULONG)scsiContext->alignmentMask << std::endl;
    std::cout << "srbType:" <<  (ULONG)scsiContext->srbType << std::endl;

    if (!NvmeUtil::GetSCSIAAddress(hHandle, &scsiContext->scsiAddr)) {
        std::cout << "Failed to get GetSCSIAAddress" << std::endl;
        return;
    }

    if (TRUE == NvmeUtil::ScsiReportLuns(scsiContext, 0, reportLunsDataSize, reportLunsData)) {
            uint32_t lunListLength = M_BytesTo4ByteValue(reportLunsData[0], reportLunsData[1], reportLunsData[2], reportLunsData[3]);
            for (uint32_t lunOffset = 8, nsidOffset = 0; lunOffset < (8 + lunListLength) && nsidOffset < dataLength && lunOffset < reportLunsDataSize; lunOffset += 8)
            {
                uint64_t lun = M_BytesTo8ByteValue(reportLunsData[lunOffset + 0], reportLunsData[lunOffset + 1], reportLunsData[lunOffset + 2], reportLunsData[lunOffset + 3], reportLunsData[lunOffset + 4], reportLunsData[lunOffset + 5], reportLunsData[lunOffset + 6], reportLunsData[lunOffset + 7]);
                {
                    //nvme uses little endian, so get this correct!
                    ptrData[nsidOffset + 0] = M_Byte((lun + 1), 3);
                    ptrData[nsidOffset + 1] = M_Byte((lun + 1) ,2);
                    ptrData[nsidOffset + 2] = M_Byte((lun + 1) ,1);
                    ptrData[nsidOffset + 3] =M_Byte((lun + 1) ,0);
                    printf("NSID : %d\n", *((uint32_t*) &ptrData[nsidOffset]) + 1);
                    nsidOffset += 4;

                }
            }
    } else {
            std::cout << "===> Fail to read NSID List :";
            print_Windows_Error_To_Screen(scsiContext->lastError);

    }


    calloc_free(reportLunsData);
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

    //identifyNamespace(hHandle);
    //identifyController(hHandle);
    //getHealthInfoLog(hHandle);
    //doSelftest(hHandle);
    //identifyNSIDList(hHandle);

    reportLuns(hHandle);

    if (hHandle != INVALID_HANDLE_VALUE) {
        DeviceMgr::CloseDevice(iHandle);
    }
    return 0;
exit_error:
    std::cout << "Error exit" << std::endl;
    return -1;
}
