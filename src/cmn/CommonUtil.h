#ifndef __COMMONUTIL_H__
#define __COMMONUTIL_H__

#include "StdMacro.h"
#include "StdHeader.h"

// ---------------------------------------------------------------------
// Utility for handling header texts
// ---------------------------------------------------------------------

typedef std::pair<std::string, int> tHeaderItem;

class HeaderList
{
public:
    int Size() const;
    int Length(unsigned int index) const;
    std::string Text(unsigned int index) const;
    void AddHeader(const tHeaderItem& item);

public:
    std::vector<int> ToSizeList() const;
    std::string ToString(const char* sep) const;

public:
    std::vector<tHeaderItem> Header;
};

#define MakeHeaderList(name, cont) HeaderList cont; do { \
    U32 size = sizeof (name) / sizeof (name[0]); \
    for (U32 i = 0; i < size; i++) { \
        tHeaderItem item = name[i]; \
        if (0 == item.second) item.second = item.first.length(); \
        cont.AddHeader(item); \
    }\
} while(0)

class CommonUtil
{
public: // MessageBox group
    static void Message(const char* msg);
    static void Message(const std::string& msg);
    static void Message(const std::vector<std::string>& msg);

public:
    static int RoundUp(double d);
    static int BitCount(unsigned int value);

    static double RoundPrecision(double d);
    
    static std::string ToHexString(int value);

    static std::string ToTimeString(int dayCount, bool approx = false);

    static void GetTimeInfo(int dayCount, int& year, int& month, int& day);

    static std::string FormatTime(const char* textFormat, const char* timeFormat, time_t sec);

    static void FormatSector(U8* bufptr, U64 lba, U32 wrtcnt);
    static U32 FormatBuffer(U8* bufptr, U32 bufsec, U64 lba, U32 wrtsec, U32 wrtcnt);

    static void FormatHeader(U8* bufptr, U64 lba, U32 wrtcnt);
    static U32 FormatHeader(U8* bufptr, U32 bufsec, U64 lba, U32 wrtsec, U32 wrtcnt);
};

#endif //__COMMONUTIL_H__
