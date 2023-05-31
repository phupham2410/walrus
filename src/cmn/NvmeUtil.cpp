#include "NvmeUtil.h"
#include <stdlib.h>
#include <stddef.h>
#include <winioctl.h>
#include <ntddscsi.h>
#include<winerror.h>
#include <tchar.h>
#include<nvme.h>
using namespace std;


static void PrintDataBuffer(PUCHAR DataBuffer, ULONG BufferLength)
{
    ULONG Cnt;
    UCHAR Str[32] = { 0 };

    fprintf(stdout, "        00  01  02  03  04  05  06  07   08  09  0A  0B  0C  0D  0E  0F\n");
    fprintf(stdout, "        ---------------------------------------------------------------\n");

    int i = 0;
    for (Cnt = 0; Cnt < BufferLength; Cnt++)
    {
        // print address
        if ((Cnt) % 16 == 0)
        {
            fprintf(stdout, " 0x%03X  ", Cnt);
        }

        // print hex data
        fprintf(stdout, "%02X  ", DataBuffer[Cnt]);
        if (isprint(DataBuffer[Cnt]))
        {
            Str[i] = DataBuffer[Cnt];
        }
        else
        {
            Str[i] = '.';
        }
        i++;
        if ((Cnt + 1) % 8 == 0)
        {
            fprintf(stdout, " ");
            Str[i++] = ' ';
        }

        // print ascii character if printable
        if ((Cnt + 1) % 16 == 0)
        {
            Str[i++] = '\0';
            i = 0;
            fprintf(stdout, "%s\n", Str);
        }
    }
    fprintf(stdout, "\n\n");
}

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



BOOL NvmeUtil::IdentifyActiveNSIDList(HANDLE hDevice, PNVME_IDENTIFY_ACTIVE_NAMESPACE_LIST pNsidList) {
    BOOL  result = FALSE;
    PVOID buffer = NULL;
    ULONG bufferLength = 0;
    ULONG returnedLength = 0;

    PSTORAGE_PROPERTY_QUERY query = NULL;
    PSTORAGE_PROTOCOL_SPECIFIC_DATA protocolData = NULL;
    PSTORAGE_PROTOCOL_DATA_DESCRIPTOR protocolDataDescr = NULL;

    // Allocate buffer for use.
    bufferLength = offsetof(STORAGE_PROPERTY_QUERY, AdditionalParameters) +
                   sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA) +
                   sizeof(NVME_IDENTIFY_ACTIVE_NAMESPACE_LIST);
    buffer = malloc(bufferLength);

    if (buffer == NULL) {
         printf("IdentifyActiveNSIDList: allocate buffer failed.\n");
        goto exit;
    }

    // Initialize query data structure to get Identify Active Namespace ID list.
    ZeroMemory(buffer, bufferLength);

    query = (PSTORAGE_PROPERTY_QUERY)buffer;
    protocolDataDescr = (PSTORAGE_PROTOCOL_DATA_DESCRIPTOR)buffer;
    protocolData = (PSTORAGE_PROTOCOL_SPECIFIC_DATA)query->AdditionalParameters;

    query->PropertyId = StorageDeviceProtocolSpecificProperty;
    query->QueryType = PropertyStandardQuery;

    protocolData->ProtocolType = ProtocolTypeNvme;
    protocolData->DataType = NVMeDataTypeIdentify;
    protocolData->ProtocolDataRequestValue =
        NVME_IDENTIFY_CNS_ACTIVE_NAMESPACES;
    protocolData->ProtocolDataRequestSubValue = 0;  // to retrieve all IDs
    protocolData->ProtocolDataOffset = sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA);
    protocolData->ProtocolDataLength = sizeof(NVME_IDENTIFY_ACTIVE_NAMESPACE_LIST);

    // Send request down.
    result = DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
                                    buffer, bufferLength, buffer, bufferLength,
                                    &returnedLength, NULL);

    if (!result) goto exit;

    //
    // Validate the returned data.
    //
    if ((protocolDataDescr->Version !=
         sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR)) ||
        (protocolDataDescr->Size != sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR))) {
        printf("IdentifyActiveNSIDList: Data Descriptor Header is not valid.\n");
        result = FALSE;
        goto exit;
        goto exit;
    }

    protocolData = &protocolDataDescr->ProtocolSpecificData;

    if ((protocolData->ProtocolDataOffset >
         sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA)) ||
        (protocolData->ProtocolDataLength < sizeof(NVME_IDENTIFY_ACTIVE_NAMESPACE_LIST))) {
        printf("IdentifyActiveNSIDList: ProtocolData Offset/Length is not valid.\n");
        result = FALSE;
        goto exit;
    }

    memcpy(pNsidList, (PCHAR)protocolData + protocolData->ProtocolDataOffset, sizeof(NVME_CHANGED_NAMESPACE_LIST_LOG));

    result = TRUE;

exit:

    if (buffer != NULL) {
        free(buffer);
    }

    return result;
}

BOOL NvmeUtil::GetSCSIAAddress(HANDLE hDevice, PSCSI_ADDRESS pScsiAddress)
{
    BOOL result = FALSE;
    ULONG returnedLength = 0;
    if (pScsiAddress && hDevice != INVALID_HANDLE_VALUE)
    {

        memset(pScsiAddress, 0, sizeof(SCSI_ADDRESS));
        result = DeviceIoControl(hDevice, IOCTL_SCSI_GET_ADDRESS, NULL, 0, pScsiAddress, sizeof(SCSI_ADDRESS), &returnedLength, NULL);
        if (!result)
        {
            pScsiAddress->PortNumber = UINT8_MAX;
            pScsiAddress->PathId = UINT8_MAX;
            pScsiAddress->TargetId = UINT8_MAX;
            pScsiAddress->Lun = UINT8_MAX;
            result = FALSE;
        }
    }

    return result;
}

#define SRB_FLAGS_DATA_IN                   0x00000040
#define SRB_FLAGS_DATA_OUT                  0x00000080

