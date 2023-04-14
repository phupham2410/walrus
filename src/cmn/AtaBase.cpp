#include "AtaBase.h"
using namespace std;

#define PROTOCOL_NO_DATA 0x06
#define PROTOCOL_PIO_IN  0x08
#define PROTOCOL_PIO_OUT 0x0A
#define PROTOCOL_DMA     0x0C

#define TLENGTH_NO_DATA    0x00
#define TLENGTH_IN_FEATURE 0x01
#define TLENGTH_IN_SECTOR  0x02
#define TLENGTH_IN_TPSIU   0x03

#define TRANSFER_BYTE      (0 << 2)
#define TRANSFER_BLOCK     (1 << 2)

#define TRANSFER_TO_DEV    (0 << 3)
#define TRANSFER_FROM_DEV  (1 << 3)

#define CHECK_CONDITION    (1 << 5)

string AtaBase::toString() const
{
    string cmdString = "UnknownCommand";

    switch (CmdCode)
    {
        #define MAP_ITEM(code, val, data, mode, cmd) case (U32) code: cmdString = #code; break;
        #include "AtaCode.def"
        #undef MAP_ITEM
    }

    return cmdString;
}

static void buildTskFile_48bCmd(U8* curr, U8* prev, U64 lba, U16 sc, U32 code) {
    U16 ftr = (code >> 8) & 0xFF;
    U16 cmd = (code     ) & 0xFF;

    // ---------------------------------------------------------
    curr[0] = (U8)(ftr      )&0xFF;
    curr[1] = (U8)(sc       )&0xFF;
    curr[2] = (U8)(lba      )&0xFF;
    curr[3] = (U8)(lba >>  8)&0xFF;
    curr[4] = (U8)(lba >> 16)&0xFF;
    curr[5] = 0xE0;
    curr[6] = (U8)(cmd      )&0xFF;
    curr[7] = 0;
    // ---------------------------------------------------------
    prev[0] = (U8)(ftr >>  8)&0xFF;
    prev[1] = (U8)(sc  >>  8)&0xFF;
    prev[2] = (U8)(lba >> 24)&0xFF;
    prev[3] = (U8)(lba >> 32)&0xFF;
    prev[4] = (U8)(lba >> 40)&0xFF;
    prev[5] = 0xE0;                 // Device
    prev[6] = (U8)(cmd      )&0xFF; // Command
    prev[7] = 0;
    // ---------------------------------------------------------
}

void AtaBase::buildTskFile(U8* task, U8* prev)
{
    // ------------------------------------------------------
    // Initialize TaskFile
    // ------------------------------------------------------
    task[0] = (CmdCode >> 8) & 0xFF; // Features, Error
    task[1] = SecCount;              // Count
    task[2] = LBA.lo & 0xFF;
    task[3] = (LBA.lo >>  8) & 0xFF;
    task[4] = (LBA.lo >> 16) & 0xFF;
    task[5] = 0;                     // Device
    task[6] = CmdCode & 0xFF;        // Command, Status

    // ------------------------------------------------------
    // Update for specific commands
    // ------------------------------------------------------
    switch (CmdCode)
    {
    case CMD_SMART_READ_DATA:
    case CMD_SMART_READ_THRESHOLD:
        task[3] |= 0x4F; // LBA Mid
        task[4] |= 0xC2; // LBA High
        break;

    case CMD_READ_SECTOR: task[5] |= 0x40; break;
    case CMD_WRITE_SECTOR: task[5] |= 0x40; break;

    case CMD_READ_DMA: task[5] |= 0x40; break;
    case CMD_WRITE_DMA: task[5] |= 0x40; break;

    case CMD_READ_DMA_48B:
    case CMD_READ_SECTOR_48B:
        buildTskFile_48bCmd(task, prev, LBA.toU64(), SecCount, CmdCode); break;

    default: break;
    }
}

