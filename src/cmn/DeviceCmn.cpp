#include "DeviceMgr.h"
#include "CoreData.h"
#include "AtaCmd.h"
#include "CommonUtil.h"
#include "StringUtil.h"

using namespace std;

void DeviceMgr::CloseDevice(sPHYDRVINFO &drive)
{
    CloseDevice(drive.DriveHandle);
}

void DeviceMgr::CloseDevice(tPHYDRVARRAY &driveList)
{
    int driveCount = driveList.size();
    for (int i = 0; i < driveCount; i++)
    {
        CloseDevice(driveList[i].DriveHandle);
    }
}

eSCANERROR DeviceMgr::ScanDriveCommon(list<sDRVINFO>& infoList, bool readSMART, const tSERIALSET& serialSet)
{
    infoList.clear();

    tPHYDRVARRAY driveList;
    OpenDevice(driveList);

    int driveCount = driveList.size();
    for (int i = 0; i < driveCount; i++)
    {
        sDRVINFO driveInfo;
        sPHYDRVINFO& physDrive = driveList[i];

        unsigned char valBuffer[512];
        unsigned char thrBuffer[512];

        AtaCmd cmd;

        do {
            cmd.setCommand(0, 1, CMD_IDENTIFY_DEVICE);
            if (false == cmd.executeCommand((int) physDrive.DriveHandle)) break;
            if (CMD_ERROR_NONE != cmd.getErrorStatus()) break;

            ParseIdentifyInfo(cmd.getBufferPtr(), physDrive.DriveName, driveInfo.IdentifyInfo);
            driveInfo.IdentifyInfo.DriveIndex = i;

            if (false == IsValidSerial(serialSet, driveInfo.IdentifyInfo.SerialNumber)) break;

            if(true == readSMART)
            {
                cmd.setCommand(0, 1, CMD_SMART_READ_DATA);
                if (false == cmd.executeCommand((int) physDrive.DriveHandle)) break;
                if (CMD_ERROR_NONE != cmd.getErrorStatus()) break;

                memcpy(valBuffer, cmd.getBufferPtr(), cmd.SecCount * 512);

                cmd.setCommand(0, 1, CMD_SMART_READ_THRESHOLD);
                if (false == cmd.executeCommand((int) physDrive.DriveHandle)) break;
                if (CMD_ERROR_NONE != cmd.getErrorStatus()) break;

                memcpy(thrBuffer, cmd.getBufferPtr(), cmd.SecCount * 512);

                ParseSmartData(valBuffer, thrBuffer, driveInfo.SmartInfo);
            }

            infoList.push_back(driveInfo);

        } while(0);
    }

    CloseDevice(driveList);

    return (infoList.size() != 0) ? SCAN_ERROR_NONE :
                ((driveCount != 0) ? SCAN_ERROR_SKIPDRIVE : SCAN_ERROR_PERMISSION);
}

bool DeviceMgr::ReadIdentifyDataCommon(const string &deviceName, U8 *identifyBuffer)
{
    int handle;
    AtaCmd cmd;
    bool status = false;

    do {
        if (false == OpenDevice(deviceName, handle)) break;

        cmd.setCommand(0, 1, CMD_IDENTIFY_DEVICE);
        bool cmdStatus = cmd.executeCommand(handle);

        CloseDevice(handle);

        if (false == cmdStatus) break;

        memcpy(identifyBuffer, cmd.getBufferPtr(), cmd.SecCount * 512);

        status = true;
    } while(0);

    return status;
}

eSECSTATE DeviceMgr::ReadSecurrityState(const sFEATURE& info)
{
    eSECSTATE state = SECURITY_STATE_INVALID;

    do {
        if (false == info.IsSecuritySupported) { state = SECURITY_STATE_NOTSUPORTED; break; }

        if (!info.IsUserPasswordPresent &&
            !info.IsDeviceLocked &&
            !info.IsDeviceFrozen &&
            !info.IsMasterPasswordMaximum) { state = SECURITY_STATE_1; break; }

        if (!info.IsUserPasswordPresent &&
            !info.IsDeviceLocked &&
             info.IsDeviceFrozen) { state = SECURITY_STATE_2; break; }

        if ( info.IsUserPasswordPresent &&
             info.IsDeviceLocked &&
            !info.IsDeviceFrozen) { state = SECURITY_STATE_4; break; }

        if ( info.IsUserPasswordPresent &&
            !info.IsDeviceLocked &&
            !info.IsDeviceFrozen) { state = SECURITY_STATE_5; break; }

        if ( info.IsUserPasswordPresent &&
            !info.IsDeviceLocked &&
             info.IsDeviceFrozen) { state = SECURITY_STATE_6; break; }

    } while(0);

    return state;
}