static BOOL sendSCSIPassThrough(tScsiContext *scsiCtx)
{
    BOOL          ret = FALSE;
    BOOL          success = FALSE;
    ULONG         returned_data = 0;
    tScsiPassThroughIOStruct* sptdioDB = (tScsiPassThroughIOStruct*) malloc(sizeof(tScsiPassThroughIOStruct) + scsiCtx->dataLength);
    if (!sptdioDB) {
        return FALSE;
    }


    memset(sptdioDB, 0, sizeof(tScsiPassThroughIOStruct) + scsiCtx->dataLength);

    sptdioDB->scsiPassthrough.Length = sizeof(SCSI_PASS_THROUGH);
    sptdioDB->scsiPassthrough.PathId = scsiCtx->scsiAddr.PathId;
    sptdioDB->scsiPassthrough.TargetId = scsiCtx->scsiAddr.TargetId;
    sptdioDB->scsiPassthrough.Lun = scsiCtx->scsiAddr.Lun;
    sptdioDB->scsiPassthrough.CdbLength = scsiCtx->cdbLength;
    sptdioDB->scsiPassthrough.ScsiStatus = 0;
    sptdioDB->scsiPassthrough.SenseInfoLength = C_CAST(UCHAR, scsiCtx->senseDataSize);
    ZeroMemory(sptdioDB->senseBuffer, SPC3_SENSE_LEN);

    switch (scsiCtx->direction)
    {
    case XFER_DATA_IN:
        sptdioDB->scsiPassthrough.DataIn = SCSI_IOCTL_DATA_IN;
        sptdioDB->scsiPassthrough.DataTransferLength = scsiCtx->dataLength;
        sptdioDB->scsiPassthrough.DataBufferOffset = offsetof(tScsiPassThroughIOStruct, dataBuffer);
        break;
    case XFER_DATA_OUT:
        sptdioDB->scsiPassthrough.DataIn = SCSI_IOCTL_DATA_OUT;
        sptdioDB->scsiPassthrough.DataTransferLength = scsiCtx->dataLength;
        sptdioDB->scsiPassthrough.DataBufferOffset = offsetof(tScsiPassThroughIOStruct, dataBuffer);
        break;
    case XFER_NO_DATA:
        sptdioDB->scsiPassthrough.DataIn = SCSI_IOCTL_DATA_UNSPECIFIED;
        sptdioDB->scsiPassthrough.DataTransferLength = 0;
        sptdioDB->scsiPassthrough.DataBufferOffset = offsetof(tScsiPassThroughIOStruct, dataBuffer);
        break;
    default:
        return FALSE;

    }

    if (scsiCtx->timeout != 0) {
        sptdioDB->scsiPassthrough.TimeOutValue = scsiCtx->timeout;
    }
    else {
        sptdioDB->scsiPassthrough.TimeOutValue = 15;
    }


    sptdioDB->scsiPassthrough.SenseInfoOffset = offsetof(tScsiPassThroughIOStruct, senseBuffer);
    memcpy(sptdioDB->scsiPassthrough.Cdb, scsiCtx->cdb, sizeof(sptdioDB->scsiPassthrough.Cdb));

    //clear any cached errors before we try to send the command
    SetLastError(ERROR_SUCCESS);
    scsiCtx->lastError = 0;

    DWORD scsiPassThroughInLength = sizeof(tScsiPassThroughIOStruct);
    DWORD scsiPassThroughOutLength = sizeof(tScsiPassThroughIOStruct);
    switch (scsiCtx->direction) {
    case XFER_DATA_IN:
        scsiPassThroughOutLength += scsiCtx->dataLength;
        break;
    case XFER_DATA_OUT:
        if (scsiCtx->pdata) {
            memcpy(sptdioDB->dataBuffer, scsiCtx->pdata, scsiCtx->dataLength);
        }
        scsiPassThroughInLength += scsiCtx->dataLength;
        break;
    default:
        break;
    }

    success = DeviceIoControl(scsiCtx->hDevice,
                              IOCTL_SCSI_PASS_THROUGH,
                              &sptdioDB->scsiPassthrough,
                              scsiPassThroughInLength,
                              &sptdioDB->scsiPassthrough,
                              scsiPassThroughOutLength,
                              &returned_data,
                              NULL);
    scsiCtx->lastError = GetLastError();

    if (success) {
        ret = TRUE;
        if (scsiCtx->pdata && scsiCtx->direction == XFER_DATA_IN) {
            memcpy(scsiCtx->pdata, sptdioDB->dataBuffer, scsiCtx->dataLength);
        }
    } else {
        ret = FALSE;
    }
    scsiCtx->returnStatus.senseKey = sptdioDB->scsiPassthrough.ScsiStatus;

    // Any sense data?
    if (scsiCtx->psense != NULL && scsiCtx->senseDataSize > 0) {
        memcpy(scsiCtx->psense, sptdioDB->senseBuffer, M_Min(sptdioDB->scsiPassthrough.SenseInfoLength, scsiCtx->senseDataSize));
    }

    if (scsiCtx->psense != NULL) {
        scsiCtx->returnStatus.format = scsiCtx->psense[0];
        switch (scsiCtx->returnStatus.format & 0x7F) {
        case SCSI_SENSE_CUR_INFO_FIXED:
        case SCSI_SENSE_DEFER_ERR_FIXED:
            scsiCtx->returnStatus.senseKey = scsiCtx->psense[2] & 0x0F;
            scsiCtx->returnStatus.asc = scsiCtx->psense[12];
            scsiCtx->returnStatus.ascq = scsiCtx->psense[13];
            break;
        case SCSI_SENSE_CUR_INFO_DESC:
        case SCSI_SENSE_DEFER_ERR_DESC:
            scsiCtx->returnStatus.senseKey = scsiCtx->psense[1] & 0x0F;
            scsiCtx->returnStatus.asc = scsiCtx->psense[2];
            scsiCtx->returnStatus.ascq = scsiCtx->psense[3];
            break;
        }
    }

    std::cout << "sptdioDB->scsiPassthrough.ScsiStatus:" << ( unsigned int )sptdioDB->scsiPassthrough.ScsiStatus << std::endl;
    std::cout << "returnStatus.format:" <<  ( unsigned int )scsiCtx->returnStatus.format << std::endl;
    std::cout << "returnStatus.senseKey:" <<  ( unsigned int )scsiCtx->returnStatus.senseKey << std::endl;
    std::cout << "scsiCtx->returnStatus.asc:" << ( unsigned int ) scsiCtx->returnStatus.asc << std::endl;
    std::cout << "scsiCtx->returnStatus.ascq:" << ( unsigned int ) scsiCtx->returnStatus.ascq << std::endl;

    safe_Free(sptdioDB);

    return ret;
}


