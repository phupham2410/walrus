#ifndef ScsiCmd_H
#define ScsiCmd_H

#include "ScsiBase.h"

class ScsiCmd : public ScsiBase
{
public:
    bool executeCommand(int handle);
    void setCommand(sADDRESS lba, U32 sectorCount, eSCSICODE cmdCode);

public:
    eCMDERR getErrorStatus();

private:
    bool buildCommand();      // setting flags, ..
    void buildBasicCommand(); // Initialize basic data
};

#endif // ScsiCmd_H