eSECSTATE DeviceMgr::ReadSecurrityState(const string &deviceName)
{
    eSECSTATE state = SECURITY_STATE_INVALID;

    // Read the Identify buffer
    U8 identifyBuffer[512];

    if (true == ReadIdentifyData(deviceName, identifyBuffer))
    {
        state = ReadSecurrityState(identifyBuffer);
    }

    return state;
}

eSECSTATE DeviceMgr::ReadSecurrityState(const U8 *identifyBuffer)
{
    eSECSTATE state = SECURITY_STATE_INVALID;

    do {
        if (NULL == identifyBuffer) break;

        U16 w82 = GET_WORD(identifyBuffer, 82);
        U16 w85 = GET_WORD(identifyBuffer, 85);
        U16 w128 = GET_WORD(identifyBuffer, 128);

        // Testing for SecurityFeatureSet w82.b1 == 1
        if (0 == GET_BIT(w82, 1)) { state = SECURITY_STATE_NOTSUPORTED; break; }

        // Consistent to values in w82
        if (GET_BIT(w128, 0) != GET_BIT(w82, 1)) break;
        if (GET_BIT(w128, 1) != GET_BIT(w85, 1)) break;

        bool activeUser   = (0 != GET_BIT(w85,  1)); // bit 1
        bool deviceLocked = (0 != GET_BIT(w128, 2)); // bit 2
        bool deviceFrozen = (0 != GET_BIT(w128, 3)); // bit 3
        bool maxMaster    = (0 != GET_BIT(w128, 8)); // bit 8

        if (!activeUser && !deviceLocked && !deviceFrozen && !maxMaster) { state = SECURITY_STATE_1; break; }
        if (!activeUser && !deviceLocked &&  deviceFrozen              ) { state = SECURITY_STATE_2; break; }
        if ( activeUser &&  deviceLocked && !deviceFrozen              ) { state = SECURITY_STATE_4; break; }
        if ( activeUser && !deviceLocked && !deviceFrozen              ) { state = SECURITY_STATE_5; break; }
        if ( activeUser && !deviceLocked &&  deviceFrozen              ) { state = SECURITY_STATE_6; break; }

    } while(0);

    return state;
}

bool DeviceMgr::TestPasswordState(eATACODE code)
{
    bool requestPassword = false;

    switch(code)
    {
        case CMD_UNLOCK:
        case CMD_ERASE_UNIT:
        case CMD_SET_PASSWORD:
        case CMD_DISABLE_PASSWORD: requestPassword = true; break;

        case CMD_FREEZE_LOCK:
        case CMD_ERASE_PREPARE:    requestPassword = false; break;

        default: break;
    }

    return requestPassword;
}

bool DeviceMgr::TestSecurityState(eSECSTATE state, eATACODE code, string &errorString)
{
    unsigned int validState = 0;

    // See Table 8, ATA Spec

    switch(code)
    {
        case CMD_DISABLE_PASSWORD: validState |= SECURITY_STATE_1 | SECURITY_STATE_5; break;
        case CMD_ERASE_PREPARE:    validState |= SECURITY_STATE_4 | SECURITY_STATE_1 | SECURITY_STATE_5; break;
        case CMD_ERASE_UNIT:       validState |= SECURITY_STATE_4 | SECURITY_STATE_1 | SECURITY_STATE_5; break;
        case CMD_FREEZE_LOCK:      validState |= SECURITY_STATE_1 | SECURITY_STATE_5 | SECURITY_STATE_2 | SECURITY_STATE_6; break;
        case CMD_SET_PASSWORD:     validState |= SECURITY_STATE_1 | SECURITY_STATE_5; break;
        case CMD_UNLOCK:           validState |= SECURITY_STATE_4 | SECURITY_STATE_1 | SECURITY_STATE_5; break;
        default: break;
    }

    if (state & validState) return true;

    switch (state)
    {
        case SECURITY_STATE_4: errorString = "Drive is locked. Unlock drive first and try again"; break;
        case SECURITY_STATE_2:
        case SECURITY_STATE_6: errorString = "Drive is frozen. Unfrozen drive first and try again"; break;
        default: break;
    }

    return false;
}

