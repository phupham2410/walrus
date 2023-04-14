#ifndef ScsiBase_H
#define ScsiBase_H

#include "StdMacro.h"
#include "StdHeader.h"
#include "CmdBase.h"

enum eSCSICODE
{
    #define MAP_ITEM(code, val, data, mode) code = (U32)((data << 30) | (mode << 28) | val),
    #include "ScsiCode.def"
    #undef MAP_ITEM
};

class ScsiBase : public CmdBase
{
public:
    void buildCmdBlock_Read10(U8* cmdb);

    void buildCmdBlock_Write10(U8* cmdb);

public:
    void buildSatCmdBlock(U8* sat);
    void buildScsiCmdBlock(U8* cmdb);

public:
    std::string toString() const;

public:
    bool IsSat;
    void setCommand(sADDRESS lba, U32 sectorCount, U32 cmdCode, bool isSat);
};

#endif // ScsiBase_H
