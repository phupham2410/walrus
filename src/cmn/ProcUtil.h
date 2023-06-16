
#ifndef ProcUtil_H
#define ProcUtil_H

#include "StorageApi.h"

namespace ProcUtil {
    struct sCopyItemLog {
        std::string message;
        std::string errorid;
        std::string filename; // extracted from message
    };

    typedef std::map<std::string, std::vector<sCopyItemLog>> tCopyLogMap;
    typedef tCopyLogMap::iterator tCopyLogMapIter;
    typedef tCopyLogMap::const_iterator tCopyLogMapConstIter;
    typedef void(*tProcessLogFunc)(void*, const char*, unsigned int);

    void DumpCopyLog(const tCopyLogMap& rlog);
    StorageApi::U32 ParseCopyLog(const std::string& str, tCopyLogMap& result);

    StorageApi::eRetCode ExecCommand(const std::string& cmdstr,
                                    tProcessLogFunc func, void* priv,
                                    volatile StorageApi::U64* phdl = NULL);
    StorageApi::eRetCode ExecCommandOnly(const std::string& cmdstr);
    StorageApi::eRetCode ExecCommandList(const std::string& script);

    StorageApi::eRetCode ExecCommandWithLog(const std::string& cmdstr,
                                            std::string* rstr = NULL);
    StorageApi::eRetCode ExecDiskPartScript(const std::string& script);
    StorageApi::eRetCode ExecDiskUsageScript(const std::string& script,
        const std::string& logfile, std::string& output);
    StorageApi::eRetCode ExecCopyItemCommand(const std::string& cmdstr,
        tCopyLogMap* lmap = NULL, volatile StorageApi::U64* phdl = NULL);
}

#endif
