// Windows implementation

#include "AtaCmd.h"

#include "windows.h"
#include "ntddscsi.h"
using namespace std;

#define SYSTEM_DATA sHeaderData
#define SYSTEM_CODE IOCTL_SCSI_PASS_THROUGH

struct sHeaderData
{
    SCSI_PASS_THROUGH Spt;
    U8 Filler[4];
    U8 Sense[64];
};

void AtaCmdScsi::buildBasicCommand()
{
    SYSTEM_DATA& p = *(SYSTEM_DATA*)(void*)getStructPtr();
    SCSI_PASS_THROUGH& spt = p.Spt;
    spt.Length = sizeof (p.Spt);
    spt.ScsiStatus = 0;
    spt.CdbLength = sizeof (spt.Cdb);
    spt.SenseInfoLength = sizeof (p.Sense);
    spt.SenseInfoOffset = sizeof (p.Spt) + sizeof (p.Filler);
    spt.DataIn = SCSI_IOCTL_DATA_UNSPECIFIED;
    spt.DataTransferLength = BufferSize;
    spt.DataBufferOffset = StructSize + FillerSize;
    spt.TimeOutValue = 30;
    if (true == isDataIn()) spt.DataIn = SCSI_IOCTL_DATA_IN;
    if (true == isDataOut()) spt.DataIn = SCSI_IOCTL_DATA_OUT;
    buildScsiCmdBlock16(spt.Cdb);
}

bool AtaCmdScsi::buildCommand()
{
    buildBasicCommand();

    // Additional update here

    return true;
}

eCMDERR AtaCmdScsi::getErrorStatus()
{
    return CMD_ERROR_NONE;
}

void AtaCmdScsi::setCommand(sADDRESS lba, U32 sectorCount, eATACODE cmdCode)
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

bool AtaCmdScsi::executeCommand(tHdl handle)
{
    HANDLE deviceHandle = (HANDLE) handle; // In Windows, HANDLE == pvoid

    U32 inputBufferSize = StructSize + FillerSize + (isDataOut() ? BufferSize : 0);
    // U32 inputBufferSize = StructSize + FillerSize + BufferSize;
    U32 outputBufferSize = StructSize + FillerSize + BufferSize;

    DWORD byteReturn = 0;
    return DeviceIoControl(
        deviceHandle,
        SYSTEM_CODE,
        getStructPtr(), inputBufferSize,
        getStructPtr(), outputBufferSize,
        &byteReturn,
        NULL);
}
