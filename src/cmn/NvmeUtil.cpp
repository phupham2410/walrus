#include<stdlib.h>
#include "NvmeUtil.h"

using namespace std;

BOOL NvmeUtil::IdentifyNamespace(HANDLE hDevice, DWORD dwNSID, PNVME_IDENTIFY_NAMESPACE_DATA pNamespaceIdentify) {
    BOOL    result = FALSE;
    PVOID   buffer = NULL;
    ULONG   bufferLength = 0;
    ULONG   returnedLength = 0;

    PSTORAGE_PROPERTY_QUERY query = NULL;
    PSTORAGE_PROTOCOL_SPECIFIC_DATA protocolData = NULL;
    PSTORAGE_PROTOCOL_DATA_DESCRIPTOR protocolDataDescr = NULL;

    if (NULL == pNamespaceIdentify) {
        printf("IdentifyNamespace: invalid input.\n");
        goto exit;
    }

    //
    // Allocate buffer for use.
    //
    bufferLength = FIELD_OFFSET(STORAGE_PROPERTY_QUERY, AdditionalParameters) 
        + sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA) 
        + sizeof(NVME_IDENTIFY_NAMESPACE_DATA);
    buffer = malloc(bufferLength);

    if (buffer == NULL) {
        printf("IdentifyNamespace: allocate buffer failed.\n");
        goto exit;
    }

    //
    // Initialize query data structure to get Identify Controller Data.
    //
    ZeroMemory(buffer, bufferLength);

    query = (PSTORAGE_PROPERTY_QUERY)buffer;
    protocolDataDescr = (PSTORAGE_PROTOCOL_DATA_DESCRIPTOR)buffer;
    protocolData = (PSTORAGE_PROTOCOL_SPECIFIC_DATA)query->AdditionalParameters;

    query->PropertyId = StorageAdapterProtocolSpecificProperty;
    query->QueryType = PropertyStandardQuery;

    protocolData->ProtocolType = ProtocolTypeNvme;
    protocolData->DataType = NVMeDataTypeIdentify;
    protocolData->ProtocolDataRequestValue = NVME_IDENTIFY_CNS_SPECIFIC_NAMESPACE;
    protocolData->ProtocolDataRequestSubValue = dwNSID;
    protocolData->ProtocolDataOffset = sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA);
    protocolData->ProtocolDataLength = sizeof(NVME_IDENTIFY_NAMESPACE_DATA);

    //
    // Send request down.
    //
    result = DeviceIoControl(hDevice,
                             IOCTL_STORAGE_QUERY_PROPERTY,
                             buffer,
                             bufferLength,
                             buffer,
                             bufferLength,
                             &returnedLength,
                             NULL
                             );

    if (!result) {
        goto exit;
    }
    // Validate the returned data.
    if ((protocolDataDescr->Version != sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR)) ||
        (protocolDataDescr->Size != sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR)))
    {
        printf("IdentifyNamespace: Data Descriptor Header is not valid.\n");
        result = FALSE;
        goto exit;
    }

    if ((protocolData->ProtocolDataOffset > sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA)) ||
        (protocolData->ProtocolDataLength < sizeof(NVME_IDENTIFY_NAMESPACE_DATA)))
    {
        printf("IdentifyController: ProtocolData Offset/Length is not valid.\n");
        result = FALSE;
        goto exit;
    }

    //
    // Identify Namespace Data 
    //
    memcpy(pNamespaceIdentify, (PCHAR)protocolData + protocolData->ProtocolDataOffset, sizeof(NVME_IDENTIFY_NAMESPACE_DATA));

    if (pNamespaceIdentify->NSZE == 0) {
        printf("IdentifyNamespace: Identify Namespace Data not valid.\n");
        result = FALSE;
        goto exit;
    }
    
    result = TRUE;
exit:
    if (buffer != NULL) {
        free(buffer);
    }
    return result;
}

