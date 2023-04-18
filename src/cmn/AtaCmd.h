#ifndef AtaCmd_H
#define AtaCmd_H

#include "AtaBase.h"

class AtaCmd : public AtaBase
{
public:
    bool executeCommand(tHdl handle);
    void setCommand(sADDRESS lba, U32 sectorCount, eATACODE cmdCode);

public:
    eCMDERR getErrorStatus();

private:
    bool buildCommand();      // setting flags, ..
    void buildBasicCommand(); // Initialize basic data
};

class AtaCmdExt : public AtaBase
{
public:
    bool executeCommand(tHdl handle);
    void setCommand(sADDRESS lba, U32 sectorCount, eATACODE cmdCode);

public:
    eCMDERR getErrorStatus();

private:
    bool buildCommand();      // setting flags, ..
    void buildBasicCommand(); // Initialize basic data
};

class AtaCmdDirect : public AtaBase
{
public:
    bool executeCommand(tHdl handle);
    void setCommand(sADDRESS lba, U32 sectorCount, eATACODE cmdCode);

public:
    eCMDERR getErrorStatus();

private:
    bool buildCommand();      // setting flags, ..
    void buildBasicCommand(); // Initialize basic data
};

class AtaCmdScsi : public AtaBase
{
public:
    bool executeCommand(tHdl handle);
    void setCommand(sADDRESS lba, U32 sectorCount, eATACODE cmdCode);

public:
    eCMDERR getErrorStatus();

private:
    bool buildCommand();      // setting flags, ..
    void buildBasicCommand(); // Initialize basic data
};

#endif // AtaCmd_H

