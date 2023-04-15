#ifndef StorageApiUsb_H
#define StorageApiUsb_H

#include "ApiUtil.h"
#include "ScsiCmd.h"
#include "DeviceMgr.h"
#include "StorageApiCmn.h"

// For USB device, use Scsi command with ATA code (aka SAT)
static eRetCode ScanDrive_UsbBus(sPHYDRVINFO& phy, U32 index, bool rsm, sDriveInfo& di, volatile sProgress *p) {
    sDRVINFO drv;

    do {
        // --------------------------------------------------
        // Read Identify data

        ScsiCmd cmd; cmd.setCommand(0, 1, CMD_IDENTIFY_DEVICE);
        if (!TRY_TO_EXECUTE_COMMAND(phy.DriveHandle, cmd)) SKIP_AND_CONTINUE(p,5)

        DeviceMgr::ParseIdentifyInfo(cmd.getBufferPtr(), phy.DriveName, drv.IdentifyInfo);
        drv.IdentifyInfo.DriveIndex = index;
        UPDATE_AND_RETURN_IF_STOP(p, 1, CloseDevice, phy, RET_ABORTED);

        // --------------------------------------------------
        // Testing for skipped drives

        // --------------------------------------------------
        // Testing for skipped drives

        // --------------------------------------------------
        // Read SMART data
        if (!rsm) UPDATE_PROGRESS(p, 4);
        else
        {
            U8 vbuf[SECTOR_SIZE]; U8 tbuf[SECTOR_SIZE];
            cmd.setCommand(0, 1, CMD_SMART_READ_DATA);
            if (!TRY_TO_EXECUTE_COMMAND(phy.DriveHandle, cmd)) SKIP_AND_CONTINUE(p,4)
            memcpy(vbuf, cmd.getBufferPtr(), cmd.SecCount * SECTOR_SIZE);
            UPDATE_AND_RETURN_IF_STOP(p, 1, CloseDevice, phy, RET_ABORTED);

            cmd.setCommand(0, 1, CMD_SMART_READ_THRESHOLD);
            if (!TRY_TO_EXECUTE_COMMAND(phy.DriveHandle, cmd)) SKIP_AND_CONTINUE(p,3)
            memcpy(tbuf, cmd.getBufferPtr(), cmd.SecCount * SECTOR_SIZE);
            UPDATE_AND_RETURN_IF_STOP(p, 1, CloseDevice, phy, RET_ABORTED);

            DeviceMgr::ParseSmartData(vbuf, tbuf, drv.SmartInfo);
            UPDATE_AND_RETURN_IF_STOP(p, 1, CloseDevice, phy, RET_ABORTED);
        }

        ApiUtil::UpdateDriveInfo(drv, di);

        return RET_OK;
    } while(0);

    return RET_FAIL;
}

// For USB device, use Scsi command with ATA code (aka SAT)
static eRetCode Read_UsbBus(HDL handle, U64 lba, U32 count, U8 *buffer, volatile sProgress *p) {
    U64 bufsize = count * SECTOR_SIZE;
    ScsiCmd cmd; cmd.setCommand(lba, count, CMD_READ_DMA); // able to read 48b ?
    if (!TRY_TO_EXECUTE_COMMAND(handle, cmd)) {
        memset(buffer, 0x00, bufsize); return RET_FAIL;
    }
    memcpy(buffer, cmd.getBufferPtr(), cmd.SecCount * SECTOR_SIZE);
    return RET_OK;
}

#endif // StorageApiUsb_H