BOOL NvmeUtil::IdentifyController(HANDLE hDevice, PNVME_IDENTIFY_CONTROLLER_DATA pControllerIdentify) {
    BOOL    result = FALSE;
    PVOID   buffer = NULL;
    ULONG   bufferLength = 0;
    ULONG   returnedLength = 0;

    PSTORAGE_PROPERTY_QUERY query = NULL;
    PSTORAGE_PROTOCOL_SPECIFIC_DATA protocolData = NULL;
    PSTORAGE_PROTOCOL_DATA_DESCRIPTOR protocolDataDescr = NULL;

    if (NULL == pControllerIdentify) {
        printf("IdentifyController: invalid input.\n");
        goto exit;
    }

    // Allocate buffer for use.
    bufferLength = offsetof(STORAGE_PROPERTY_QUERY, AdditionalParameters)
        + sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA)
        + sizeof(NVME_IDENTIFY_CONTROLLER_DATA);
    buffer = malloc(bufferLength);

    if (buffer == NULL) {
        printf("IdentifyController: allocate buffer failed.\n");
        goto exit;
    }

    //
    // Initialize query data structure to get Identify Controller Data.
    //
    ZeroMemory(buffer, bufferLength);

    query = (PSTORAGE_PROPERTY_QUERY)buffer;
    protocolDataDescr = (PSTORAGE_PROTOCOL_DATA_DESCRIPTOR)buffer;
    protocolData = (PSTORAGE_PROTOCOL_SPECIFIC_DATA)query->AdditionalParameters;

    query->PropertyId = StorageAdapterProtocolSpecificProperty;
    query->QueryType = PropertyStandardQuery;

    protocolData->ProtocolType = ProtocolTypeNvme;
    protocolData->DataType = NVMeDataTypeIdentify;
    protocolData->ProtocolDataRequestValue = NVME_IDENTIFY_CNS_CONTROLLER;
    protocolData->ProtocolDataRequestSubValue = 0;
    protocolData->ProtocolDataOffset = sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA);
    protocolData->ProtocolDataLength = sizeof(NVME_IDENTIFY_CONTROLLER_DATA);

    //
    // Send request down.
    //
    result = DeviceIoControl(
        hDevice,
        IOCTL_STORAGE_QUERY_PROPERTY,
        buffer,
        bufferLength,
        buffer,
        bufferLength,
        &returnedLength,
        NULL
    );

    if (!result) goto exit;


    // Validate the returned data.
    if ((protocolDataDescr->Version != sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR)) ||
        (protocolDataDescr->Size != sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR)))
    {
        printf("IdentifyController: Data Descriptor Header is not valid\n");
        result = FALSE;
        goto exit;
    }

    protocolData = &protocolDataDescr->ProtocolSpecificData;

    if ((protocolData->ProtocolDataOffset > sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA)) ||
        (protocolData->ProtocolDataLength < sizeof(NVME_IDENTIFY_CONTROLLER_DATA)))
    {
        printf("IdentifyController: ProtocolData Offset/Length is not valid.\n");
        result = FALSE;
        goto exit;
    }

    memcpy(pControllerIdentify, (PCHAR)protocolData + protocolData->ProtocolDataOffset, sizeof(NVME_IDENTIFY_CONTROLLER_DATA));
    if ((pControllerIdentify->VID == 0) || (pControllerIdentify->NN == 0))
    {
        printf("IdentifyController: Identify Controller Data is not valid.\n");
        result = FALSE;
        goto exit;
    }

    result = TRUE;

exit:

    if (buffer != NULL)
    {
        free(buffer);
    }

    return result;
}

