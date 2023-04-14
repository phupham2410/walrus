#ifndef __HEXFRMT_H__
#define __HEXFRMT_H__

#include "StdMacro.h"
#include "StdHeader.h"

struct HexFrmt
{
public:
    enum eValueMode
    {
        MODE_BIN,
        MODE_HEX,
    };

public:
    U32 ColSize;
    U32 ColCount;
    U32 Offset;

    bool ShowHex;
    bool ShowText;
    bool ShowOffset;

    eValueMode Mode;

    HexFrmt();

    void setMode(eValueMode mode);
    void setSize(U32 colSize = 8, U32 colCount = 4, U32 offset = 0);
    void setState(bool showHex = true, bool showText = true, bool showOffset = true);

    std::string toString(const U8* bufPtr, U32 bufSize) const;
    void saveString(const std::string& fileName, const U8* bufPtr, U32 bufSize) const;

public:
    // static function, using default formatter
    static std::string ToHexString(const U8* bufPtr, U32 bufSize);
    static void SaveHexString(const std::string& fileName, const U8* bufPtr, U32 bufSize);
};

#endif //__HEXFRMT_H__
