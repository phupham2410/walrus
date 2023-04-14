#ifndef ScsiCmd_H
#define ScsiCmd_H

#include "AtaBase.h"
#include "ScsiBase.h"

class ScsiCmd : public ScsiBase
{
public:
    bool executeCommand(int handle);
    void setCommand(sADDRESS lba, U32 sectorCount, eATACODE cmdCode);
    void setCommand(sADDRESS lba, U32 sectorCount, eSCSICODE cmdCode);

public:
    eCMDERR getErrorStatus();

private:
    bool buildCommand();      // setting flags, ..
    void buildBasicCommand(); // Initialize basic data

private:
    bool IsSat;
    void setCommand(sADDRESS lba, U32 sectorCount, U32 cmdCode, bool isSat);

};

#endif // ScsiCmd_H