BOOL NvmeUtil::GetHealthInfoLog(HANDLE hDevice, DWORD dwNSID, PNVME_HEALTH_INFO_LOG hlog){
    BOOL    result = FALSE;
    PVOID   buffer = NULL;
    ULONG   bufsize = 0;
    ULONG   retsize = 0;

    PSTORAGE_PROPERTY_QUERY query = NULL;
    PSTORAGE_PROTOCOL_SPECIFIC_DATA pdata = NULL;
    PSTORAGE_PROTOCOL_DATA_DESCRIPTOR pdesc = NULL;

    if (NULL == hlog) goto exit;

    // Allocate buffer for use.
    bufsize = offsetof(STORAGE_PROPERTY_QUERY, AdditionalParameters) +
              sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA) +
              sizeof(NVME_HEALTH_INFO_LOG);
    buffer = malloc(bufsize);

    if (buffer == NULL) goto exit;
    ZeroMemory(buffer, bufsize);

    query = (PSTORAGE_PROPERTY_QUERY)buffer;
    pdesc = (PSTORAGE_PROTOCOL_DATA_DESCRIPTOR)buffer;
    pdata = (PSTORAGE_PROTOCOL_SPECIFIC_DATA)query->AdditionalParameters;

    query->PropertyId = StorageDeviceProtocolSpecificProperty;
    query->QueryType = PropertyStandardQuery;

    pdata->ProtocolType = ProtocolTypeNvme;
    pdata->DataType = NVMeDataTypeLogPage;
    pdata->ProtocolDataRequestValue = NVME_LOG_PAGE_HEALTH_INFO;
    pdata->ProtocolDataRequestSubValue = dwNSID;
    pdata->ProtocolDataOffset = sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA);
    pdata->ProtocolDataLength = sizeof(NVME_HEALTH_INFO_LOG);

    // Send request down.
    result = DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
                             buffer, bufsize, buffer, bufsize, &retsize, NULL);

    if (!result) goto exit;

    // Validate the returned data.
    if ((pdesc->Version != sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR)) ||
        (pdesc->Size != sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR))) {
        result = FALSE; goto exit;
    }

    pdata = &pdesc->ProtocolSpecificData;

    if ((pdata->ProtocolDataOffset < sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA)) ||
        (pdata->ProtocolDataLength < sizeof(NVME_HEALTH_INFO_LOG))) {
        result = FALSE; goto exit;
    }

    memcpy(hlog, (PCHAR)pdata + pdata->ProtocolDataOffset, sizeof(NVME_HEALTH_INFO_LOG));

    result = TRUE;
exit:
    if (buffer != NULL) free(buffer);
    return result;
}

BOOL NvmeUtil::GetHealthInfoLog(HANDLE hDevice, PNVME_HEALTH_INFO_LOG pHealthInfoLog){
    return GetHealthInfoLog(hDevice, NVME_NAMESPACE_ALL, pHealthInfoLog);
}


