#include "SystemUtil.h"
#include "windows.h"
#include <shlobj_core.h>

bool SystemUtil::IsAdminMode()
{
    return true;
    //return IsUserAnAdmin ();
}