static BOOL scsiCtx_Direct(tScsiContext *scsiCtx)
{
    BOOL          ret = FALSE;
    BOOL          success = FALSE;
    ULONG         returned_data = 0;
    tScsiPassThroughIOStruct* sptdioDB = (tScsiPassThroughIOStruct*) malloc(sizeof(tScsiPassThroughIOStruct));
    bool localAlignedBuffer = false;
    uint8_t *alignedPointer = scsiCtx->pdata;
    uint8_t *localBuffer = NULL;//we need to save this to free up the memory properly later.
    //Check the alignment...if we need to use a local buffer, we'll use one, then copy the data back
    if (scsiCtx->pdata && scsiCtx->alignmentMask != 0)
    {
        //This means the driver requires some sort of aligned pointer for the data buffer...so let's check and make sure that the user's pointer is aligned
        //If the user's pointer isn't aligned properly, align something local that is aligned to meet the driver's requirements, then copy data back for them.
        alignedPointer = C_CAST(uint8_t*, (C_CAST(UINT_PTR, scsiCtx->pdata) + C_CAST(UINT_PTR, scsiCtx->alignmentMask)) & ~C_CAST(UINT_PTR, scsiCtx->alignmentMask));
        if (alignedPointer != scsiCtx->pdata)
        {
            localAlignedBuffer = true;
            uint32_t totalBufferSize = scsiCtx->dataLength + C_CAST(uint32_t, scsiCtx->alignmentMask);
            localBuffer = C_CAST(uint8_t*, calloc(totalBufferSize, sizeof(uint8_t)));//TODO: If we want to remove allocating more memory, we should investigate making the scsiCtx->pdata a double pointer so we can reallocate it for the user.
            if (!localBuffer)
            {
                perror("error allocating aligned buffer for ATA Passthrough Direct...attempting to use user's pointer.");
                localAlignedBuffer = false;
                alignedPointer = scsiCtx->pdata;
            }
            else
            {
                alignedPointer = C_CAST(uint8_t*, (C_CAST(UINT_PTR, localBuffer) + C_CAST(UINT_PTR, scsiCtx->alignmentMask)) & ~C_CAST(UINT_PTR, scsiCtx->alignmentMask));
                if (scsiCtx->direction == XFER_DATA_OUT)
                {
                    memcpy(alignedPointer, scsiCtx->pdata, scsiCtx->dataLength);
                }
            }
        }
    }

    if (!sptdioDB) {
        return FALSE;
    }


    memset(sptdioDB, 0, sizeof(tScsiPassThroughIOStruct));

    sptdioDB->scsiPassthroughDirect.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
    sptdioDB->scsiPassthroughDirect.PathId = scsiCtx->scsiAddr.PathId;
    sptdioDB->scsiPassthroughDirect.TargetId = scsiCtx->scsiAddr.TargetId;
    sptdioDB->scsiPassthroughDirect.Lun = scsiCtx->scsiAddr.Lun;
    sptdioDB->scsiPassthroughDirect.CdbLength = scsiCtx->cdbLength;
    sptdioDB->scsiPassthroughDirect.ScsiStatus = 255;
    sptdioDB->scsiPassthroughDirect.SenseInfoLength = C_CAST(UCHAR, scsiCtx->senseDataSize);
    ZeroMemory(sptdioDB->senseBuffer, SPC3_SENSE_LEN);

    switch (scsiCtx->direction)
    {
    case XFER_DATA_IN:
        sptdioDB->scsiPassthroughDirect.DataIn = SCSI_IOCTL_DATA_IN;
        sptdioDB->scsiPassthroughDirect.DataTransferLength = scsiCtx->dataLength;
        sptdioDB->scsiPassthroughDirect.DataBuffer = alignedPointer;
        break;
    case XFER_DATA_OUT:
        sptdioDB->scsiPassthroughDirect.DataIn = SCSI_IOCTL_DATA_OUT;
        sptdioDB->scsiPassthroughDirect.DataTransferLength = scsiCtx->dataLength;
        sptdioDB->scsiPassthroughDirect.DataBuffer = alignedPointer;
        break;
    case XFER_NO_DATA:
        sptdioDB->scsiPassthroughDirect.DataIn = SCSI_IOCTL_DATA_UNSPECIFIED;
        sptdioDB->scsiPassthroughDirect.DataTransferLength = 0;
        sptdioDB->scsiPassthroughDirect.DataBuffer = NULL;
        break;
    default:
        return FALSE;

    }

    if (scsiCtx->timeout != 0) {
        sptdioDB->scsiPassthroughDirect.TimeOutValue = scsiCtx->timeout;
    }
    else {
        sptdioDB->scsiPassthroughDirect.TimeOutValue = 15;
    }


    sptdioDB->scsiPassthroughDirect.SenseInfoOffset = offsetof(tScsiPassThroughIOStruct, senseBuffer);
    memcpy(sptdioDB->scsiPassthroughDirect.Cdb, scsiCtx->cdb, sizeof(sptdioDB->scsiPassthroughDirect.Cdb));

    //clear any cached errors before we try to send the command
    SetLastError(ERROR_SUCCESS);
    scsiCtx->lastError = 0;

    DWORD scsiPassThroughBufLen = sizeof(tScsiPassThroughIOStruct);


    success = DeviceIoControl(scsiCtx->hDevice,
                              IOCTL_SCSI_PASS_THROUGH_DIRECT,
                              &sptdioDB->scsiPassthroughDirect,
                              scsiPassThroughBufLen,
                              &sptdioDB->scsiPassthroughDirect,
                              scsiPassThroughBufLen,
                              &returned_data,
                              NULL);
    scsiCtx->lastError = GetLastError();

    if (success) {
        ret = TRUE;
        if (scsiCtx->pdata && scsiCtx->direction == XFER_DATA_IN && localAlignedBuffer) {
            memcpy(scsiCtx->pdata, sptdioDB->dataBuffer, scsiCtx->dataLength);
        }
    } else {
        ret = FALSE;
    }
    scsiCtx->returnStatus.senseKey = sptdioDB->scsiPassthrough.ScsiStatus;

    // Any sense data?
    if (scsiCtx->psense != NULL && scsiCtx->senseDataSize > 0) {
        memcpy(scsiCtx->psense, sptdioDB->senseBuffer, M_Min(sptdioDB->scsiPassthrough.SenseInfoLength, scsiCtx->senseDataSize));
    }

    if (scsiCtx->psense != NULL) {
        scsiCtx->returnStatus.format = scsiCtx->psense[0];
        switch (scsiCtx->returnStatus.format & 0x7F) {
        case SCSI_SENSE_CUR_INFO_FIXED:
        case SCSI_SENSE_DEFER_ERR_FIXED:
            scsiCtx->returnStatus.senseKey = scsiCtx->psense[2] & 0x0F;
            scsiCtx->returnStatus.asc = scsiCtx->psense[12];
            scsiCtx->returnStatus.ascq = scsiCtx->psense[13];
            break;
        case SCSI_SENSE_CUR_INFO_DESC:
        case SCSI_SENSE_DEFER_ERR_DESC:
            scsiCtx->returnStatus.senseKey = scsiCtx->psense[1] & 0x0F;
            scsiCtx->returnStatus.asc = scsiCtx->psense[2];
            scsiCtx->returnStatus.ascq = scsiCtx->psense[3];
            break;
        }
    }


    safe_Free(sptdioDB);

    return ret;
}

