#ifndef __COREDATA_H__
#define __COREDATA_H__

#include "StdMacro.h"
#include "StdHeader.h"

#include "CoreUtil.h"

struct sSMARTATTR
{
    U8  ID;
    U8  Value;
    U8  Worst;
    U8  Threshold;
    U32 LowRaw;
    U32 HighRaw;
    std::string Name;
    std::string Note;

    sSMARTATTR();
    void reset();
};

typedef std::map<U8, sSMARTATTR> tATTRMAP;
typedef tATTRMAP::iterator tATTRMAPITR;
typedef tATTRMAP::const_iterator tATTRMAPCITR;
struct sSMARTINFO
{
    tATTRMAP AttrMap; // ID <-> Attribute

    void reset();
};

struct sFEATURE
{
    // SecureErase Feature
    bool IsSecuritySupported;       // W82:1 (also W128:0)
    bool IsUserPasswordPresent;     // W85:1 (also W128:1)
    bool IsDeviceLocked;            // W128:2
    bool IsDeviceFrozen;            // W128:3
    bool IsPasswordAttemptExceeded; // W128:4
    bool IsEnhancedEraseSupported;  // W128:5
    bool IsMasterPasswordMaximum;   // W128:8

    U16 SecureEraseTime;            // W89
    U16 EnhancedSecureEraseTime;    // W90

    // Other flags:
    bool IsDlCodeSupported;         // Supported(W83:0)
    bool IsLba48Supported;          // Supported(W83:10), Enabled(W86:10)
    bool IsTestSupported;           // Supported(W84:1)
    bool IsSmartSupported;          // Supported(W82:0), Enabled(W85:0)
    bool IsDmaSupported;            // Supported(W49:8)
    U8 MaxDmaMode;                  // W63 & W88
    U8 AtaVersion;                  // W80:4->8; 0000 or FFFF: not reported
    bool IsNcqSupported;            // W76:8

    // Recheck these values
    bool IsTrimSupported;           // W169:0
    bool IsOpSupported;             // W168:14

    sFEATURE();
    void reset();
    std::string toString() const;
};

struct sIDENTIFY
{
    std::string DriveName;
    std::string DeviceModel;
    std::string SerialNumber;
    std::string FirmwareVersion;
    double UserCapacity;    // User capacity in GB ( not raw capacity )

    U8 DriveIndex;

    sFEATURE SectorInfo;

    sIDENTIFY();
    void reset();
    std::string toString() const;
};

struct sDRVINFO
{
    sSMARTINFO SmartInfo;
    sIDENTIFY IdentifyInfo;

    sDRVINFO();
    void reset();
};

typedef std::set<std::string> tSERIALSET;

// -----------------------------------------------------------------
// Function for handling core data
// -----------------------------------------------------------------

// Utility function
std::string ToString(const sSMARTINFO& info);
std::string ToString(const sIDENTIFY& info);
std::string ToString(const sDRVINFO& info);

std::string ToVerboseString(const sSMARTINFO& info);
std::string ToVerboseString(const sIDENTIFY& info);
std::string ToVerboseString(const sDRVINFO& info);

#endif
