#include "CoreData.h"
#include "CoreUtil.h"
#include "StringUtil.h"
#include "CommonUtil.h"

using namespace std;

sSMARTATTR::sSMARTATTR()
{
    reset();
}

void sSMARTATTR::reset()
{
    ID = 0;
    Value = Worst = Threshold = 0;
    LowRaw = HighRaw = 0;
    Name = "Reserved_Attribute";
    Note = "";
}

sFEATURE::sFEATURE()
{
    reset();
}

void sFEATURE::reset()
{
    memset((void*) this, 0x00, sizeof(*this));
}

string sFEATURE::toString() const
{
    stringstream sstr;

    #define MAP_FLAG(name, flag) \
         sstr << name << ": " << (flag ? "Yes" : "No") << endl

    #define MAP_ITEM(name, value) \
         sstr << name << ": " << (U32) value << endl

    MAP_FLAG("Security", IsSecuritySupported);
    MAP_FLAG("Password", IsUserPasswordPresent);
    MAP_FLAG("Locked", IsDeviceLocked);
    MAP_FLAG("Frozen", IsDeviceFrozen);
    MAP_FLAG("Exceeded", IsPasswordAttemptExceeded);
    MAP_FLAG("MasterPw", IsMasterPasswordMaximum);
    MAP_ITEM("EraseTime", SecureEraseTime);
    MAP_ITEM("EnhancedTime", EnhancedSecureEraseTime);
    MAP_FLAG("Lba48Mode", IsLba48Supported);
    MAP_ITEM("AtaVersion", AtaVersion);
    MAP_FLAG("Dma", IsDmaSupported);
    MAP_ITEM("MaxDmaMode", MaxDmaMode);
    MAP_FLAG("Trim", IsTrimSupported);
    MAP_FLAG("Smart", IsSmartSupported);
    MAP_FLAG("Ncq", IsNcqSupported);
    MAP_FLAG("OverProvision", IsOpSupported);
    MAP_FLAG("SelfTest", IsTestSupported);
    MAP_FLAG("Download Microcode", IsDlCodeSupported);

    #undef MAP_ITEM
    #undef MAP_FLAG

    return sstr.str();
}

sIDENTIFY::sIDENTIFY()
{
    reset();
}

void sIDENTIFY::reset()
{
    DriveName = "";
    DeviceModel = "";
    SerialNumber = "";
    FirmwareVersion = "";
    UserCapacity = 0;

    DriveIndex = 0;

    SectorInfo.reset();
}

string sIDENTIFY::toString() const
{
    stringstream sstr;

    sstr << "Model: " << DeviceModel << endl;
    sstr << "Serial: " << SerialNumber << endl;
    sstr << "Capacity: " << UserCapacity << "(GB)" << endl;
    sstr << "Firmware: " << FirmwareVersion << endl;

    return sstr.str();
}

void sSMARTINFO::reset()
{
    AttrMap.clear();
}

sDRVINFO::sDRVINFO()
{
    reset();
}

void sDRVINFO::reset()
{
    SmartInfo.reset();
    IdentifyInfo.reset();
}

// -----------------------------------------------------------------
// Function for handling core data
// -----------------------------------------------------------------

string ToString(const sSMARTINFO& info)
{
    stringstream sstr;
    const char* sep = ";";

    tATTRMAP::const_iterator iter;
    for (iter = info.AttrMap.begin(); iter != info.AttrMap.end(); ++iter)
    {
        const sSMARTATTR& attr = iter->second;

        ASSERT (iter->first == attr.ID);

        sstr
            << (U32) attr.ID  << sep
            << attr.Name      << sep
            << (U32) attr.LowRaw    << sep
            << (U32) attr.Value     << sep
            << (U32) attr.Worst     << sep
            << (U32) attr.Threshold << sep;
    }

    return sstr.str();
}

string ToString(const sIDENTIFY& info)
{
    stringstream sstr;
    const char* sep = ";";

    sstr
        << info.DriveName       << sep
        << info.DeviceModel     << sep
        << info.SerialNumber    << sep
        << info.FirmwareVersion << sep
        << info.UserCapacity    << sep;

    return sstr.str();
}

string ToString(const sDRVINFO& info)
{
    stringstream sstr;
    const char* sep = ";";

    sstr << info.IdentifyInfo.SerialNumber << sep;
    sstr << ToString(info.IdentifyInfo);
    sstr << ToString(info.SmartInfo);

    return sstr.str();
}

string ToVerboseString(const sIDENTIFY& info)
{
    stringstream sstr;

    sstr << "Name: " << info.DriveName << endl;
    sstr << "Model: " << info.DeviceModel << endl;
    sstr << "Serial: " << info.SerialNumber << endl;
    sstr << "Version: " << info.FirmwareVersion << endl;
    sstr << "UserCap: " << info.UserCapacity << endl;

    return sstr.str();
}

string ToVerboseString(const sSMARTINFO& info)
{
    stringstream sstr;
    const char* sep = " ";

    tHeaderItem itemList[] = {
        tHeaderItem("ID", 3),
        tHeaderItem("Name", 28),
        tHeaderItem("LowRaw", 10),
        tHeaderItem("Val", 3),
        tHeaderItem("Wor", 3),
        tHeaderItem("Thr", 3),
    };

    MakeHeaderList(itemList, header);
    sstr << header.ToString(" ") << endl;

    tATTRMAP::const_iterator iter;
    for (iter = info.AttrMap.begin(); iter != info.AttrMap.end(); ++iter)
    {
        const sSMARTATTR& attr = iter->second;

        ASSERT (iter->first == attr.ID);

        sstr
            << FRMT_U32(header.Length(0), attr.ID)        << sep
            << FRMT_STR(header.Length(1), attr.Name)      << sep
            << FRMT_U32(header.Length(2), attr.LowRaw)    << sep
            << FRMT_U32(header.Length(3), attr.Value)     << sep
            << FRMT_U32(header.Length(4), attr.Worst)     << sep
            << FRMT_U32(header.Length(5), attr.Threshold) << sep
            << endl;
    }

    return sstr.str();
}

string ToVerboseString(const sDRVINFO& info)
{
    stringstream sstr; string sep(55, '-');
    sstr << ToVerboseString(info.IdentifyInfo) << sep << endl;
    sstr << ToVerboseString(info.SmartInfo) << sep << endl;

    return sstr.str();
}