static BOOL sendSCSIPassThrough_EX(tScsiContext *scsiCtx)
{
    BOOL           ret = FALSE;
    BOOL          success = FALSE;
    ULONG         returned_data = 0;
    tScsiPassThroughEXIOStruct *sptdioEx = C_CAST(tScsiPassThroughEXIOStruct*, malloc(sizeof(tScsiPassThroughEXIOStruct)));
    if (!sptdioEx) {
        return FALSE;
    }

    memset(sptdioEx, 0, sizeof(tScsiPassThroughEXIOStruct));
    memset(&sptdioEx->scsiPassThroughEX, 0, sizeof(SCSI_PASS_THROUGH_EX));
    sptdioEx->scsiPassThroughEX.Version = 0;//MSDN says set this to zero
    sptdioEx->scsiPassThroughEX.Length = sizeof(SCSI_PASS_THROUGH_EX);
    sptdioEx->scsiPassThroughEX.CdbLength = scsiCtx->cdbLength;
    sptdioEx->scsiPassThroughEX.StorAddressLength = sizeof(STOR_ADDR_BTL8);
    sptdioEx->scsiPassThroughEX.ScsiStatus = 0;
    sptdioEx->scsiPassThroughEX.SenseInfoLength = SPC3_SENSE_LEN;
    sptdioEx->scsiPassThroughEX.Reserved = 0;
    //setup the store port address struct
    sptdioEx->storeAddr.Type = STOR_ADDRESS_TYPE_BTL8;//Microsoft documentation says to set this
    sptdioEx->storeAddr.AddressLength = STOR_ADDR_BTL8_ADDRESS_LENGTH;
    //The host bus adapter (HBA) port number.
    sptdioEx->storeAddr.Port = scsiCtx->scsiAddr.PortNumber;//This may or maynot be correct. Need to test it.
    sptdioEx->storeAddr.Path = scsiCtx->scsiAddr.PathId;
    sptdioEx->storeAddr.Target = scsiCtx->scsiAddr.TargetId;
    sptdioEx->storeAddr.Lun = scsiCtx->scsiAddr.Lun;
    sptdioEx->storeAddr.Reserved = 0;
    sptdioEx->scsiPassThroughEX.StorAddressOffset = offsetof(tScsiPassThroughEXIOStruct, storeAddr);
    ZeroMemory(sptdioEx->senseBuffer, SPC3_SENSE_LEN);
    switch (scsiCtx->direction) {
    case XFER_DATA_OUT:
        sptdioEx->scsiPassThroughEX.DataDirection = SCSI_IOCTL_DATA_OUT;
        sptdioEx->scsiPassThroughEX.DataOutTransferLength = scsiCtx->dataLength;
        sptdioEx->scsiPassThroughEX.DataOutBufferOffset = offsetof(tScsiPassThroughEXIOStruct, dataOutBuffer);
        sptdioEx->scsiPassThroughEX.DataInBufferOffset = 0;
        break;
    case XFER_DATA_IN:
        sptdioEx->scsiPassThroughEX.DataDirection = SCSI_IOCTL_DATA_IN;
        sptdioEx->scsiPassThroughEX.DataInTransferLength = scsiCtx->dataLength;
        sptdioEx->scsiPassThroughEX.DataInBufferOffset = offsetof(tScsiPassThroughEXIOStruct, dataInBuffer);
        sptdioEx->scsiPassThroughEX.DataOutBufferOffset = 0;
        break;
    case XFER_NO_DATA:
        sptdioEx->scsiPassThroughEX.DataDirection = SCSI_IOCTL_DATA_UNSPECIFIED;
        break;

    default:
        return FALSE;

    }

    if (scsiCtx->timeout != 0) {
        sptdioEx->scsiPassThroughEX.TimeOutValue = scsiCtx->timeout;
    } else {
        sptdioEx->scsiPassThroughEX.TimeOutValue = 15;
    }

    sptdioEx->scsiPassThroughEX.SenseInfoOffset = offsetof(tScsiPassThroughEXIOStruct, senseBuffer);
    memcpy(sptdioEx->scsiPassThroughEX.Cdb, scsiCtx->cdb, scsiCtx->cdbLength);

    SetLastError(ERROR_SUCCESS);//clear any cached errors before we try to send the command
    scsiCtx->lastError = 0;
    DWORD sptBufInLen = sizeof(tScsiPassThroughEXIOStruct);
    DWORD sptBufOutLen = sizeof(tScsiPassThroughEXIOStruct);
    switch (scsiCtx->direction) {
    case XFER_DATA_IN:
        sptBufOutLen += scsiCtx->dataLength;
        break;
    case XFER_DATA_OUT:
        //need to copy the data we're sending to the device over!
        if (scsiCtx->pdata)
        {
            memcpy(sptdioEx->dataOutBuffer, scsiCtx->pdata, scsiCtx->dataLength);
        }
        sptBufInLen += scsiCtx->dataLength;
        break;
    default:
        break;
    }

    success = DeviceIoControl(scsiCtx->hDevice,
                              IOCTL_SCSI_PASS_THROUGH_EX,
                              &sptdioEx->scsiPassThroughEX,
                              sptBufInLen,
                              &sptdioEx->scsiPassThroughEX,
                              sptBufOutLen,
                              &returned_data,
                              NULL);
    scsiCtx->lastError = GetLastError();

    scsiCtx->returnStatus.senseKey = sptdioEx->scsiPassThroughEX.ScsiStatus;

    if (success) {
        ret = TRUE;
        if (scsiCtx->pdata && scsiCtx->direction == XFER_DATA_IN)
        {
            memcpy(scsiCtx->pdata, sptdioEx->dataInBuffer, scsiCtx->dataLength);
        }
    }
    else {
       ret = FALSE;
    }

    // Any sense data?
    if (scsiCtx->psense != NULL && scsiCtx->senseDataSize > 0) {
        memcpy(scsiCtx->psense, &sptdioEx->senseBuffer[0], M_Min(sptdioEx->scsiPassThroughEX.SenseInfoLength, scsiCtx->senseDataSize));
    }

    if (scsiCtx->psense != NULL) {
        //use the format, sensekey, acq, acsq from the sense data buffer we passed in rather than what windows reports...because windows doesn't always match what is in your sense buffer
        scsiCtx->returnStatus.format = scsiCtx->psense[0];
        switch (scsiCtx->returnStatus.format & 0x7F)
        {
        case SCSI_SENSE_CUR_INFO_FIXED:
        case SCSI_SENSE_DEFER_ERR_FIXED:
            scsiCtx->returnStatus.senseKey = scsiCtx->psense[2] & 0x0F;
            scsiCtx->returnStatus.asc = scsiCtx->psense[12];
            scsiCtx->returnStatus.ascq = scsiCtx->psense[13];
            break;
        case SCSI_SENSE_CUR_INFO_DESC:
        case SCSI_SENSE_DEFER_ERR_DESC:
            scsiCtx->returnStatus.senseKey = scsiCtx->psense[1] & 0x0F;
            scsiCtx->returnStatus.asc = scsiCtx->psense[2];
            scsiCtx->returnStatus.ascq = scsiCtx->psense[3];
            break;
        }
    }

    safe_Free(sptdioEx)
     return ret;
}

