
static void ConvertSmartAttr(const sSMARTATTR& src, sSmartAttr& dst) {
    dst.reset();
    #define MAP_ITEM(d, s) dst.d = src.s
    MAP_ITEM(worst, Worst); MAP_ITEM(threshold, Threshold);
    MAP_ITEM(id, ID); MAP_ITEM(name, Name); MAP_ITEM(value, Value);
    MAP_ITEM(loraw, LowRaw); MAP_ITEM(hiraw, HighRaw); MAP_ITEM(note, Note);
    #undef MAP_ITEM
}

static void ConvertFeatures(const sFEATURE& src, sFeature& dst) {
    dst.reset();
    #define MAP_ITEM(d, s) dst.d = src.s
    MAP_ITEM(ataversion,  AtaVersion);
    MAP_ITEM(dma,  IsDmaSupported);
    MAP_ITEM(lbamode,  IsLba48Supported);
    MAP_ITEM(ncq,  IsNcqSupported);
    MAP_ITEM(provision,  IsOpSupported);
    MAP_ITEM(security,  IsSecuritySupported);
    // secure-erase is mandatory in security feature set
    MAP_ITEM(erase,  IsSecuritySupported);
    MAP_ITEM(smart,  IsSmartSupported);
    MAP_ITEM(test,  IsTestSupported);
    MAP_ITEM(trim,  IsTrimSupported);
    MAP_ITEM(dlcode,  IsDlCodeSupported);
    #undef MAP_ITEM
}

static void ConvertIdentify(const sIDENTIFY& src, sIdentify& dst) {
    dst.reset();
    #define MAP_ITEM(d, s) dst.d = src.s
    MAP_ITEM(cap, UserCapacity);
    MAP_ITEM(model, DeviceModel);
    MAP_ITEM(serial, SerialNumber);
    MAP_ITEM(version, FirmwareVersion);
    #undef MAP_ITEM

    ConvertFeatures(src.SectorInfo, dst.features);
}

static void UpdateDriveInfo(const sDRVINFO& src, sDriveInfo& dst) {

    dst.name = src.IdentifyInfo.DriveName;

    if (1) {
        ConvertIdentify(src.IdentifyInfo, dst.id);
    }

    if (1) {
        // Clone SMART info
        tAttrMap& dm = dst.si.amap; sSmartAttr attr;
        const tATTRMAP& sm = src.SmartInfo.AttrMap;
        for (tATTRMAPCITR it = sm.begin(); it != sm.end(); it++) {
            ConvertSmartAttr(it->second, attr); dm[it->first] = attr;
        }
    }

    if (1) {
        // Others
        sSmartAttr attr;
        if (GetSmartAttr(dst.si.amap, SMA_TEMPERATURE, attr)) {
            dst.temp = ((U64)attr.hiraw << 32) | attr.loraw;
        }

        if (GetSmartAttr(dst.si.amap, SMA_TOTAL_HOST_READ, attr)) {
            dst.tread = ((U64)attr.hiraw << 32) | attr.loraw;
        }

        if (GetSmartAttr(dst.si.amap, SMA_TOTAL_HOST_WRITTEN, attr)) {
            dst.twrtn = ((U64)attr.hiraw << 32) | attr.loraw;
        }
    }
}

static void UpdateAdapterInfo(const sAdapterInfo& src, sDriveInfo& dst) {
    dst.bustype = src.BusType;
    dst.maxtfsec = src.MaxTransferSector;
}

static void CloseDevice(sPHYDRVINFO& phy) {
    return DeviceMgr::CloseDevice(phy);
}

static eRetCode ScanDrive_SataBus(sPHYDRVINFO& phy, U32 index, bool rsm, sDriveInfo& di, volatile sProgress *p) {
    sDRVINFO drv;

    do {
        // --------------------------------------------------
        // Read Identify data

        AtaCmd cmd; cmd.setCommand(0, 1, CMD_IDENTIFY_DEVICE);
        if (!TRY_TO_EXECUTE_COMMAND(phy.DriveHandle, cmd)) SKIP_AND_CONTINUE(p,5)

        DeviceMgr::ParseIdentifyInfo(cmd.getBufferPtr(), phy.DriveName, drv.IdentifyInfo);
        drv.IdentifyInfo.DriveIndex = index;
        UPDATE_AND_RETURN_IF_STOP(p, 1, CloseDevice, phy, RET_ABORTED);

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

        UpdateDriveInfo(drv, di);

        return RET_OK;
    } while(0);

    return RET_FAIL;
}

