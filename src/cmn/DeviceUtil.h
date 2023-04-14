#ifndef __DEVICEMANAGERUTIL_H__
#define __DEVICEMANAGERUTIL_H__

#include "CoreData.h"
#include "StdMacro.h"
#include "StdHeader.h"
#include "DeviceMgr.h"
#include "SysHeader.h"

// Utility functions for Windows port

typedef bool (*tIssueCommandFunc) (HANDLE, int, unsigned char*);

class DeviceUtil
{
public:
    static void ScanPhysicalDrive_Test();
    static void IssueCommand_Test(tIssueCommandFunc func);

    static bool IssueCommand_IdentifyDrive(HANDLE handle, int driveNumber, unsigned char* data);
    static bool IssueCommand_ReadSmartValue(HANDLE handle, int driveNumber, unsigned char* data);
    static bool IssueCommand_ReadSmartThreshold(HANDLE handle, int driveNumber, unsigned char* data);
};

#endif // __DEVICEMANAGERUTIL_H__
