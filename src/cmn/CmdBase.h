#ifndef CMDBASE_H
#define CMDBASE_H

#include "StdMacro.h"
#include "StdHeader.h"

enum eDATAMODE
{
    DATA_NONE = 0,
    DATA_IN   = 1,
    DATA_OUT  = 2,
};

enum eTRANSMODE
{
    MODE_NONE = 0,
    MODE_PIO  = 1,
    MODE_DMA  = 2,
};

enum eCMDMODE
{
    CMD_28B  = 1,
    CMD_48B  = 2,
};

enum eCMDERR
{
    #define MAP_ITEM(code) code,
    #include "CmdError.def"
    #undef MAP_ITEM
};

struct sADDRESS
{
    U32 lo;
    U32 hi;

    sADDRESS();
    sADDRESS(U64 lba);
    sADDRESS(U32 low, U32 high);
    sADDRESS operator+(U32 lba);
    sADDRESS operator+(const sADDRESS& lba);
    bool operator==(const sADDRESS& lba);
    U64 toU64() const;
};

class CmdBase
{
public:
    CmdBase();
    ~CmdBase();

public:
    sADDRESS LBA;
    U32 SecCount;
    U32 CmdCode;
    
    U8* Buffer;
    U32 StructSize;
    U32 FillerSize;
    U32 BufferSize;

public:
    U8* getBufferPtr();
    U8* getStructPtr();
    void allocBuffer();
    void releaseBuffer();

public:
    void reset();

public:
    void formatAddress(U32 writeCount);
    void formatBuffer(U8 *pattern, U32 size, U32 writeCount);
    void randomBuffer(U32 writeCount);

public:
    bool isDmaMode() const;
    bool isPioMode() const;

    bool isDataIn() const;
    bool isDataOut() const;
    bool is48BitCmd() const;
    bool hasTransfer() const;

    U8   getFeatureCode() const;
    
public:
    static std::string getErrorString(eCMDERR code);
};

#endif // CMDBASE_H
