#include "AtaCmd.h"

#include "windows.h"
#include "ntddscsi.h"
using namespace std;

#define SYSTEM_DATA ATA_PASS_THROUGH_DIRECT
#define SYSTEM_CODE IOCTL_ATA_PASS_THROUGH_DIRECT

void AtaCmdDirect::buildBasicCommand()
{
    SYSTEM_DATA& p = *(SYSTEM_DATA*)(void*)getStructPtr();

    p.Length = StructSize;
    p.TimeOutValue = 30;
    p.DataTransferLength = BufferSize;
    p.DataBuffer = Buffer + (StructSize + FillerSize);

    p.AtaFlags |= ATA_FLAGS_DRDY_REQUIRED;
    if (true == isDmaMode()) p.AtaFlags |= ATA_FLAGS_USE_DMA;
    if (true == isDataIn()) p.AtaFlags |= ATA_FLAGS_DATA_IN;
    if (true == isDataOut()) p.AtaFlags |= ATA_FLAGS_DATA_OUT;

    buildTskFile(p.CurrentTaskFile);
}

bool AtaCmdDirect::buildCommand()
{
    buildBasicCommand();

    // Additional update here

    return true;
}

eCMDERR AtaCmdDirect::getErrorStatus()
{
    #define ACC_ERROR(e, bit, code) ((e & bit) ? code : CMD_ERROR_NONE);

    const SYSTEM_DATA& p = *(SYSTEM_DATA*)(void*) getStructPtr();
    const U8* t = p.CurrentTaskFile;

    if (0 == (t[6] & 0x1)) return CMD_ERROR_NONE;            // Error bit is zero
    if ((CmdCode & 0xFF) == t[6]) return CMD_ERROR_EXEC; // CommandCode not changed

    U8 errorByte = t[0];
    eCMDERR errorCode = CMD_ERROR_NONE;

    switch (CmdCode)
    {
    // Security Feature. ErrorCode: Abort | InterfaceCRC
    case CMD_DISABLE_PASSWORD    :
    case CMD_SET_PASSWORD        :
    case CMD_ERASE_PREPARE       :
    case CMD_ERASE_UNIT          :
    case CMD_UNLOCK              :
    case CMD_FREEZE_LOCK         :
        if (CMD_ERROR_NONE == errorCode) errorCode = ACC_ERROR(errorByte, 0x0004, CMD_ERROR_ABORT);
        if (CMD_ERROR_NONE == errorCode) errorCode = ACC_ERROR(errorByte, 0x0080, CMD_ERROR_ICRC );
        break;

    // SMART feature. ErrorCode:
    case CMD_SMART_READ_DATA:
    case CMD_SMART_READ_THRESHOLD:
        if (CMD_ERROR_NONE == errorCode) errorCode = ACC_ERROR(errorByte, 0x0004, CMD_ERROR_ABORT);
        if (CMD_ERROR_NONE == errorCode) errorCode = ACC_ERROR(errorByte, 0x0010, CMD_ERROR_IDNF );
        if (CMD_ERROR_NONE == errorCode) errorCode = ACC_ERROR(errorByte, 0x0040, CMD_ERROR_UNC  );
        if (CMD_ERROR_NONE == errorCode) errorCode = ACC_ERROR(errorByte, 0x0080, CMD_ERROR_ICRC );
        break;

    // General FeatureSet
    case CMD_IDENTIFY_DEVICE:
        break;

    default:
        ASSERT(0); break;
    }

    return errorCode;
}

void AtaCmdDirect::setCommand(sADDRESS lba, U32 sectorCount, eATACODE cmdCode)
{
    LBA = lba;
    SecCount = sectorCount;
    CmdCode = cmdCode;

    StructSize = sizeof (SYSTEM_DATA);
    FillerSize = 0;
    BufferSize = SecCount * 512;

    allocBuffer();
    buildCommand();
}

bool AtaCmdDirect::executeCommand(tHdl handle)
{
    HANDLE deviceHandle = (HANDLE) handle; // In Windows, HANDLE == pvoid

    U32 inputBufferSize = StructSize;
    U32 outputBufferSize = StructSize;

    DWORD byteReturn = 0;
    return DeviceIoControl(
        deviceHandle,
        SYSTEM_CODE,
        getStructPtr(), inputBufferSize,
        getStructPtr(), outputBufferSize,
        &byteReturn,
        NULL);
}
