#include "CommonUtil.h"
#include "StdMacro.h"

using namespace std;

int HeaderList::Size() const
{
    return Header.size();
}

int HeaderList::Length(unsigned int index) const
{
    return (index < Header.size()) ? Header[index].second : 0;
}

string HeaderList::Text(unsigned int index) const
{
    return (index < Header.size()) ? Header[index].first : "";
}

void HeaderList::AddHeader(const tHeaderItem &item)
{
    Header.push_back(item);
}

string HeaderList::ToString(const char *sep) const
{
    stringstream sstr;

    int size = Header.size();
    for(int i = 0; i < size; i++)
    {
        sstr
            << setfill(' ') << setw(Header[i].second)
            << Header[i].first
            << sep;
    }
    return sstr.str();
}

vector<int> HeaderList::ToSizeList() const
{
    vector<int> lst;

    int size = Header.size();
    for(int i = 0; i < size; i++) lst.push_back( Length(i) );
    return lst;
}

void CommonUtil::Message(const char* msg)
{

}

void CommonUtil::Message(const string &msg)
{

}

void CommonUtil::Message(const vector<string> &msg)
{

}

string CommonUtil::ToHexString(int value)
{
    stringstream sstr;
    sstr << hex << uppercase << value;
    return sstr.str();
}

void CommonUtil::GetTimeInfo(int dayCount, int &year, int &month, int &day)
{
    year = dayCount / 365; dayCount %= 365;
    month = dayCount / 30; dayCount %= 30;
    day = dayCount;
}

string CommonUtil::ToTimeString(int dayCount, bool approx)
{
    stringstream sstr;
    int year, month, day;
    GetTimeInfo(dayCount, year, month, day);

    do
    {
        if(0 != year)
        {
            sstr << year << ((year > 1) ? " years" : " year");
        }

        if(0 != month)
        {
            if(year != 0) sstr << " ";
            sstr << month << ((month > 1) ? " months" : " month");
            if((year != 0) && approx) break;
        }

        if(0 != day)
        {
            if((month != 0) || (year != 0)) sstr << " ";
            sstr << day << ((day > 1) ? " days" : " day");
        }
    } while(0);

    return sstr.str();
}

int CommonUtil::BitCount(unsigned int value)
{
    int index = 0;
    while(0 != value) { value >>= 1; index++; }
    return index;
}

int CommonUtil::RoundUp(double d)
{
    int res;

    int bitCount = BitCount((int) d);

    res = (1 << bitCount) >> 1;
    if (res < d) res <<= 1;

    return res;
}

double CommonUtil::RoundPrecision(double d)
{
    return (int) (d * 100) / 100.0;
}

string CommonUtil::FormatTime(const char* textFormat, const char* timeFormat, time_t sec)
{
    char timeBuff[80];
    int buffSize = sizeof(timeBuff) / sizeof(timeBuff[0]);
    strftime( timeBuff, buffSize, timeFormat, localtime(&sec));

    string timeString;
    FORMAT_STRING(timeString, textFormat, timeBuff);
    return timeString;
}

void CommonUtil::FormatSector(U8* bufptr, U64 lba, U32 wrtcnt) {
    char tmp[33] = {0}; U8* ptr = bufptr;
    sprintf(tmp, "LBA %12X CNT %10d ", lba, wrtcnt);
    memcpy(ptr, tmp, 32); ptr += 32;

    const char* pat =
        "LINE1:SECTOR_DATA_SECTOR_DATA_SE"
        "LINE2:CTOR_DATA_SECTOR_DATA_SECT"
        "LINE3:OR_DATA_SECTOR_DATA_SECTOR"
        "LINE4:_DATA_SECTOR_DATA_SECTOR_D"
        "LINE5:ATA_SECTOR_DATA_SECTOR_DAT"
        "LINE6:A_SECTOR_DATA_SECTOR_DATA_"
        "LINE7:SECTOR_DATA_SECTOR_DATA_SE"
        "LINE8:CTOR_DATA_SECTOR_DATA_SECT"
        "LINE9:OR_DATA_SECTOR_DATA_SECTOR"
        "LINEA:_DATA_SECTOR_DATA_SECTOR_D"
        "LINEB:ATA_SECTOR_DATA_SECTOR_DAT"
        "LINEC:A_SECTOR_DATA_SECTOR_DATA_"
        "LINED:SECTOR_DATA_SECTOR_DATA_SE"
        "LINEE:CTOR_DATA_SECTOR_DATA_SECT"
        "LINEF:OR_DATA_SECTOR_DATA_SECTOR";

    memcpy(ptr, pat, 32 * 15);
}

U32 CommonUtil::FormatBuffer(U8* bufptr, U32 bufsec, U64 lba, U32 wrtsec, U32 wrtcnt) {
    U32 minsec = MIN2(bufsec, wrtsec);
    for(U64 i = lba, maxi = lba + minsec; i < maxi; i++) {
        FormatSector(bufptr + (i - lba) * 512, i, wrtcnt);
    }
    return minsec;
}

void CommonUtil::FormatHeader(U8* bufptr, U64 lba, U32 wrtcnt) {
    char tmp[33] = {0}; U8* ptr = bufptr;
    sprintf(tmp, "LBA %12X CNT %10d ", lba, wrtcnt);
    memcpy(ptr, tmp, 32); ptr += 32;
}

U32 CommonUtil::FormatHeader(U8* bufptr, U32 bufsec, U64 lba, U32 wrtsec, U32 wrtcnt) {
    U32 minsec = MIN2(bufsec, wrtsec);
    for(U64 i = lba, maxi = lba + minsec; i < maxi; i++) {
        FormatHeader(bufptr + (i - lba) * 512, i, wrtcnt);
    }
    return minsec;
}
