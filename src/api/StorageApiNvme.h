
static eRetCode ScanDrive_NvmeBus(sPHYDRVINFO& phy, U32 index, bool rsm, sDriveInfo& di, volatile sProgress *p) {
    std::stringstream sstr;
    sstr << *const_cast<std::string*>(&p->info) << std::endl;
    sstr << "Scanning device " << index << "  on NvmeBus";
    *const_cast<std::string*>(&p->info) = sstr.str();
    UPDATE_PROGRESS(p, 6);
    return RET_NOT_SUPPORT;
}
