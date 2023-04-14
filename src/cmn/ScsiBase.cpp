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
