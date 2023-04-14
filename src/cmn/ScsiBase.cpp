#include "AtaBase.h"
#include "ScsiBase.h"
using namespace std;

string ScsiBase::toString() const
{
    string cmdString = "UnknownCommand";

    switch (CmdCode)
    {
        #define MAP_ITEM(code, val, data, mode) case (U32) code: cmdString = #code; break;
        #include "ScsiCode.def"
        #undef MAP_ITEM
    }

    return cmdString;
}

void ScsiBase::buildScsiCmdBlock(U8 *cmdb)
{
    // ------------------------------------------------------
    // Initialize cmdb
    // ------------------------------------------------------
    cmdb[0] = CmdCode & 0xFF;

    switch (CmdCode)
    {
        case SCSI_READ_10: buildCmdBlock_Read10(cmdb); break;
        case SCSI_WRITE_10: buildCmdBlock_Write10(cmdb); break;
    }
}

void ScsiBase::buildSatCmdBlock(U8 *sat)
{
    // ------------------------------------------------------
    // Initialize sat
    // ------------------------------------------------------
    sat[ 0] = 0x85;                      // SCSI command code

    // Build sat[1]: MULTIPLE_COUNT | pROTOCOL | EXTEND
    U8 transferMode = 0;
    if (false == hasTransfer()) transferMode = PROTOCOL_NO_DATA;
    else {
        if (true == isDmaMode()) transferMode = PROTOCOL_DMA;
        else {
            if (true == isDataIn()) transferMode = PROTOCOL_PIO_IN;
            if (true == isDataOut()) transferMode = PROTOCOL_PIO_OUT;
        }
    }

    sat[ 1] |= transferMode;

    if(true == hasTransfer())
    {
        sat[2] |= TRANSFER_BLOCK;
        sat[2] |= TLENGTH_IN_SECTOR;
        sat[2] |= isDataIn() ? TRANSFER_FROM_DEV : TRANSFER_TO_DEV;
    }
    else
    {
        sat[2] = CHECK_CONDITION;
    }

    sat[ 3] = 0;                         // Feature
    sat[ 4] = getFeatureCode();

    sat[ 5] = 0;                         // Sector Count
    sat[ 6] = SecCount & 0xFF;

    sat[ 7] = 0;                         // LBA (7 : 12)
    sat[ 8] = LBA.lo & 0xFF;

    sat[ 9] = 0;
    sat[10] = (LBA.lo >>  8) & 0xFF;

    sat[11] = 0;
    sat[12] = (LBA.lo >> 16) & 0xFF;

    sat[13] = (LBA.lo >> 24) & 0xF ;     // Device
    sat[14] = CmdCode & 0xFF;            // CommandCode

    // ------------------------------------------------------
    // Update for specific commands
    // ------------------------------------------------------
    switch (CmdCode)
    {
    case CMD_SMART_READ_DATA:
    case CMD_SMART_READ_THRESHOLD:
        sat[10] = 0x4F;
        sat[12] = 0xC2;
        break;

    case CMD_READ_SECTOR: sat[13] |= 0x40; break;
    case CMD_WRITE_SECTOR: sat[13] |= 0x40; break;

    case CMD_READ_DMA: sat[13] |= 0x40; break;
    case CMD_WRITE_DMA: sat[13] |= 0x40; break;

    default: break;
    }
}

void ScsiBase::buildCmdBlock_Read10(U8 *cmdb)
{
    cmdb[0] = CmdCode & 0xFF;
    cmdb[2] = (LBA.lo >> 24) & 0xFF;
    cmdb[3] = (LBA.lo >> 16) & 0xFF;
    cmdb[4] = (LBA.lo >>  8) & 0xFF;
    cmdb[5] = (LBA.lo >>  0) & 0xFF;
    cmdb[7] = 0;
    cmdb[8] = SecCount;
}

void ScsiBase::buildCmdBlock_Write10(U8 *cmdb)
{
    cmdb[0] = CmdCode & 0xFF;
    cmdb[2] = (LBA.lo >> 24) & 0xFF;
    cmdb[3] = (LBA.lo >> 16) & 0xFF;
    cmdb[4] = (LBA.lo >>  8) & 0xFF;
    cmdb[5] = (LBA.lo >>  0) & 0xFF;
    cmdb[7] = 0;
    cmdb[8] = SecCount;
}
