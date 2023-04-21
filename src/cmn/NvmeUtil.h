#ifndef NvmeUtil_H
#define NvmeUtil_H

#include "StdMacro.h"
#include "StdHeader.h"
#include "CmdBase.h"

#include <windows.h>
#include <nvme.h>


class NvmeUtil
{
public:
    static BOOL IdentifyNamespace(HANDLE hDevice, DWORD dwNSID, PNVME_IDENTIFY_NAMESPACE_DATA pNamespaceIdentify);
    static BOOL IdentifyController(HANDLE hDevice, PNVME_IDENTIFY_CONTROLLER_DATA pControllerIdentify);
    static BOOL GetHealthInfoLog(HANDLE hDevice, PNVME_HEALTH_INFO_LOG pHealthInfoLog);
    static BOOL GetHealthInfoLog(HANDLE hDevice, DWORD dwNSID, PNVME_HEALTH_INFO_LOG hlog);
};

#endif // NvmeUtil_H
