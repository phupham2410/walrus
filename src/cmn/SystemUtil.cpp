#include "SystemUtil.h"
#include "windows.h"
#include "shlobj.h"

bool SystemUtil::IsAdminMode()
{
    return IsUserAnAdmin ();
}
