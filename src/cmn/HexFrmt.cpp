#include "HexFrmt.h"
#include "StdMacro.h"

using namespace std;

HexFrmt::HexFrmt()
{
    setMode(MODE_HEX);
    setSize(8, 4, 0);
    setState(true, true, true);
}

void HexFrmt::setMode(eValueMode mode)
{
    Mode = mode;
}

void HexFrmt::setSize(U32 colSize, U32 colCount, U32 offset)
{
    ColSize = colSize;
    ColCount = colCount;
    Offset = offset;
}

void HexFrmt::setState(bool showHex, bool showText, bool showOffset)
{
    ShowHex = showHex;
    ShowText = showText;
    ShowOffset = showOffset;
}

string HexFrmt::toString(const U8 *bufPtr, U32 bufSize) const
{
    const string sHexTable[] = {
        "0", "1", "2", "3",
        "4", "5", "6", "7",
        "8", "9", "A", "B",
        "C", "D", "E", "F",
    };

    const string sBinTable[] = {
        "0000", "0001", "0010", "0011",
        "0100", "0101", "0110", "0111",
        "1000", "1001", "1010", "1011",
        "1100", "1101", "1110", "1111",
    };

    const string* lookupTable = (Mode == MODE_HEX) ? sHexTable : sBinTable;
    const char* emptyString = (Mode == MODE_HEX) ? "   " : "         ";

    stringstream sstr;

    U32 rowSize = ColSize * ColCount;

    for (U32 i = 0; i < bufSize; i+=rowSize)
    {
        U32 j = 0;
        U32 offset = i;
        stringstream ostr; // OffsetStream
        stringstream hstr; // HexStream
        stringstream tstr; // TextStream

        ostr
            << "["
            << setw(8) << setfill('0') << hex << uppercase
            << offset
            << "]";

        for (; (j < rowSize) && (offset < bufSize); j++, offset++)
        {
            unsigned char c = bufPtr[offset];

            if (j && (0 == (j % ColSize))) hstr << ' ';

            hstr << lookupTable[ (c>>4) & 0xF] << lookupTable[c&0xF] << " ";

            char outchar = '.';
            if ( (c >= 32) && (c < 128) ) outchar = (char) c;
            tstr << outchar;
        }

        for (; j < rowSize; j++) {
            if (j && (0 == (j % ColSize))) hstr << ' ';
            hstr << emptyString;
            tstr << ' ';
        }

        if (true == ShowOffset) sstr << ostr.str();
        if (true == ShowHex)    sstr << ' ' << hstr.str();
        if (true == ShowText)   sstr << ' ' << tstr.str();

        sstr << endl;
    }

    return sstr.str();
}

void HexFrmt::saveString(const string& fileName, const U8* bufPtr, U32 bufSize) const
{
    fstream fstr;
    fstr.open(fileName.c_str(), ios::out);

    fstr << toString(bufPtr, bufSize);

    fstr.close();
}

string HexFrmt::ToHexString(const U8 *bufPtr, U32 bufSize)
{
    return HexFrmt().toString(bufPtr, bufSize);
}

void HexFrmt::SaveHexString(const string& fileName, const U8* bufPtr, U32 bufSize)
{
    HexFrmt().saveString(fileName, bufPtr, bufSize);
}