bool DeviceMgr::IsValidSerial(const tSERIALSET& serialSet, const string& serial)
{
    return (0 == serialSet.size()) || (serialSet.end() != serialSet.find(serial));
}

void DeviceMgr::Message(eSCANERROR scanError)
{
    string sErrorMessage[] = {
        "",
        "Some valid drives are skipped.\nPlease check Setting::Select Device",
        "Please run this program as administrator",
    };

    if (SCAN_ERROR_NONE != scanError)
        CommonUtil::Message(sErrorMessage[scanError]);
}

void DeviceMgr::CopyWord(unsigned char* pdst0, const unsigned char* src, int wordcount, bool byteswap)
{
    unsigned char* pdst1 = pdst0 + 1;
    const unsigned char* psrc0 = src + (byteswap ? 1 : 0);
    const unsigned char* psrc1 = src + (byteswap ? 0 : 1);
    for (int i = 0; i < wordcount; *pdst0 = *psrc0, *pdst1 = *psrc1, pdst0+=2, psrc0+=2, pdst1+=2, psrc1+=2, ++i);
}

// convert 512byte-sector to string for testing
string DeviceMgr::ToAsciiString(const unsigned char* buff)
{
    stringstream sstr;

    int rowCount = 32;
    int colCount = 512 / rowCount;

    for (int i = 0; i < rowCount; i++)
    {
        stringstream textstr;
        int index = i * colCount;

        sstr << hex << uppercase << setw(4) << setfill('0') << i << " | ";

        for (int j = 0; j < colCount; j++)
        {
            // int k = index + j + 1;
            // k = (k % 2) ? k : k - 2;
            int k = index + j;

            unsigned char val = buff[k];
            sstr << hex << uppercase << setw(2) << setfill('0') << (int) val << " ";

            char ascVal = ((val >= 32) && (val < 127)) ? val : '.';

            textstr << ascVal;
        }

        sstr << " | " << textstr.str() << endl;
    }

    return sstr.str();
}

static void UpdateFeature_AtaVersion(U16 w80, U8* p) {
    // 0000h or FFFFh = device does not report version
    // 15:9 Reserved
    // 8 1 = supports ATA8-ACS
    // 7 1 = supports ATA/ATAPI-7
    // 6 1 = supports ATA/ATAPI-6
    // 5 1 = supports ATA/ATAPI-5
    // 4 1 = supports ATA/ATAPI-4
    // 3 Obsolete
    // 2 Obsolete
    // 1 Obsolete
    // 0 Reserved

    *p = 0;
    if (!w80 || (w80 == 0xFFFF)) return;
    for (int i = 15; i >= 4; i--)
        if ((w80 >> i) & 0x1) { *p = i; break; }
}

