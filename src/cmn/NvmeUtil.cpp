#include "NvmeUtil.h"
#include<stdlib.h>
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