static BOOL scsiSendCdb(tScsiContext *scsiCtx, uint8_t *cdb, eCDBLen cdbLen, uint8_t *pdata, uint32_t dataLen, eDataTransferDirection dataDirection, uint8_t *senseData, uint32_t senseDataLen, uint32_t timeoutSeconds)
{
    BOOL ret = FALSE;

    uint8_t *senseBuffer = senseData;
    if (!senseBuffer || senseDataLen == 0) {
        senseBuffer = scsiCtx->lastCommandSenseData;
        senseDataLen = SPC3_SENSE_LEN;
    } else {
        memset(senseBuffer, 0, senseDataLen);
    }
    //check a couple of the parameters before continuing
    if (!scsiCtx) {
        perror("context struct is NULL!");
        return FALSE;
    }
    if (!cdb) {
        perror("cdb array is NULL!");
        return FALSE;
    }
    if (cdbLen == CDB_LEN_UNKNOWN) {
        perror("Invalid CDB length specified!");
        return FALSE;
    }
    if (!pdata && dataLen != 0) {
        perror("Datalen must be set to 0 when pdata is NULL");
        return FALSE;
    }

    //set up the context
    scsiCtx->psense = senseBuffer;
    scsiCtx->senseDataSize = senseDataLen;
    memcpy(&scsiCtx->cdb[0], &cdb[0], cdbLen);
    scsiCtx->cdbLength = cdbLen;
    scsiCtx->direction = dataDirection;
    scsiCtx->pdata = pdata;
    scsiCtx->dataLength = dataLen;
    if (scsiCtx->srbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        std::cout << "Try sendSCSIPassThrough_EX\n";
        //ret = sendSCSIPassThrough(scsiCtx);
        ret = scsiCtx_Direct(scsiCtx);
    } else {
        ret = sendSCSIPassThrough(scsiCtx);
    }
    if (senseData && senseDataLen > 0 && senseData != scsiCtx->lastCommandSenseData) {
        memcpy(scsiCtx->lastCommandSenseData, senseBuffer, M_Min(SPC3_SENSE_LEN, senseDataLen));
    }

    return ret;
}