static void UpdateFeature_MaxDmaMode(U16 w63, U16 w88, U8* p) {
    *p = 0;

    // Ultra DMA modes
    // 15 Reserved
    // 14 1 = Ultra DMA mode 6 is selected
    //    0 = Ultra DMA mode 6 is not selected
    // 13 1 = Ultra DMA mode 5 is selected
    //    0 = Ultra DMA mode 5 is not selected
    // 12 1 = Ultra DMA mode 4 is selected
    //    0 = Ultra DMA mode 4 is not selected
    // 11 1 = Ultra DMA mode 3 is selected
    //    0 = Ultra DMA mode 3 is not selected
    // 10 1 = Ultra DMA mode 2 is selected
    //    0 = Ultra DMA mode 2 is not selected
    // 9  1 = Ultra DMA mode 1 is selected
    //    0 = Ultra DMA mode 1 is not selected
    // 8  1 = Ultra DMA mode 0 is selected
    //    0 = Ultra DMA mode 0 is not selected
    // 7 Reserved
    // 6  1 = Ultra DMA mode 6 and below are supported
    // 5  1 = Ultra DMA mode 5 and below are supported
    // 4  1 = Ultra DMA mode 4 and below are supported
    // 3  1 = Ultra DMA mode 3 and below are supported
    // 2  1 = Ultra DMA mode 2 and below are supported
    // 1  1 = Ultra DMA mode 1 and below are supported
    // 0  1 = Ultra DMA mode 0 is supported

    U8 utdma = 0;
    #define MAP_ITEM(w, sup, sel, mode) \
        if (GET_BIT(w, sup) && GET_BIT(w, sel)) { utdma = mode; break; }
    do {
        MAP_ITEM(w88, 6,14, 6); MAP_ITEM(w88, 5,13, 5);
        MAP_ITEM(w88, 4,12, 4); MAP_ITEM(w88, 3,11, 3);
        MAP_ITEM(w88, 2,10, 2); MAP_ITEM(w88, 1, 9, 1);
        MAP_ITEM(w88, 0, 8, 0);
    } while(0);
    #undef MAP_ITEM

    // W63
    // 15:11 Reserved
    // 10 1 = Multiword DMA mode 2 is selected
    //    0 = Multiword DMA mode 2 is not selected
    // 9  1 = Multiword DMA mode 1 is selected
    //    0 = Multiword DMA mode 1 is not selected
    // 8  1 = Multiword DMA mode 0 is selected
    //    0 = Multiword DMA mode 0 is not selected
    // 7:3 Reserved
    // 2  1 = Multiword DMA mode 2 and below are supported
    // 1  1 = Multiword DMA mode 1 and below are supported
    // 0  1 = Multiword DMA mode 0 is supported

    U8 mwdma = 0;
    #define MAP_ITEM(w, sup, sel, mode) \
        if (GET_BIT(w, sup) && GET_BIT(w, sel)) { mwdma = mode; break; }
    do {
        MAP_ITEM(w63, 2,10, 2);
        MAP_ITEM(w63, 1, 9, 1);
        MAP_ITEM(w63, 0, 8, 0);
    } while(0);
    #undef MAP_ITEM

    // Word 63 identifies the Multiword DMA transfer modes supported by the device and indicates the mode that is
    // currently selected. Only one DMA mode shall be selected at any given time. If an Ultra DMA mode is enabled,
    // then no Multiword DMA mode shall be enabled. If a Multiword DMA mode is enabled then no Ultra DMA mode
    // shall be enabled.

    // Word 88 identifies the Ultra DMA transfer modes supported by the device and indicates the mode that is
    // currently selected. Only one DMA mode shall be selected at any given time. If an Ultra DMA mode is selected,
    // then no Multiword DMA mode shall be selected. If a Multiword DMA mode is selected, then no Ultra DMA mode
    // shall be selected. Support of this word is mandatory if any Ultra DMA mode is supported.

    U8 res[][2] = {{0,mwdma},{(U8)0x80 | utdma,0}};
    *p = res[!mwdma][!utdma];
}

