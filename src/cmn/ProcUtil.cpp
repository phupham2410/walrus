
#include "SystemUtil.h"
#include "SysHeader.h"
#include "CloneUtil.h"
#include "CoreUtil.h"
#include "FsUtil.h"
#include "ProcUtil.h"

using namespace std;
using namespace FsUtil;
using namespace ProcUtil;
using namespace StorageApi;
using namespace DiskCloneUtil;

#include <cstdio>
#include <string.h>
#include <diskguid.h>
#include <ntdddisk.h>
#include <processthreadsapi.h>

#define DBGMODE 0

// ----------------------------------------------------------------------------
// Temporaryly put here

#define INIT_PROGRESS(prog, load)     do { if (prog) prog->init(load); } while(0)
#define UPDATE_PROGRESS(prog, weight) do { if (prog) prog->progress += weight; } while(0)
#define CLOSE_PROGRESS(prog)          do { if (prog) prog->done = true; } while(0)
#define UPDATE_RETCODE(prog, code)    do { if (prog) prog->rval = code; } while(0)
#define UPDATE_INFO(prog, infostr)    do { if (prog) prog->info = infostr; } while(0)
#define FINALIZE_PROGRESS(prog, code) do { \
UPDATE_RETCODE(prog, code); CLOSE_PROGRESS(prog); } while(0)
#define RETURN_IF_STOP(prog, code)    do { if (prog && prog->stop) { \
    FINALIZE_PROGRESS(prog, code); return code; }} while(0)
#define FINALIZE_IF_STOP(prog, code)    do { if (prog && prog->stop) { \
    FINALIZE_PROGRESS(prog, code); return code; }} while(0)

#define SKIP_AND_CONTINUE(prog, weight) \
if (prog) { prog->progress += weight; continue; }

#define UPDATE_AND_RETURN_IF_STOP(p, w, ret) \
    UPDATE_PROGRESS(p, w); FINALIZE_IF_STOP(p, ret)

#define TRY_TO_EXECUTE_COMMAND(hdl, cmd) \
    (cmd.executeCommand((int) hdl) && (CMD_ERROR_NONE == cmd.getErrorStatus()))

#define SET_CURSTEP(p, v) if (p) do { p->priv.clone.curstep = v; } while(0)

// ----------------------------------------------------------------------------

eRetCode ProcUtil::ExecCommandOnly(const string& cmdstr) {
    const U32 bufsize = 4096; char cmdline[bufsize];
    sprintf( cmdline, "cmd.exe /c %s", cmdstr.c_str());

    if (DBGMODE) {
        cout << ">> Input: " << cmdstr << endl;
        cout << "## Execute command: " << cmdline << endl;
    }

    PROCESS_INFORMATION pi; memset( &pi, 0, sizeof(pi));
    STARTUPINFOA si; memset( &si, 0, sizeof(si)); si.cb = sizeof(si);

    DWORD err = 0; BOOL ret;
    ret = CreateProcessA( NULL, cmdline, NULL, NULL, FALSE,
        NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, NULL, NULL, &si, &pi );

    if (!ret) err = GetLastError();
    else {
        WaitForSingleObject( pi.hProcess, INFINITE );
        GetExitCodeProcess( pi.hProcess, &err );
        CloseHandle( pi.hProcess );
    }

    if (DBGMODE) {
        if( err )
            cout << "Execute command fail. Error " << err << endl;
    }

    return RET_OK;
}

static string GenScriptBaseName() {
    string tmp(tmpnam(NULL)); size_t len = tmp.length();
    for (U32 i = 0; i < len; i++) {
        char c = tmp[i];
        if (!isalnum(c)) tmp[i] = 'a' + (c % 24);
    }
    return tmp;
}

eRetCode ProcUtil::ExecCommandList(const std::string& script) {
    // Start child process
    // Execute shell script

    string base = GenScriptBaseName();
    string fname = "./" + base + ".cmd";
    ofstream fstr; fstr.open (fname);
    fstr << script << endl; fstr.close();
    ExecCommandOnly(fname); remove(fname.c_str());
    return RET_OK;
}