BOOL NvmeUtil::win10FW_GetfwdlInfoQuery(HANDLE hHandle,PSTORAGE_HW_FIRMWARE_INFO fwdlInfo,  BOOL isNvme, PBYTE pFwupdateSlotId) {
    BOOL ret = FALSE;
    STORAGE_HW_FIRMWARE_INFO_QUERY fwdlInfoQuery;
    memset(&fwdlInfoQuery, 0, sizeof(STORAGE_HW_FIRMWARE_INFO_QUERY));
    fwdlInfoQuery.Version = sizeof(STORAGE_HW_FIRMWARE_INFO_QUERY);
    fwdlInfoQuery.Size = sizeof(STORAGE_HW_FIRMWARE_INFO_QUERY);
    uint8_t slotCount = 7;//7 is maximum number of firmware slots...always reading with this for now since it doesn't hurt sas/sata drives. - TJE
    uint32_t outputDataSize = sizeof(STORAGE_HW_FIRMWARE_INFO) + (sizeof(STORAGE_HW_FIRMWARE_SLOT_INFO) * slotCount);
    uint8_t *outputData = C_CAST(uint8_t*, malloc(outputDataSize));
    if (!outputData)
    {
        printf("Allocate buffer for read firmware info failed. Error code: %d\n", GetLastError());
        return FALSE;
    }
    memset(outputData, 0, outputDataSize);
    DWORD returned_data = 0;

    if (isNvme) {
        fwdlInfoQuery.Flags |= STORAGE_HW_FIRMWARE_REQUEST_FLAG_CONTROLLER;
    }

    int fwdlRet = DeviceIoControl(hHandle,
                                  IOCTL_STORAGE_FIRMWARE_GET_INFO,
                                  &fwdlInfoQuery,
                                  sizeof(STORAGE_HW_FIRMWARE_INFO_QUERY),
                                  outputData,
                                  outputDataSize,
                                  &returned_data,
                                  NULL);
    //Got the version info, but that doesn't mean we'll be successful with commands...
    if (fwdlRet)
    {
        PSTORAGE_HW_FIRMWARE_INFO fwdlSupportedInfo = C_CAST(PSTORAGE_HW_FIRMWARE_INFO, outputData);

        printf("Got Win10 FWDL Info\n");
        printf("\tSupported: %d\n", fwdlSupportedInfo->SupportUpgrade);
        printf("\tPayload Alignment: %ld\n", fwdlSupportedInfo->ImagePayloadAlignment);
        printf("\tmaxXferSize: %ld\n", fwdlSupportedInfo->ImagePayloadMaxSize);
        printf("\tPendingActivate: %d\n", fwdlSupportedInfo->PendingActivateSlot);
        printf("\tActiveSlot: %d\n", fwdlSupportedInfo->ActiveSlot);
        printf("\tSlot Count: %d\n", fwdlSupportedInfo->SlotCount);
        printf("\tFirmware Shared: %d\n", fwdlSupportedInfo->FirmwareShared);
        //print out what's in the slots!
        for (uint8_t iter = 0; iter < fwdlSupportedInfo->SlotCount && iter < slotCount; ++iter) {
            printf("\t    Firmware Slot %d:\n", fwdlSupportedInfo->Slot[iter].SlotNumber);
            printf("\t\tRead Only: %d\n", fwdlSupportedInfo->Slot[iter].ReadOnly);
            printf("\t\tRevision: %s\n", fwdlSupportedInfo->Slot[iter].Revision);
        }

        *pFwupdateSlotId = 255;
        // Search first slot for updating firmware
        for (uint8_t iter = 0; iter < fwdlSupportedInfo->SlotCount && iter < slotCount; ++iter) {
            if (fwdlSupportedInfo->Slot[iter].ReadOnly == 0) {
                *pFwupdateSlotId = fwdlSupportedInfo->Slot[iter].SlotNumber;
                break;
            }
        }

        // if no valid slot
        if (*pFwupdateSlotId == 255) {
            printf("Novalid slot to do firmware update\n");
            ret = FALSE;
        } else {
            printf("Slot to do firmware update is %d\n", *pFwupdateSlotId);
            memcpy(fwdlInfo, fwdlSupportedInfo, sizeof(STORAGE_HW_FIRMWARE_INFO));
            ret = TRUE;
        }
    }
    else
    {
        //DWORD lastError = GetLastError();
        ret = FALSE;
    }
    safe_Free(outputData)
        return ret;
}


static BOOL readFile(HANDLE fileHandle,PUCHAR buffer, ULONG bufferLength, PULONG readLength) {
    BOOL   result = TRUE;
    ULONG currentReadOffset = 0;
    ULONG tmpReadLength = 0;

    RtlZeroMemory(buffer, bufferLength);
    *readLength = 0;
    while(currentReadOffset < bufferLength) {
        result = ReadFile(fileHandle, &buffer[currentReadOffset], bufferLength - currentReadOffset, &tmpReadLength, NULL);
        if (result == FALSE) {
            return FALSE;
        }

        if (tmpReadLength == 0) {
            break;
        }

        currentReadOffset += tmpReadLength;
    }

    *readLength = currentReadOffset;
    return TRUE;
}