void DeviceMgr::ParseIdentifyInfo(unsigned char* idSector, string driveName, sIDENTIFY& driveInfo)
{
    // Parse identify info and store in driveInfo
    if(1) {
        driveInfo.DriveName = driveName;
    }

    if(1) {
        const int wordPos = 27;   // Word Position of ModelNumber
        const int wordCount = 20; // Word Count
        unsigned char buffer[wordCount*2 + 1] = {0};
        CopyWord(buffer, idSector + wordPos * 2, wordCount, true);
        driveInfo.DeviceModel = string((char*)buffer);
        StringUtil::Trim(driveInfo.DeviceModel);
    }

    if(1) {
        const int wordPos = 10;   // Word Position of SerialNumber
        const int wordCount = 10; // Word Count
        unsigned char buffer[wordCount*2 + 1] = {0};
        CopyWord(buffer, idSector + wordPos * 2, wordCount, true);
        driveInfo.SerialNumber = string((char*)buffer);
        StringUtil::Trim(driveInfo.SerialNumber);
    }

    if(1) {
        const int wordPos = 23;   // Word Position of FirmwareVersion
        const int wordCount =  4; // Word Count
        unsigned char buffer[wordCount*2 + 1] = {0};
        CopyWord(buffer, idSector + wordPos * 2, wordCount, true);
        driveInfo.FirmwareVersion = string((char*)buffer);
        StringUtil::Trim(driveInfo.FirmwareVersion);
    }

    if(1) {
        // Read Capacity (48b)

        unsigned int usercap_low =
            (idSector[203] << 24) |
            (idSector[202] << 16) |
            (idSector[201] <<  8) |
            (idSector[200]      );

        unsigned int usercap_high =
            (idSector[207] << 24) |
            (idSector[206] << 16) |
            (idSector[205] <<  8) |
            (idSector[204]      );

        // UINT64 usercap = ((((UINT64) usercap_high << 32) | usercap_low) << 9 ) / 100000000;
        // driveInfo.UserCapacity = (double) usercap / 10;

        driveInfo.UserCapacity = 32 * usercap_high * pow(1.6, 9) + usercap_low * pow(0.2, 9);
    }

    #define DEF_WORD(idx) U16 w##idx = GET_WORD(idSector, idx)
    #define GET_WORD_BIT(name, widx, start, len) \
        p.name = (w##widx >> start) & ((1 << len) - 1)
    if (1) {
        // Read Security Feature:
        DEF_WORD(82); DEF_WORD(85);
        DEF_WORD(89); DEF_WORD(90); DEF_WORD(128);

        sFEATURE &p = driveInfo.SectorInfo;
        GET_WORD_BIT(IsSecuritySupported      ,  82, 1, 1);
        GET_WORD_BIT(IsUserPasswordPresent    ,  85, 1, 1);
        GET_WORD_BIT(IsDeviceLocked           , 128, 2, 1);
        GET_WORD_BIT(IsDeviceFrozen           , 128, 3, 1);
        GET_WORD_BIT(IsPasswordAttemptExceeded, 128, 4, 1);
        GET_WORD_BIT(IsMasterPasswordMaximum  , 128, 8, 1);
        p.SecureEraseTime = w89;
        p.EnhancedSecureEraseTime = w90;
    }

    if (1) {
        // Read Other Features:
        DEF_WORD(83); DEF_WORD(93); DEF_WORD(49);
        DEF_WORD(168); DEF_WORD(169); DEF_WORD(82);
        DEF_WORD(76); DEF_WORD(84); DEF_WORD(80);
        DEF_WORD(63); DEF_WORD(88);

        sFEATURE &p = driveInfo.SectorInfo;
        GET_WORD_BIT(IsDlCodeSupported, 83,  0, 1);
        GET_WORD_BIT(IsLba48Supported,  83, 10, 1);
        GET_WORD_BIT(AtaVersion,        80,  4, 8);
        GET_WORD_BIT(IsDmaSupported,    49,  8, 1);
        GET_WORD_BIT(IsSmartSupported,  82,  0, 1);
        GET_WORD_BIT(IsNcqSupported,    76,  8, 1);
        GET_WORD_BIT(IsTestSupported,   84,  1, 1);

        GET_WORD_BIT(IsOpSupported,    168, 14, 1);
        GET_WORD_BIT(IsTrimSupported,  169,  0, 1);

        // Overwrite some features:
        UpdateFeature_AtaVersion(w80, &p.AtaVersion);
        UpdateFeature_MaxDmaMode(w63, w88, &p.MaxDmaMode);
    }
    #undef DEF_WORD
    #undef GET_WORD_BIT
}

bool DeviceMgr::ParseSmartAttribute(unsigned char* valSector, unsigned char* thrSector, sSMARTATTR& attr)
{
    unsigned char attrID = valSector[0];
    if((0 == attrID) || (attrID != thrSector[0])) return false;

    unsigned char attrThr = thrSector[1];
    unsigned char attrVal = valSector[3];
    unsigned char attrWor = valSector[4];
    unsigned long attrRawHigh = (valSector[10] << 8) | (valSector[9]);
    unsigned long attrRawLow  = (valSector[8] << 24) | (valSector[7] << 16) | (valSector[6] << 8) | (valSector[5]);

    attr.ID = attrID;

    // Fix name of attributes

    string name;
    if(true == cCoreUtil::LookupAttributeName(attr.ID, name))
    {
        attr.Name = name;
    }

    attr.Value = attrVal;
    attr.Worst = attrWor;
    attr.Threshold = attrThr;
    attr.LowRaw = attrRawLow;
    attr.HighRaw = attrRawHigh;
    return true;
}

void DeviceMgr::CorrectAttribute(sSMARTATTR& attr)
{
    // Correct values of some attribute:
    switch(attr.ID)
    {
        case 0xC2: attr.LowRaw &= 0xFF; break; // Temperature:
        default: break;
    }
}

void DeviceMgr::ParseSmartData(unsigned char* valSector, unsigned char* thrSector, sSMARTINFO& smartInfo)
{
    valSector += 2;
    thrSector += 2;

    // Standard attributes
    for (int i = 0; i < 30; i++)
    {
        sSMARTATTR attr;
        if (true == ParseSmartAttribute(valSector, thrSector, attr))
        {
            CorrectAttribute(attr);
            smartInfo.AttrMap[attr.ID] = attr;
        }

        valSector += 12;
        thrSector += 12;
    }
}

