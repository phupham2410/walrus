#ifndef AtaBase_H
#define AtaBase_H

#include "StdMacro.h"
#include "StdHeader.h"
#include "CmdBase.h"

enum eATACODE
{
    #define MAP_ITEM(code, val, data, mode, cmd) code = (U32) ((data << 30) | (mode << 28) | (cmd << 26) | val),
    #include "AtaCode.def"
    #undef MAP_ITEM
};

class AtaBase : public CmdBase
{
public:
    void buildTskFile(U8* task, U8* prev = NULL);
    void buildScsiCmdBlock16(U8* cmdb);
    void buildScsiCmdBlock12(U8* cmdb);

public:
    std::string toString() const;
};

#endif // AtaBase_H