BOOL NvmeUtil::win10FW_TransferFile(HANDLE hHandle, PSTORAGE_HW_FIRMWARE_INFO fwdlInfo, BYTE slotId, LPCSTR fwFileName, int64_t transize) {
    BOOL                    result = TRUE;

    PUCHAR                  buffer = NULL;
    ULONG                   bufferSize;
    ULONG                   imageBufferLength;

    PSTORAGE_HW_FIRMWARE_DOWNLOAD   firmwareDownload = NULL;
    ULONG                   returnedLength;


    HANDLE                  fileHandle = NULL;
    ULONG                   imageOffset;
    ULONG                   readLength;
    BOOLEAN                 moreToDownload;
    DWORD fileSize;
    BOOL firstFrame = TRUE;
    BOOL lastFrame = FALSE;


    /* The Max Transfer Length limits the part of buffer that may need to transfer to controller, not the whole buffer.*/
    bufferSize = FIELD_OFFSET(STORAGE_HW_FIRMWARE_DOWNLOAD, ImageBuffer);
    bufferSize += fwdlInfo->ImagePayloadMaxSize;

    buffer = (PUCHAR)malloc(bufferSize);
    if (buffer == NULL) {
        printf("Allocate buffer for sending firmware image file failed. Error code: %d\n", GetLastError());
        return FALSE;
    }

    /* Setup header of firmware download data structure.*/
    RtlZeroMemory(buffer, bufferSize);

    firmwareDownload = (PSTORAGE_HW_FIRMWARE_DOWNLOAD)buffer;
    firmwareDownload->Version = sizeof(STORAGE_HW_FIRMWARE_DOWNLOAD);
    firmwareDownload->Size = bufferSize;

    if (fwdlInfo->FirmwareShared) {
        firmwareDownload->Flags = STORAGE_HW_FIRMWARE_REQUEST_FLAG_CONTROLLER;
    } else {
        firmwareDownload->Flags = 0;
    }

    firmwareDownload->Slot = slotId;

    /* Open image file and download it to controller.*/
    //imageBufferLength = bufferSize - FIELD_OFFSET(STORAGE_HW_FIRMWARE_DOWNLOAD, ImageBuffer);
    imageBufferLength = M_Min(bufferSize - FIELD_OFFSET(STORAGE_HW_FIRMWARE_DOWNLOAD, ImageBuffer),static_cast<ULONG>(transize*1024));


    imageOffset = 0;
    readLength = 0;
    moreToDownload = TRUE;


    fileHandle = CreateFile(fwFileName,   // file to open
                            GENERIC_READ,          // open for reading
                            FILE_SHARE_READ,       // share for reading
                            NULL,                  // default security
                            OPEN_EXISTING,         // existing file only
                            FILE_ATTRIBUTE_NORMAL, // normal file
                            NULL);                 // no attr. template

    if (fileHandle == INVALID_HANDLE_VALUE) {
        printf("Unable to open handle for firmware image file errocode %d\n", GetLastError());
        if (buffer != NULL)
            free(buffer);
        return FALSE;
    }

    fileSize = GetFileSize(fileHandle, NULL);
    if (fileSize == INVALID_FILE_SIZE) {
        printf("Unable to read file size: %d\n", GetLastError());
        if (buffer != NULL)
            free(buffer);
        return FALSE;
    }

    while (moreToDownload) {
        result = readFile(fileHandle, firmwareDownload->ImageBuffer, imageBufferLength, &readLength);
        if(result == FALSE) {
            printf("Read firmware file failed. Error code: %d\n", GetLastError());

            if (fileHandle != NULL) {
                CloseHandle(fileHandle);
            }

            if (buffer != NULL) {
                free(buffer);
            }

            return FALSE;
        }

        if (readLength == 0) {
            if (imageOffset == 0) {
                printf("Firmware image file read return length value 0. Error code: %d\n", GetLastError());
                if (fileHandle != NULL) {
                    CloseHandle(fileHandle);
                }

                if (buffer != NULL) {
                    free(buffer);
                }

                return FALSE;
            }

            moreToDownload = FALSE;
            break;
        }

        firmwareDownload->Offset = imageOffset;
        if (firstFrame) {
            firstFrame = FALSE;
            firmwareDownload->Flags |= STORAGE_HW_FIRMWARE_REQUEST_FLAG_FIRST_SEGMENT;
        } else {
            firmwareDownload->Flags ^= STORAGE_HW_FIRMWARE_REQUEST_FLAG_FIRST_SEGMENT;
        }

        if (readLength > 0) {
            firmwareDownload->BufferSize = ((readLength - 1) / fwdlInfo->ImagePayloadAlignment + 1) * fwdlInfo->ImagePayloadAlignment;
        } else {
            firmwareDownload->BufferSize = 0;
        }

        if  (imageOffset + (ULONG)firmwareDownload->BufferSize >= fileSize) {
            // This is last segment
            firmwareDownload->Flags |= STORAGE_HW_FIRMWARE_REQUEST_FLAG_LAST_SEGMENT;
        }

        /* download this piece of firmware to device */
        result = DeviceIoControl(hHandle,
                                 IOCTL_STORAGE_FIRMWARE_DOWNLOAD,
                                 buffer,
                                 bufferSize,
                                 buffer,
                                 bufferSize,
                                 &returnedLength,
                                 NULL
                                 );

        if (result == FALSE) {
            printf(("\t DeviceStorageFirmwareUpgrade - firmware download IOCTL failed. Error code: %d\n"), GetLastError());
            if (fileHandle != NULL) {
                CloseHandle(fileHandle);
            }

            if (buffer != NULL) {
                free(buffer);
            }

            return FALSE;
        }

        /* Update Image Offset for next loop.*/
        imageOffset += (ULONG)firmwareDownload->BufferSize;
    }
    return result;
}