BOOL NvmeUtil::ScsiReportLuns(tScsiContext *scsiCtx, uint8_t selectReport, uint32_t allocationLength, uint8_t *ptrData) {
    BOOL       result = FALSE;
    uint8_t   cdb[CDB_LEN_12] = { 0 };
    scsiCtx->senseDataSize = SPC3_SENSE_LEN;

    // Set up the CDB.
    cdb[SCSI_OPERATION_CODE] = REPORT_LUNS_CMD;
    cdb[1] = RESERVED;
    cdb[2] = selectReport;
    cdb[3] = RESERVED;
    cdb[4] = RESERVED;
    cdb[5] = RESERVED;
    cdb[6] = M_Byte(allocationLength, 3);
    cdb[7] = M_Byte(allocationLength, 2);
    cdb[8] = M_Byte(allocationLength, 1);
    cdb[9] = M_Byte(allocationLength, 0);
    cdb[10] = RESERVED;
    cdb[11] = 0;//control

    //send the command
    if (allocationLength > 0) {
        result = scsiSendCdb(scsiCtx, &cdb[0], (eCDBLen)sizeof(cdb), ptrData, allocationLength, XFER_DATA_IN, scsiCtx->lastCommandSenseData, SPC3_SENSE_LEN, 15);
    } else {
        result = scsiSendCdb(scsiCtx, &cdb[0], (eCDBLen)sizeof(cdb), NULL, 0, XFER_NO_DATA, scsiCtx->lastCommandSenseData, SPC3_SENSE_LEN, 15);
    }

    return result;
}

#define MAX_VOL_BITS (32)
#define MAX_VOL_STR_LEN (8)
#define MAX_DISK_EXTENTS (32)
#define WIN_PHYSICAL_DRIVE  "\\\\.\\PhysicalDrive"

