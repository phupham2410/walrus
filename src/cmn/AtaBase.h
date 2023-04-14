#ifndef AtaBase_H
#define AtaBase_H

#include "StdMacro.h"
#include "StdHeader.h"
#include "CmdBase.h"

#define PROTOCOL_NO_DATA 0x06
#define PROTOCOL_PIO_IN  0x08
#define PROTOCOL_PIO_OUT 0x0A
#define PROTOCOL_DMA     0x0C

#define TLENGTH_NO_DATA    0x00
#define TLENGTH_IN_FEATURE 0x01
#define TLENGTH_IN_SECTOR  0x02
#define TLENGTH_IN_TPSIU   0x03

#define TRANSFER_BYTE      (0 << 2)
#define TRANSFER_BLOCK     (1 << 2)

#define TRANSFER_TO_DEV    (0 << 3)
#define TRANSFER_FROM_DEV  (1 << 3)

#define CHECK_CONDITION    (1 << 5)

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
