#include "NvmeUtil.h"
#include <stdlib.h>
#include <stddef.h>
#include <winioctl.h>
#include <ntddscsi.h>
#include<winerror.h>
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
    sptdioDB->scsiPassthrough.ScsiStatus = 255;
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
        ret = sendSCSIPassThrough_EX(scsiCtx);
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


BOOL NvmeUtil::ScsiSanitizeCmd(tScsiContext *scsiCtx, eScsiSanitizeType sanitizeType, bool immediate, bool znr, bool ause, uint16_t parameterListLength, uint8_t *ptrData)
{
    BOOL       result       = FALSE;
    uint8_t   cdb[CDB_LEN_10]       = { 0 };
    eDataTransferDirection dataDir = XFER_NO_DATA;

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

    return result;
}



BOOL NvmeUtil::win10FW_GetfwdlInfoQuery(HANDLE hHandle,PSTORAGE_HW_FIRMWARE_INFO fwdlInfo,  BOOL isNvme)
{
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
        for (uint8_t iter = 0; iter < fwdlSupportedInfo->SlotCount && iter < slotCount; ++iter)
        {
            printf("\t    Firmware Slot %d:\n", fwdlSupportedInfo->Slot[iter].SlotNumber);
            printf("\t\tRead Only: %d\n", fwdlSupportedInfo->Slot[iter].ReadOnly);
            printf("\t\tRevision: %s\n", fwdlSupportedInfo->Slot[iter].Revision);
        }
        memcpy(fwdlInfo, fwdlSupportedInfo, sizeof(STORAGE_HW_FIRMWARE_INFO));
        ret = TRUE;
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

