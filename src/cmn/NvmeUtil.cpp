#include "NvmeUtil.h"
#include <stdlib.h>
#include <stddef.h>
#include "winioctl.h"

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

BOOL NvmeUtil::GetSMARTHealthInfoLog(HANDLE hDevice, PNVME_HEALTH_INFO_LOG pHealthInfoLog){
    BOOL    result = FALSE;
    PVOID   buffer = NULL;
    ULONG   bufferLength = 0;
    ULONG   returnedLength = 0;

    PSTORAGE_PROPERTY_QUERY query = NULL;
    PSTORAGE_PROTOCOL_SPECIFIC_DATA protocolData = NULL;
    PSTORAGE_PROTOCOL_DATA_DESCRIPTOR protocolDataDescr = NULL;

    if (NULL == pHealthInfoLog) {
        printf("GetSMARTHealthInfoLog: invalid input.\n");
        goto exit;
    }

    // Allocate buffer for use.
    bufferLength = offsetof(STORAGE_PROPERTY_QUERY, AdditionalParameters)
        + sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA)
        + sizeof(NVME_HEALTH_INFO_LOG);
    buffer = malloc(bufferLength);

    if (buffer == NULL) {
        printf("GetSMARTHealthInfoLog: allocate buffer failed.\n");
        goto exit;
    }

    ZeroMemory(buffer, bufferLength);

    query = (PSTORAGE_PROPERTY_QUERY)buffer;
    protocolDataDescr = (PSTORAGE_PROTOCOL_DATA_DESCRIPTOR)buffer;
    protocolData = (PSTORAGE_PROTOCOL_SPECIFIC_DATA)query->AdditionalParameters;

    query->PropertyId = StorageDeviceProtocolSpecificProperty;
    query->QueryType = PropertyStandardQuery;

    protocolData->ProtocolType = ProtocolTypeNvme;
    protocolData->DataType = NVMeDataTypeLogPage;
    protocolData->ProtocolDataRequestValue = NVME_LOG_PAGE_HEALTH_INFO;
    protocolData->ProtocolDataRequestSubValue = NVME_NAMESPACE_ALL;
    protocolData->ProtocolDataOffset = sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA);
    protocolData->ProtocolDataLength = sizeof(NVME_HEALTH_INFO_LOG);

    // Send request down.  
    result = DeviceIoControl(hDevice,
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
        (protocolDataDescr->Size != sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR))) {
        printf("GetSMARTHealthInfoLog: Data Descriptor Header is not valid.\n");
        result = FALSE;
        goto exit;
    }

    protocolData = &protocolDataDescr->ProtocolSpecificData;

    if ((protocolData->ProtocolDataOffset < sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA)) ||
        (protocolData->ProtocolDataLength < sizeof(NVME_HEALTH_INFO_LOG))) {
        printf("IdentifyController: ProtocolData Offset/Length is not valid.\n");
        result = FALSE;
        goto exit;
    }

    memcpy(pHealthInfoLog, (PCHAR)protocolData + protocolData->ProtocolDataOffset, sizeof(NVME_HEALTH_INFO_LOG));
 
    result = TRUE; 
exit:
    if (buffer != NULL)
    {
        free(buffer);
    }

    return result;
}

BOOL NvmeUtil::GetSelfTestLog(HANDLE hDevice, PNVME_DEVICE_SELF_TEST_LOG pSelfTestLog) {
    BOOL    result = FALSE;
    PVOID   buffer = NULL;
    ULONG   bufferLength = 0;
    ULONG   returnedLength = 0;

    PSTORAGE_PROPERTY_QUERY query = NULL;
    PSTORAGE_PROTOCOL_SPECIFIC_DATA protocolData = NULL;
    PSTORAGE_PROTOCOL_DATA_DESCRIPTOR protocolDataDescr = NULL;

    if (NULL == pSelfTestLog) {
        printf("GetSelfTestLog: invalid input.\n");
        goto exit;
    }

    // Allocate buffer for use.
    bufferLength = offsetof(STORAGE_PROPERTY_QUERY, AdditionalParameters)
        + sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA)
        + sizeof(NVME_DEVICE_SELF_TEST_LOG);
    buffer = malloc(bufferLength);

    if (buffer == NULL) {
        printf("GetSelfTestLog: allocate buffer failed.\n");
        goto exit;
    }

    ZeroMemory(buffer, bufferLength);

    query = (PSTORAGE_PROPERTY_QUERY)buffer;
    protocolDataDescr = (PSTORAGE_PROTOCOL_DATA_DESCRIPTOR)buffer;
    protocolData = (PSTORAGE_PROTOCOL_SPECIFIC_DATA)query->AdditionalParameters;

    query->PropertyId = StorageDeviceProtocolSpecificProperty;
    query->QueryType = PropertyStandardQuery;

    protocolData->ProtocolType = ProtocolTypeNvme;
    protocolData->DataType = NVMeDataTypeLogPage;
    protocolData->ProtocolDataRequestValue = NVME_LOG_PAGE_DEVICE_SELF_TEST;
    protocolData->ProtocolDataRequestSubValue = NVME_NAMESPACE_ALL;
    protocolData->ProtocolDataOffset = sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA);
    protocolData->ProtocolDataLength = sizeof(NVME_DEVICE_SELF_TEST_LOG);

    // Send request down.  
    result = DeviceIoControl(hDevice,
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
        (protocolDataDescr->Size != sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR))) {
        printf("GetSelfTestLog: Data Descriptor Header is not valid.\n");
        result = FALSE;
        goto exit;
    }

    protocolData = &protocolDataDescr->ProtocolSpecificData;

    if ((protocolData->ProtocolDataOffset < sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA)) ||
        (protocolData->ProtocolDataLength < sizeof(NVME_DEVICE_SELF_TEST_LOG))) {
        printf("GetSelfTestLog: ProtocolData Offset/Length is not valid.\n");
        result = FALSE;
        goto exit;
    }

    memcpy(pSelfTestLog, (PCHAR)protocolData + protocolData->ProtocolDataOffset, sizeof(NVME_DEVICE_SELF_TEST_LOG));

    result = TRUE;
exit:
    if (buffer != NULL)
    {
        free(buffer);
    }

    return result;
}

BOOL NvmeUtil::DeviceSelfTest(HANDLE hDevice, eDeviceSelftestCode selftestCode) {
    BOOL  result = FALSE;
    PVOID buffer = NULL;
    ULONG bufferLength = 0;
    ULONG returnedLength = 0;

    PSTORAGE_PROTOCOL_COMMAND pCmd = NULL;
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
    pCmd = (PSTORAGE_PROTOCOL_COMMAND)buffer;

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
    pNvmeCmd->u.GENERAL.CDW10 = selftestCode;

    // Send request down.
    result = DeviceIoControl(hDevice,
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