BOOL NvmeUtil::win10FW_Active(HANDLE hHandle, PSTORAGE_HW_FIRMWARE_INFO fwdlInfo, BYTE slotId){
    BOOL ret = FALSE;
    PSTORAGE_HW_FIRMWARE_ACTIVATE   firmwareActivate = NULL;
    PUCHAR buffer;
    ULONGLONG  tickCountStart = 0;
    ULONGLONG  tickCountEnd = 0;
    ULONG returnedLength = 0;

    buffer = (PUCHAR)malloc(sizeof(STORAGE_HW_FIRMWARE_ACTIVATE));
    if (buffer == NULL) {
        return FALSE;
    }

    memset(buffer, 0, sizeof(STORAGE_HW_FIRMWARE_ACTIVATE));

    firmwareActivate = (PSTORAGE_HW_FIRMWARE_ACTIVATE) buffer;
    firmwareActivate->Version = sizeof(STORAGE_HW_FIRMWARE_ACTIVATE);
    firmwareActivate->Size = sizeof(STORAGE_HW_FIRMWARE_ACTIVATE);
    firmwareActivate->Slot = slotId;

    if (fwdlInfo->FirmwareShared) {
        firmwareActivate->Flags = STORAGE_HW_FIRMWARE_REQUEST_FLAG_CONTROLLER;
    }


    tickCountStart = GetTickCount();

    /* activate firmware */
    ret = DeviceIoControl(hHandle,
                          IOCTL_STORAGE_FIRMWARE_ACTIVATE,
                          buffer,
                          sizeof(STORAGE_HW_FIRMWARE_ACTIVATE),
                          buffer,
                          sizeof(STORAGE_HW_FIRMWARE_ACTIVATE),
                          &returnedLength,
                          NULL);

    tickCountEnd = GetTickCount();


    ULONG   seconds = (ULONG)((tickCountEnd - tickCountStart) / 1000);
    ULONG   milliseconds = (ULONG)((tickCountEnd - tickCountStart) % 1000);
    printf( ("\n\tFirmware activation process took %d.%d seconds.\n"), seconds, milliseconds);


    if (ret) {
        printf( ("\tNew firmware has been successfully applied to device.\n"));
    } else if (GetLastError() == STG_S_POWER_CYCLE_REQUIRED) {
        printf( ("\tWarning: Upgrade completed. Power cycle is required to activate the new firmware.\n"));
    } else {
        if (buffer != NULL) {
            free(buffer);
        }
        return FALSE;
    }

    if (buffer != NULL) {
        free(buffer);
    }

    return TRUE;
}


