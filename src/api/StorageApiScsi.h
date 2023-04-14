
static eRetCode ScanDrive_ScsiBus(sPHYDRVINFO& phy, U32 index, bool rsm, sDriveInfo& di, volatile sProgress *p) {
    std::stringstream sstr;
    sstr << "Scanning device " << index << " on ScsiBus";
    AppendLog(p, sstr.str());

    UPDATE_PROGRESS(p, 6);
    return RET_NOT_SUPPORT;
}