void AtaBase::buildScsiCmdBlock16(U8 *cmdb)
{
    // ------------------------------------------------------
    // Initialize cmdb
    // ------------------------------------------------------
    cmdb[0] = 0x85;                      // SCSI command code

    // Build cmdb[1]: MULTIPLE_COUNT | pROTOCOL | EXTEND
    U8 transferMode = 0;
    if (false == hasTransfer()) transferMode = PROTOCOL_NO_DATA;
    else {
        if (true == isDmaMode()) transferMode = PROTOCOL_DMA;
        else {
            if (true == isDataIn()) transferMode = PROTOCOL_PIO_IN;
            if (true == isDataOut()) transferMode = PROTOCOL_PIO_OUT;
        }
    }

    cmdb[ 1] |= transferMode;

    if(true == hasTransfer())
    {
        cmdb[2] |= TRANSFER_BLOCK;
        cmdb[2] |= TLENGTH_IN_SECTOR;
        cmdb[2] |= isDataIn() ? TRANSFER_FROM_DEV : TRANSFER_TO_DEV;
    }
    else
    {
        cmdb[2] = CHECK_CONDITION;
    }

    cmdb[ 3] = 0;                         // Feature
    cmdb[ 4] = (CmdCode >> 8) & 0xFF;

    cmdb[ 5] = 0;                         // Sector Count
    cmdb[ 6] = SecCount;

    cmdb[ 7] = 0;                         // LBA (7 : 12)
    cmdb[ 8] = LBA.lo & 0xFF;

    cmdb[ 9] = 0;
    cmdb[10] = (LBA.lo >>  8) & 0xFF;

    cmdb[11] = 0;
    cmdb[12] = (LBA.lo >> 16) & 0xFF;

    cmdb[13] = 0;                          // Device
    cmdb[14] = CmdCode & 0xFF;         // CommandCode

    // ------------------------------------------------------
    // Update for specific commands
    // ------------------------------------------------------
    switch (CmdCode)
    {
    case CMD_SMART_READ_DATA:
    case CMD_SMART_READ_THRESHOLD:
        cmdb[10] = 0x4F;
        cmdb[12] = 0xC2;
        break;

    case CMD_READ_SECTOR: cmdb[13] |= 0x40; break;
    case CMD_WRITE_SECTOR: cmdb[13] |= 0x40; break;

    case CMD_READ_DMA: cmdb[13] |= 0x40; break;
    case CMD_WRITE_DMA: cmdb[13] |= 0x40; break;

    default: break;
    }
}

void AtaBase::buildScsiCmdBlock12(U8 *cmdb)
{
    // ------------------------------------------------------
    // Initialize cmdb
    // ------------------------------------------------------
    cmdb[ 0] = 0xA1;                      // SCSI command code

    // Build cmdb[1]: MULTIPLE_COUNT | pROTOCOL | EXTEND
    U8 protocol = 0;
    if (false == hasTransfer()) protocol = PROTOCOL_NO_DATA;
    else {
        if (true == isDmaMode()) protocol = PROTOCOL_DMA;
        else {
            if (true == isDataIn()) protocol = PROTOCOL_PIO_IN;
            if (true == isDataOut()) protocol = PROTOCOL_PIO_OUT;
        }
    }

    cmdb[ 1] |= protocol;

    if(true == hasTransfer())
    {
        cmdb[2] |= TRANSFER_BLOCK;
        cmdb[2] |= TLENGTH_IN_SECTOR;
        cmdb[2] |= isDataIn() ? TRANSFER_FROM_DEV : TRANSFER_TO_DEV;
    }
    else
    {
        cmdb[2] = CHECK_CONDITION;
    }

    cmdb[3] = (CmdCode >> 8) & 0xFF; // Features
    cmdb[4] = SecCount;               // SecCount
    cmdb[5] = LBA.lo & 0xFF;
    cmdb[6] = (LBA.lo >>  8) & 0xFF;
    cmdb[7] = (LBA.lo >> 16) & 0xFF;
    cmdb[8] = 0;                          // Device
    cmdb[9] = CmdCode & 0xFF;         // CommandCode

    // ------------------------------------------------------
    // Update for specific commands
    // ------------------------------------------------------
    switch (CmdCode)
    {
    case CMD_SMART_READ_DATA:
    case CMD_SMART_READ_THRESHOLD:
        cmdb[6] = 0x4F;
        cmdb[7] = 0xC2;
        break;

    case CMD_READ_SECTOR: cmdb[8] |= 0x40; break;
    case CMD_WRITE_SECTOR: cmdb[8] |= 0x40; break;

    case CMD_READ_DMA: cmdb[8] |= 0x40; break;
    case CMD_WRITE_DMA: cmdb[8] |= 0x40; break;

    default: break;
    }
}