BOOL os_Unmount_File_Systems_On_Device(tScsiContext *scsiCtx)
{
    BOOL ret = TRUE;
    DWORD driveLetters = 0;
    TCHAR currentLetter = 'A';
    uint32_t volumeCounter = 0;
    uint32_t os_drive_number = UINT32_MAX;
    uint32_t volumeBitField = 0;
    sscanf_s(scsiCtx->driveName, WIN_PHYSICAL_DRIVE "%u" , &os_drive_number);


    driveLetters = GetLogicalDrives();//TODO: This will remount everything. If we can figure out a better way to do this, we should so that not everything is remounted. - TJE

    uint8_t volIter = 0;
    for (volIter = 0; volIter < MAX_VOL_BITS; ++volIter, ++currentLetter, ++volumeCounter)
    {
        if (M_BitN(volIter) & driveLetters)
        {
            //a volume with this letter exists...check it's physical device number
            TCHAR volume_name[MAX_VOL_STR_LEN] = { 0 };
            TCHAR *ptrLetterName = &volume_name[0];
            _sntprintf_s(ptrLetterName, MAX_VOL_STR_LEN, _TRUNCATE, TEXT("\\\\.\\%c:"), currentLetter);
            HANDLE letterHandle = CreateFile(ptrLetterName,
                                             GENERIC_WRITE | GENERIC_READ,
                                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                                             NULL,
                                             OPEN_EXISTING,
                                             //#if !defined(WINDOWS_DISABLE_OVERLAPPED)
                                             //                    FILE_FLAG_OVERLAPPED,
                                             //#else
                                             0,
                                             //#endif
                                             NULL);
            if (letterHandle != INVALID_HANDLE_VALUE)
            {
                DWORD returnedBytes = 0;
                DWORD maxExtents = MAX_DISK_EXTENTS;//https://technet.microsoft.com/en-us/library/cc772180(v=ws.11).aspx
                PVOLUME_DISK_EXTENTS diskExtents = NULL;
                DWORD diskExtentsSizeBytes = sizeof(VOLUME_DISK_EXTENTS) + (sizeof(DISK_EXTENT) * maxExtents);
                diskExtents = C_CAST(PVOLUME_DISK_EXTENTS, malloc(diskExtentsSizeBytes));
                if (diskExtents)
                {
                    memset(diskExtents, 0, diskExtentsSizeBytes);
                    if (DeviceIoControl(letterHandle, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, diskExtents, diskExtentsSizeBytes, &returnedBytes, NULL))
                    {
                        for (DWORD counter = 0; counter < diskExtents->NumberOfDiskExtents; ++counter)
                        {
                            if (diskExtents->Extents[counter].DiskNumber == os_drive_number)
                            {

                                //Set a bit to note that this particular volume (letter) is on this device
                                volumeBitField |= (UINT32_C(1) << volumeCounter);

                                //now we need to determine if this volume has the system directory on it.
                                TCHAR systemDirectoryPath[MAX_PATH] = { 0 };

                                if (GetSystemDirectory(systemDirectoryPath, MAX_PATH) > 0)
                                {
                                    if (_tcslen(systemDirectoryPath) > 0)
                                    {
                                        //we need to check only the first letter of the returned string since this is the volume letter
                                        if (systemDirectoryPath[0] == currentLetter)
                                        {
                                            //This volume contains a system directory

                                        }
                                    }

                                }
                            }
                        }
                    }
                    //DWORD lastError = GetLastError();
                    safe_Free(diskExtents)
                }
            }
            CloseHandle(letterHandle);
        }
    }

    //If the volume bitfield is blank, then there is nothing to unmount - TJE
    if (volumeBitField > 0)
    {
        //go through each bit in the bitfield. Bit0 = A, BIT1 = B, BIT2 = C, etc
        //unmount each volume for the specified device.
        uint8_t volIter = 0;
        TCHAR volumeLetter = 'A';//always start with A
        for (volIter = 0; volIter < MAX_VOL_BITS; ++volIter, ++volumeLetter)
        {
            if (M_BitN(volIter) & volumeBitField)
            {
                //found a volume.
                //Steps:
                //1. open handle to the volume
                //2. lock the volume: FSCTL_LOCK_VOLUME
                //3. dismount volume: FSCTL_DISMOUNT_VOLUME
                //4. unlock the volume: FSCTL_UNLOCK_VOLUME
                //5. close the handle to the volume
                HANDLE volumeHandle = INVALID_HANDLE_VALUE;
                DWORD bytesReturned = 0;
                TCHAR volumeHandleString[MAX_VOL_STR_LEN] = { 0 };
                _sntprintf_s(volumeHandleString, MAX_VOL_STR_LEN, _TRUNCATE, TEXT("\\\\.\\%c:"), volumeLetter);
                volumeHandle = CreateFile(volumeHandleString, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
                if (INVALID_HANDLE_VALUE != volumeHandle)
                {
                    BOOL ioctlResult = FALSE, lockResult = FALSE;
                    lockResult = DeviceIoControl(volumeHandle, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL);
                    if (lockResult == FALSE)
                    {

                            _tprintf(TEXT("WARNING: Unable to lock volume: %s\n"), volumeHandleString);

                    }
                    ioctlResult = DeviceIoControl(volumeHandle, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL);
                    if (ioctlResult == FALSE)
                    {

                            _tprintf(TEXT("Error: Unable to dismount volume: %s\n"), volumeHandleString);

                        ret = FALSE;
                    }
                    if (lockResult == TRUE)
                    {
                        ioctlResult = DeviceIoControl(volumeHandle, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL);
                        if (ioctlResult == FALSE)
                        {

                                _tprintf(TEXT("WARNING: Unable to unlock volume: %s\n"), volumeHandleString);

                        }
                    }
                    CloseHandle(volumeHandle);
                    volumeHandle = INVALID_HANDLE_VALUE;
                }
            }
        }
    }
    return ret;
}

//TODO: We may need to switch between locking fd and scsiSrbHandle in some way...for now just locking fd value.
//https://docs.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-fsctl_lock_volume
BOOL os_Lock_Device(tScsiContext *scsiCtx)
{
    BOOL ret = TRUE;
    DWORD returnedBytes = 0;
    if (!DeviceIoControl(scsiCtx->hDevice, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &returnedBytes, NULL))
    {
        //This can fail is files are open, it's a system disk, or has a pagefile.
        ret = FALSE;
    }
    return ret;
}

BOOL os_Unlock_Device(tScsiContext *scsiCtx)
{
    BOOL ret = TRUE;
    DWORD returnedBytes = 0;
    if (!DeviceIoControl(scsiCtx->hDevice, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &returnedBytes, NULL))
    {
        ret = FALSE;
    }
    return ret;
}

 BOOL os_Update_File_System_Cache(tScsiContext *scsiCtx)
{
   BOOL ret = TRUE;
    DWORD returnedBytes = 0;
   if (!DeviceIoControl(scsiCtx->hDevice, IOCTL_DISK_UPDATE_PROPERTIES, NULL, 0, NULL, 0, &returnedBytes, NULL))
    {
        ret = FALSE;
    }
    return ret;
}


BOOL NvmeUtil::ScsiSanitizeCmd(tScsiContext *scsiCtx, eScsiSanitizeType sanitizeType, bool immediate, bool znr, bool ause, uint16_t parameterListLength, uint8_t *ptrData)
{
    BOOL       result       = FALSE;
    uint8_t   cdb[CDB_LEN_10]       = { 0 };
    eDataTransferDirection dataDir = XFER_NO_DATA;
    result = os_Lock_Device(scsiCtx);
    if (!result) {
        std::cout<<" Lock device failed" <<std::endl;
        return result;
    }

    result = os_Unmount_File_Systems_On_Device(scsiCtx);

    if (!result) {
        std::cout<<" Lock filesystem failed" <<std::endl;
        return result;
    }
    result       = FALSE;
    scsiCtx->senseDataSize = SPC3_SENSE_LEN;

    memset(scsiCtx->lastCommandSenseData, 0, scsiCtx->senseDataSize);

    cdb[SCSI_OPERATION_CODE] = SANITIZE_CMD;
    cdb[1] = sanitizeType & 0x1F;
    if (immediate) {
        cdb[1] |= M_BitN(7);
    }
    if (znr) {
        cdb[1] |= M_BitN(6);
    }
    if (ause)
    {
        cdb[1] |= M_BitN(5);
    }
    cdb[2] = RESERVED;
    cdb[3] = RESERVED;
    cdb[4] = RESERVED;
    cdb[5] = RESERVED;
    cdb[6] = RESERVED;
    //parameter list length
    cdb[7] = M_Byte(parameterListLength, 1);
    cdb[8] = M_Byte(parameterListLength, 0);

    switch (sanitizeType) {
    case SCSI_SANITIZE_OVERWRITE:
        dataDir = XFER_DATA_OUT;
        break;
    default:
        dataDir = XFER_NO_DATA;
        break;
    }

    result = scsiSendCdb(scsiCtx, &cdb[0], (eCDBLen)sizeof(cdb), ptrData, parameterListLength, dataDir, scsiCtx->lastCommandSenseData, scsiCtx->senseDataSize, 15);
    os_Update_File_System_Cache(scsiCtx);
    os_Unlock_Device(scsiCtx);

    PrintDataBuffer(scsiCtx->lastCommandSenseData, scsiCtx->senseDataSize);
    return result;
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

BOOL NvmeUtil::win10FW_TransferFile(HANDLE hHandle, PSTORAGE_HW_FIRMWARE_INFO fwdlInfo, BYTE slotId, LPCWSTR fwFileName, int64_t transize) {
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


BOOL NvmeUtil::Deallocate(HANDLE hHandle) {
    BOOL ret = FALSE;
    PVOID buffer = NULL;
    ULONG bufferLength = 0;
    ULONG returnedLength = 0;

    PDEVICE_MANAGE_DATA_SET_ATTRIBUTES pAttr = NULL;
    PDEVICE_DSM_RANGE pRange = NULL;
    tScsiContext scsiContext;
    scsiContext.hDevice = hHandle;
    os_Lock_Device(&scsiContext);
    os_Unmount_File_Systems_On_Device(&scsiContext);


    // Allocate buffer for use.
    bufferLength =
        sizeof(DEVICE_MANAGE_DATA_SET_ATTRIBUTES);
    buffer = malloc(bufferLength);

    if (buffer == NULL) {
        printf("Deallocate: allocate buffer failed.\n");
         os_Unlock_Device(&scsiContext);
        return FALSE;
    }

    // see also
    // https://docs.microsoft.com/ja-jp/windows/desktop/api/winioctl/ni-winioctl-ioctl_storage_manage_data_set_attributes
    // see also winioctl.h
    // https://github.com/RetroSoftwareRepository/FreeNT-0.4.8/blob/ed4eb9c7c337a19783f58a919da2efc1e26b0d85/drivers/filesystems/btrfs/balance.c#L2871
    ZeroMemory(buffer, bufferLength);
    pAttr = (PDEVICE_MANAGE_DATA_SET_ATTRIBUTES)buffer;

    pAttr->Action = DeviceDsmAction_Trim;
    pAttr->Flags =
        DEVICE_DSM_FLAG_ENTIRE_DATA_SET_RANGE | DEVICE_DSM_FLAG_TRIM_NOT_FS_ALLOCATED;
    pAttr->ParameterBlockOffset =
        0;  // TRIM does not need additional parameters
    pAttr->ParameterBlockLength = 0;
    pAttr->DataSetRangesOffset = 0;
        pAttr->DataSetRangesLength = 0;
#if 0
    pRange = (PDEVICE_DSM_RANGE)((ULONGLONG)pAttr +
                                  sizeof(DEVICE_MANAGE_DATA_SET_ATTRIBUTES));
    pRange->StartingOffset = 0;   // LBA = 0
    pRange->LengthInBytes = 512;  // 1 sector
#endif
    // Send request down (through WinFunc.cpp)
    ret = DeviceIoControl(
        hHandle,  // (HANDLE) hDevice             // handle to device
        IOCTL_STORAGE_MANAGE_DATA_SET_ATTRIBUTES,  // dwIoControlCode
        buffer,           // (LPVOID) lpInBuffer          // input buffer
        bufferLength,     // (DWORD) nInBufferSize        // size of the input
        // buffer
        buffer,           // (LPVOID) lpOutBuffer         // output buffer
        bufferLength,     // (DWORD) nOutBufferSize       // size of the output
        // buffer
        &returnedLength,  // (LPDWORD) lpBytesReturned    // number of bytes
        // returned
        NULL);  // (LPOVERLAPPED) lpOverlapped  // OVERLAPPED structure

    free(buffer);
 os_Unlock_Device(&scsiContext);
    os_Update_File_System_Cache(&scsiContext);
    return ret;
}


BOOL NvmeUtil::GetSanitizeStatusLog(HANDLE hDevice, uint8_t sanitizeStatusLog[512]) {
    BOOL    result = FALSE;
    PVOID   buffer = NULL;
    ULONG   bufferLength = 0;
    ULONG   returnedLength = 0;

    PSTORAGE_PROPERTY_QUERY query = NULL;
    PSTORAGE_PROTOCOL_SPECIFIC_DATA protocolData = NULL;
    PSTORAGE_PROTOCOL_DATA_DESCRIPTOR protocolDataDescr = NULL;


    // Allocate buffer for use.
    bufferLength = offsetof(STORAGE_PROPERTY_QUERY, AdditionalParameters)
                   + sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA)
                   + sizeof(sanitizeStatusLog);
    buffer = malloc(bufferLength);

    if (buffer == NULL) {
        printf("GetSanitizeStatusLog: allocate buffer failed.\n");
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
    protocolData->ProtocolDataRequestValue = NVME_LOG_PAGE_SANITIZE_STATUS;
    protocolData->ProtocolDataRequestSubValue = NVME_NAMESPACE_ALL;
    protocolData->ProtocolDataOffset = sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA);
    protocolData->ProtocolDataLength = sizeof(sanitizeStatusLog);

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
        printf("GetSanitizeStatusLog: Data Descriptor Header is not valid.\n");
        result = FALSE;
        goto exit;
    }

    protocolData = &protocolDataDescr->ProtocolSpecificData;

    if ((protocolData->ProtocolDataOffset < sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA)) ||
        (protocolData->ProtocolDataLength < sizeof(NVME_DEVICE_SELF_TEST_LOG))) {
        printf("GetSanitizeStatusLog: ProtocolData Offset/Length is not valid.\n");
        result = FALSE;
        goto exit;
    }

    memcpy(sanitizeStatusLog, (PCHAR)protocolData + protocolData->ProtocolDataOffset, sizeof(sanitizeStatusLog));

    result = TRUE;
exit:
    if (buffer != NULL)
    {
        free(buffer);
    }

    return result;
}
#define M_GETBITRANGE(input, msb, lsb) (((input) >> (lsb)) & ~(~UINT64_C(0) << ((msb) - (lsb) + 1)))

