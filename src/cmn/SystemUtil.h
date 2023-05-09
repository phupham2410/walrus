#ifndef __SYSTEMUTIL_H__
#define __SYSTEMUTIL_H__

#include "StdHeader.h"

class SystemUtil
{
public:
    static bool IsAdminMode();
    static std::string GetLastErrorString();
    
};

#endif //__SYSTEMUTIL_H__