// Exec command in child process. Get output text in rstr
eRetCode ProcUtil::ExecCommand(const std::string& cmdstr,
                               tProcessLogFunc func, void* priv,
                               volatile StorageApi::U64* phdl) {

    HANDLE job = CreateJobObject(NULL, NULL);
    if (INVALID_HANDLE_VALUE == job) return RET_FAIL;

    HANDLE cso_rd = NULL; // child std out handle
    HANDLE cso_wr = NULL; // child std out handle

    // Set the bInheritHandle flag so pipe handles are inherited.
    SECURITY_ATTRIBUTES sa; sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE; sa.lpSecurityDescriptor = NULL;

    // Create a pipe for the child process's STDOUT.
    if (!CreatePipe(&cso_rd, &cso_wr, &sa, 0) ) {
        if (DBGMODE) cout << "Cannot create pipe" << endl;
        return RET_FAIL;
    }

    // Ensure the read handle to the pipe for STDOUT is not inherited.
    if (!SetHandleInformation(cso_rd, HANDLE_FLAG_INHERIT, 0)) {
        if (DBGMODE) cout << "SetHandleInformation fail" << endl;
        return RET_FAIL;
    }

    // ----------------------------------------------------------------
    const U32 bufsize = 4096; char cmdline[bufsize];
    sprintf( cmdline, "cmd.exe /c %s", cmdstr.c_str());

    if (DBGMODE) cout << "## Executing command: " << cmdline << endl;

    PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
    STARTUPINFOA si; memset( &si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.hStdError = cso_wr;
    si.hStdOutput = cso_wr;
    si.dwFlags |= STARTF_USESTDHANDLES;

    BOOL status;
    status = CreateProcessA( NULL, cmdline, NULL, NULL, TRUE,
                            NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, NULL, NULL, &si, &pi );

    // Close handles to the stdin and stdout pipes no longer needed by the child process.
    CloseHandle(cso_wr);

    if (!status) {
        if (DBGMODE) cout << "Cannot create child process" << endl;
        return RET_FAIL;
    }

    if (DBGMODE) cout << "#### Start copy-item process: pid " << pi.dwProcessId << endl;

    if (!AssignProcessToJobObject(job, pi.hProcess)) {
        if (DBGMODE) cout << "Cannot assign child process" << endl;
        TerminateJobObject(job, 1);
        CloseHandle(pi.hProcess); CloseHandle(job); return RET_FAIL;
    }

    if (phdl) *phdl = (U64) job;

    if (func) {
        DWORD readsize;
        const U32 tmpsize = 1024; char tmp[tmpsize];
        for (;;) {
            status = ReadFile(cso_rd, tmp, tmpsize, &readsize, NULL);
            if(!status || !readsize) break;
            func(priv, tmp, readsize);
        }
        if (DBGMODE) cout << "## Finish reading from child process" << endl;
    }
    WaitForSingleObject( pi.hProcess, INFINITE );

    DWORD err = 0;
    GetExitCodeProcess( pi.hProcess, &err );
    // if(err) {
    //     if (DBGMODE)
    //         cout << "Execute command fail: "
    //             << SystemUtil::GetLastErrorString() << endl;
    //     return RET_FAIL;
    // }

    CloseHandle( pi.hProcess );
    TerminateJobObject(job, 1); CloseHandle(job);
    return RET_OK;
}

static void AppendLogHandle(void* priv, const char* buffer, unsigned int buffsize) {
    if (!priv) return;
    string* str = (string*) priv;
    *str += string(buffer, buffsize);
}

// Exec command in child process. Get output text in rstr
eRetCode ProcUtil::ExecCommandWithLog(const string& cmdstr, string* rstr) {
    return ExecCommand(cmdstr, AppendLogHandle, (void*) rstr);
}

eRetCode ProcUtil::ExecDiskPartScript(const std::string& script) {
    // Start child process
    // Execute diskpart /s script
    string base = GenScriptBaseName();
    string lname = "./" + base + ".log";
    string fname = "./" + base + ".dp";
    ofstream fstr; fstr.open (fname);
    fstr << script << endl; fstr.close();
    stringstream cstr; cstr << "diskpart /s " << fname;
    if (lname.length()) cstr << " > " << lname;
    string cmd = cstr.str();
    ExecCommandOnly(cmd);
    remove(fname.c_str()); remove(lname.c_str());
    return RET_OK;
}

eRetCode ProcUtil::ExecDiskUsageScript(const std::string& script, const std::string& logfile, string& output) {
    // Start child process
    // Execute script, save output to log file, then get its content
    // Why dont use the pipe version here ? --> for simplicity
    ExecCommandOnly(script + " > " + logfile);
    ifstream fstr; fstr.open(logfile, ifstream::in);
    output.assign(istreambuf_iterator<char>(fstr), (istreambuf_iterator<char>()));
    if (DBGMODE) cout << output << endl;
    return RET_OK;
}

void ProcUtil::DumpCopyLog(const tCopyLogMap& rlog) {
    if (DBGMODE) {
        for (auto &rl : rlog) {
            cout << "Error: " << rl.first << endl;
            for (auto &it : rl.second) {
                if (it.filename.length()) cout << "   + File: " << it.filename;
                else cout << "   + Message: " << it.message;
                cout << endl;
            }
        }
    }
}

U32 ProcUtil::ParseCopyLog(const string& str, tCopyLogMap& result) {
    result.clear();

    string log = str; string delim = "\r\n";
    for (char c : delim)
        log.erase(std::remove(log.begin(), log.end(), c), log.end());

    const string pre0 = "Copy-Item : ";
    const string pst0 = "At line:";
    const string pre1 = "+ FullyQualifiedErrorId : ";
    const string pst1 = ",Microsoft.PowerShell.Commands";
    size_t len0 = pre0.length(), len1 = pre1.length();
    size_t head = 0, tail, nexthead = 0;
    while((head = log.find(pre0, head)) != string::npos) {
        sCopyItemLog item;

        head += len0;
        if((tail = log.find(pst0, head)) == string::npos) continue;
        item.message = log.substr(head, tail - head);

        if((head = log.find(pre1, tail)) == string::npos) continue;
        head += len1;

        if((tail = log.find(pst1, head)) == string::npos) continue;
        item.errorid = log.substr(head, tail - head);

        do {
            // extract filename from message (if any)
            size_t pos0 = 0, pos1; string& msg = item.message;
            if ((pos0 = msg.find("'", pos0 + 0)) == string::npos) break;
            if ((pos1 = msg.find("'", pos0 + 1)) == string::npos) break;
            item.filename = msg.substr(pos0, pos1 - pos0);
        } while(0);

        result[item.errorid].push_back(item);
        head = tail + pst1.length(); nexthead = head - 1;
    }
    return nexthead; // next search position in input string
}


struct sCopyParam {
    string data;
    tCopyLogMap* lmap;
};

static void CopyLogHandle(void* priv, const char* buffer, unsigned int buffsize) {
    if (!priv) return;
    sCopyParam* param = (sCopyParam*) priv;
    if (!param->lmap) return;
    string& res = param->data;
    tCopyLogMap& rlog = *param->lmap;
    res += string(buffer, buffsize);
    U32 nextpos = ParseCopyLog(res, rlog);
    res = res.substr(nextpos); DumpCopyLog(rlog);
}

// Exec command in child process. Get output text in rstr
eRetCode ProcUtil::ExecCopyItemCommand(const string& cmdstr, tCopyLogMap* lmap, volatile U64* phdl) {
    sCopyParam param; param.lmap = lmap; param.data = "";
    return ExecCommand(cmdstr, CopyLogHandle, (void*)(&param), phdl);
}
