#ifndef DEVICEMANAGER
#define DEVICEMANAGER

#include "CoreData.h"
#include "AtaBase.h"

enum eSCANERROR
{
    SCAN_ERROR_NONE,
    SCAN_ERROR_SKIPDRIVE,
    SCAN_ERROR_PERMISSION,
};

enum eSECSTATE
{
    SECURITY_STATE_INVALID     = 0x00000001,
    SECURITY_STATE_NOTSUPORTED = 0x00000002,
    SECURITY_STATE_0           = 0x00000010,
    SECURITY_STATE_1           = 0x00000020,
    SECURITY_STATE_2           = 0x00000040,
    SECURITY_STATE_3           = 0x00000080,
    SECURITY_STATE_4           = 0x00000100,
    SECURITY_STATE_5           = 0x00000200,
    SECURITY_STATE_6           = 0x00000400,
};

struct sPHYDRVINFO
{
    int DriveNumber;
    int DriveHandle;
    std::string DriveName;
};

typedef std::vector<sPHYDRVINFO> tPHYDRVARRAY;

class DeviceMgr
{
// -------------------------------------------------------------
// Porting to each OS
// -------------------------------------------------------------
public:
    static bool OpenDevice(const std::string& deviceName, int& handle);
    static void CloseDevice(int handle);
    static bool OpenDevice(U32 idx, sPHYDRVINFO& drive);
    static void OpenDevice(tPHYDRVARRAY& driveList);
    static void CloseDevice(sPHYDRVINFO& drive);
    static void CloseDevice(tPHYDRVARRAY& driveList);
    static bool ReadIdentifyData(const std::string& deviceName, U8* identifyBuffer);
    static eSCANERROR ScanDrive(std::list<sDRVINFO>& infoList, bool readSMART = true, const tSERIALSET& serialSet = tSERIALSET());

// -------------------------------------------------------------
// Common functions
// -------------------------------------------------------------
public: // Utility functions
    static void Message(eSCANERROR scanError);
    static std::string ToAsciiString(const unsigned char* buff);
    static void CopyWord(unsigned char* pdst0, const unsigned char* src, int wordcount, bool byteswap = false);

public:
    static bool IsValidSerial(const tSERIALSET& serialSet, const std::string& serialNum);

public: // Handle scan drive
    static eSCANERROR ScanDriveCommon(std::list<sDRVINFO>& infoList, bool readSMART = true, const tSERIALSET& serialSet = tSERIALSET());

public: // Handle Identify data
    static bool ReadIdentifyDataCommon(const std::string& deviceName, U8* identifyBuffer); // Implement using AtaCmd
    static void ParseIdentifyInfo(unsigned char* idSector, std::string driveName, sIDENTIFY& driveInfo);

public: // Handle SMART data
    static void CorrectAttribute(sSMARTATTR& attr);
    static void ParseSmartData(unsigned char* valSector, unsigned char* thrSector, sSMARTINFO& smartInfo);
    static bool ParseSmartAttribute(unsigned char* valSector, unsigned char* thrSector, sSMARTATTR& attr);

public: // Support Security Feature
    static eSECSTATE ReadSecurrityState(const sFEATURE& info);
    static eSECSTATE ReadSecurrityState(const std::string& deviceName);
    static eSECSTATE ReadSecurrityState(const U8* identifyBuffer);

    static bool TestPasswordState(eATACODE code);
    static bool TestSecurityState(eSECSTATE state, eATACODE code, std::string& errorString);
};


#endif // DEVICEMANAGER