// Big endian parameter order, little endian value
    #define M_BytesTo2ByteValue(b1, b0)                            (        \
    static_cast<uint16_t> (  (static_cast<uint16_t>(b1) << 8) | (static_cast<uint16_t>(b0) <<  0)  )          \
                                                                   )

BOOL NvmeUtil::GetNVMESanitizeProgress(HANDLE hHandle, double *percentComplete, eSanitizeStatus *sanitizeStatus)
{
    BOOL result = TRUE;
    //read the sanitize status log
    uint8_t sanitizeStatusLog[512] = { 0 };


    //TODO: Set namespace ID?
    if (NvmeUtil::GetSanitizeStatusLog(hHandle, sanitizeStatusLog))
    {

        uint16_t sprog = M_BytesTo2ByteValue(sanitizeStatusLog[1], sanitizeStatusLog[0]);
        uint16_t sstat = M_BytesTo2ByteValue(sanitizeStatusLog[3], sanitizeStatusLog[2]);
        *percentComplete = sprog;

        switch(M_GETBITRANGE(sstat, 2, 0))
        {
        case 0:
            *sanitizeStatus = SANITIZE_STATUS_NEVER_SANITIZED;
            break;
        case 1:
            *sanitizeStatus = SANITIZE_STATUS_SUCCESS;
            break;
        case 2:
            *sanitizeStatus = SANITIZE_STATUS_IN_PROGRESS;
            break;
        case 3:
            *sanitizeStatus = SANITIZE_STATUS_FAILED;
            break;
        default:
            *sanitizeStatus = SANITIZE_STATUS_UNKNOWN;
            break;
        }
    }
    else
    {
        result = FALSE;
    }
    *percentComplete *= 100.0;
    *percentComplete /= 65536.0;
    return result;
}