BOOL NvmeUtil::GetSelfTestLog(HANDLE hdl, PNVME_DEVICE_SELF_TEST_LOG stl) {

    BOOL result = FALSE;
    PVOID bufptr = NULL;
    ULONG bufsize = 0;
    ULONG retlen = 0;

    do {
        if (NULL == stl) break;

        // Allocate buffer for use.
        bufsize = offsetof(STORAGE_PROPERTY_QUERY, AdditionalParameters)
                  + sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA)
                  + sizeof(NVME_DEVICE_SELF_TEST_LOG);
        bufptr = malloc(bufsize); if (bufptr == NULL) break;
        memset (bufptr, 0x00, bufsize);

        STORAGE_PROPERTY_QUERY* query = (STORAGE_PROPERTY_QUERY*)bufptr;
        STORAGE_PROTOCOL_DATA_DESCRIPTOR* pdesc = (STORAGE_PROTOCOL_DATA_DESCRIPTOR*)bufptr;
        STORAGE_PROTOCOL_SPECIFIC_DATA* pdata = (STORAGE_PROTOCOL_SPECIFIC_DATA*)query->AdditionalParameters;

        query->PropertyId = StorageDeviceProtocolSpecificProperty;
        query->QueryType = PropertyStandardQuery;

        pdata->ProtocolType = ProtocolTypeNvme;
        pdata->DataType = NVMeDataTypeLogPage;
        pdata->ProtocolDataRequestValue = NVME_LOG_PAGE_DEVICE_SELF_TEST;
        pdata->ProtocolDataRequestSubValue = NVME_NAMESPACE_ALL;
        pdata->ProtocolDataOffset = sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA);
        pdata->ProtocolDataLength = sizeof(NVME_DEVICE_SELF_TEST_LOG);

        // Send request down.
        result = DeviceIoControl(hdl, IOCTL_STORAGE_QUERY_PROPERTY,
                                 bufptr, bufsize, bufptr, bufsize, &retlen, NULL );
        if (!result) break;

        // Validate the returned data.
        if ((pdesc->Version != sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR)) ||
            (pdesc->Size != sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR))) {
            printf("GetSelfTestLog: Data Descriptor Header is not valid.\n");
            break;
        }

        pdata = &pdesc->ProtocolSpecificData;

        if ((pdata->ProtocolDataOffset < sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA)) ||
            (pdata->ProtocolDataLength < sizeof(NVME_DEVICE_SELF_TEST_LOG))) {
            printf("GetSelfTestLog: ProtocolData Offset/Length is not valid.\n");
            break;
        }

        memcpy(stl, (PCHAR)pdata + pdata->ProtocolDataOffset, sizeof(NVME_DEVICE_SELF_TEST_LOG));

        result = TRUE;
    } while(0);

    if (bufptr != NULL) free(bufptr);
    return result;
}


BOOL NvmeUtil::DeviceSelfTest(HANDLE hdl, eSelfTestCode testcode) {
    BOOL  result = FALSE;
    PVOID buffer = NULL;
    ULONG bufferLength = 0;
    ULONG returnedLength = 0;

    STORAGE_PROTOCOL_COMMAND* pCmd = NULL;
    PNVME_COMMAND pNvmeCmd;

    // Allocate buffer for use.
    bufferLength = offsetof(STORAGE_PROTOCOL_COMMAND, Command) +
                   STORAGE_PROTOCOL_COMMAND_LENGTH_NVME;
    buffer = malloc(bufferLength);

    if (buffer == NULL) {
        printf("DeviceSelfTest: allocate buffer failed.\n");
        goto exit;
    }


    ZeroMemory(buffer, bufferLength);
    pCmd = (STORAGE_PROTOCOL_COMMAND*)buffer;

    pCmd->Length = sizeof(STORAGE_PROTOCOL_COMMAND);
    pCmd->Version = STORAGE_PROTOCOL_STRUCTURE_VERSION;
    pCmd->ProtocolType = ProtocolTypeNvme;
    pCmd->Flags = STORAGE_PROTOCOL_COMMAND_FLAG_ADAPTER_REQUEST;
    pCmd->CommandLength = STORAGE_PROTOCOL_COMMAND_LENGTH_NVME;
    pCmd->ErrorInfoLength = 0;
    pCmd->ErrorInfoOffset = 0;
    pCmd->DataFromDeviceBufferOffset = 0;
    pCmd->DataFromDeviceTransferLength = 0;
    pCmd->TimeOutValue = 15; // In seconds
    pCmd->CommandSpecific = STORAGE_PROTOCOL_SPECIFIC_NVME_ADMIN_COMMAND;

    pNvmeCmd = (PNVME_COMMAND)pCmd->Command;
    pNvmeCmd->CDW0.OPC = NVME_ADMIN_COMMAND_DEVICE_SELF_TEST;
    pNvmeCmd->NSID = NVME_NAMESPACE_ALL;
    pNvmeCmd->u.GENERAL.CDW10 = testcode;

    // Send request down.
    result = DeviceIoControl(hdl,
                             IOCTL_STORAGE_PROTOCOL_COMMAND,
                             buffer,
                             bufferLength,
                             buffer,
                             bufferLength,
                             &returnedLength,
                             NULL
                             );

exit:
    if (buffer != NULL) {
        free(buffer);
    }

    return result;
}

