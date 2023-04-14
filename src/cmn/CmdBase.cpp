#include "CmdBase.h"

using namespace std;

sADDRESS::sADDRESS() : lo(0), hi(0) {}
sADDRESS::sADDRESS(U32 low, U32 high) : lo(low), hi(high) {}
sADDRESS::sADDRESS(U64 lba) : lo(lba&0xFFFFFFFF), hi((lba>>32) & 0xFFFFFFFF) {}

sADDRESS sADDRESS::operator+ ( U32 lba )
{
    sADDRESS sum(lba, 0);
    return *this + sum;
}

sADDRESS sADDRESS::operator+ ( const sADDRESS& iLBA )
{
    sADDRESS sum(lo  + iLBA.lo, hi + iLBA.hi);
    if ( ( sum.lo < lo ) || ( sum.lo < iLBA.lo ) ) ++sum.hi;
    return sum;
}

bool sADDRESS::operator== ( const sADDRESS& iLBA )
{
    return ( lo == iLBA.lo ) && ( hi == iLBA.hi );
}

U64 sADDRESS::toU64() const {
    return ((U64)hi << 32) | lo;
}

void CmdBase::reset()
{
    LBA.lo = LBA.hi = 0;
    SecCount = 0;
    CmdCode = 0xFFFF;

    Buffer = NULL;
}

CmdBase::CmdBase()
{
    reset();
}

CmdBase::~CmdBase()
{
    releaseBuffer();
}

U8* CmdBase::getStructPtr()
{
    return Buffer;
}

U8* CmdBase::getBufferPtr()
{
    return Buffer + (StructSize + FillerSize);
}

void CmdBase::allocBuffer()
{
    U32 TotalSize = BufferSize + StructSize + FillerSize;

    if (NULL != Buffer) releaseBuffer();

    if (0 == TotalSize) return;

    Buffer = new U8[TotalSize];
    memset(Buffer, 0x00, TotalSize);
}

void CmdBase::releaseBuffer()
{
    if (NULL != Buffer) delete Buffer;

    Buffer = NULL;
}

bool CmdBase::isDataOut() const // Write
{
    return ((CmdCode >> 30) & 0x3) == DATA_OUT;
}

bool CmdBase::isDataIn() const // Read
{
    return ((CmdCode >> 30) & 0x3) == DATA_IN;
}

bool CmdBase::hasTransfer() const
{
    return ((CmdCode >> 30) & 0x3) != 0;
}

bool CmdBase::isPioMode() const
{
    return ((CmdCode >> 28) & 0x3) == MODE_PIO;
}

bool CmdBase::isDmaMode() const
{
    return ((CmdCode >> 28) & 0x3) == MODE_DMA;
}

bool CmdBase::is48BitCmd() const // Write
{
    return ((CmdCode >> 26) & 0x3) == CMD_48B;
}

U8 CmdBase::getFeatureCode() const
{
    return (CmdCode >> 8) & 0xFF;
}

string CmdBase::getErrorString(eCMDERR code)
{
    string errorString = "Unknown Error Code";
    switch (code)
    {
        #define MAP_ITEM(code) case code: errorString = #code; break;
        #include "CmdError.def"
        #undef MAP_ITEM
    }

    return errorString;
}

void CmdBase::formatAddress(U32 writeCount)
{
    U32 lbaAddress = LBA.lo;
    U8* buffer = getBufferPtr();

    for (U32 i = 0; i < SecCount; i++)
    {
        stringstream sstr;
        sstr << "#LBA:" << FRMT_HEX(8) << lbaAddress << "h" << "  ";
        sstr << "#CNT:" << FRMT_DEC(4) << writeCount << "       ";
        string addrStr = sstr.str();

        memcpy(buffer, addrStr.c_str(), addrStr.length());

        lbaAddress += 1;
        buffer += 512;
    }
}

void CmdBase::randomBuffer(U32 writeCount)
{
    U32 bufSize = (rand() % 512) + 1;
    U8* bufPtr = new U8[bufSize];

    for (U32 i = 0; i < bufSize; i++) bufPtr[i] = rand() % 256;

    formatBuffer(bufPtr, bufSize, writeCount);

    delete[] bufPtr;
}

void CmdBase::formatBuffer(U8 *pattern, U32 size, U32 writeCount)
{
    /* 0000:AB281009h
     * 0020:20
     * 0040:PPPPPPPPPPPPPPPPPPPPPPPPPPP
     * 0060:PPPPPPPPPPPPPPPPPPPPPPPPPPP
     * 0080:PPPPPPPPPPPPPPPPPPPPPPPPPPP
     * 00A0:PPPPPPPPPPPPPPPPPPPPPPPPPPP
     * 00C0:PPPPPPPPPPPPPPPPPPPPPPPPPPP
     * 00E0:PPPPPPPPPPPPPPPPPPPPPPPPPPP
     * 0100:PPPPPPPPPPPPPPPPPPPPPPPPPPP
     * 0120:PPPPPPPPPPPPPPPPPPPPPPPPPPP
     * 0140:PPPPPPPPPPPPPPPPPPPPPPPPPPP
     * 0160:PPPPPPPPPPPPPPPPPPPPPPPPPPP
     * 0180:PPPPPPPPPPPPPPPPPPPPPPPPPPP
     * 01A0:PPPPPPPPPPPPPPPPPPPPPPPPPPP
     * 01C0:PPPPPPPPPPPPPPPPPPPPPPPPPPP
     * 01E0:PPPPPPPPPPPPPPPPPPPPPPPPPPP
     */

     int totalByte = SecCount * 512;
     U8* buffer = getBufferPtr();

     while(totalByte > 0)
     {
         int byteCount = MIN2(size, totalByte);
         memcpy(buffer, pattern, byteCount);

         buffer += byteCount;
         totalByte -= byteCount;
     }

    formatAddress(writeCount);
}
