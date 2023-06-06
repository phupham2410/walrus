#include "SysHeader.h"
#include "StdHeader.h"
#include "SystemUtil.h"
#include "StorageApi.h"

using namespace std;
using namespace StorageApi;

#define DBGMODE 1

#define DUMPERR(msg) \
    cout << msg << ". " << SystemUtil::GetLastErrorString() << endl

#define BUFSIZE 4096

HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;
HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;

HANDLE g_hInputFile = NULL;

void CreateChildProcess(void);
void WriteToPipe(void);
void ReadFromPipe(void);
void ErrorExit(const char* s);

// cmdstr: + a windows command: diskpart /s dpscript.scr
// cmdstr: + a windows command: powershell -command "ps command"
//         + a script file named <file>.cmd
static eRetCode ExecCommand(const string& cmdstr, U8* buffer, U32 bufsize, U32& datasize) {
    HANDLE cso_rd = NULL; // child std out handle
    HANDLE cso_wr = NULL; // child std out handle

    memset(buffer, 0x00, bufsize); datasize = 0;

    // Set the bInheritHandle flag so pipe handles are inherited.
    SECURITY_ATTRIBUTES sa; sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE; sa.lpSecurityDescriptor = NULL;

    // Create a pipe for the child process's STDOUT.
    if (!CreatePipe(&cso_rd, &cso_wr, &sa, 0) ) {
        DUMPERR("CreatePipe for child's stdout"); return RET_FAIL;
    }

    // Ensure the read handle to the pipe for STDOUT is not inherited.
    if (!SetHandleInformation(cso_rd, HANDLE_FLAG_INHERIT, 0)) {
        DUMPERR("SetHandleInformation for child's stdout"); return RET_FAIL;
    }

    // ----------------------------------------------------------------

    char cmdline[ MAX_PATH];
    sprintf( cmdline, "cmd.exe /c %s", cmdstr.c_str());

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

    eRetCode ret = RET_OK;

    if (!status) {
        DUMPERR("Cannot create child process");
    }
    else {
        // read output from pipe
        DWORD readsize; U32 remsize = bufsize;
        const U32 tmpsize = BUFSIZE; char tmp[tmpsize];

        for (;;) {
            BOOL trim = FALSE;
            status = ReadFile(cso_rd, tmp, tmpsize, &readsize, NULL);
            if(!status || !readsize) break;

            if (!remsize) {
                cout << "Input buffer not enough" << endl;
                ret = RET_OUT_OF_SPACE; break;
            }

            if (remsize < readsize) {
                readsize = remsize; trim = TRUE;
            }

            // memcpy into output buffer
            memcpy(buffer + datasize, tmp, readsize);
            datasize += readsize; remsize -= readsize;

            if (trim) {
                cout << "Data trimmed" << endl;
                ret = RET_OUT_OF_SPACE; break;
            }
        }
        WaitForSingleObject( pi.hProcess, INFINITE );

        DWORD err = 0;
        GetExitCodeProcess( pi.hProcess, &err );
        if(err) DUMPERR("Execute command fail");

        CloseHandle( pi.hProcess );
    }

    return ret;
}

static eRetCode ExecCommand(const string& cmdstr, string& output) {
    bool stop; eRetCode ret;
    U32 bufsize = 1024, datasize;
    do {
        U8* ptr  = new U8[bufsize]; stop = true;
        ret = ExecCommand(cmdstr, ptr, bufsize, datasize);
        if (ret == RET_OK) {
            output = string((S8*)ptr, datasize);
        }
        else if(ret == RET_OUT_OF_SPACE) {
            stop = false; bufsize *= 2;
        }
        delete[] ptr;
    } while(!stop);
    return ret;
}

static bool ParseString_ShadowID(const string& str, string& result) {
    stringstream sstr(str); string line;
    while(getline(sstr, line, '\n')) {
        size_t pos0 = line.find("ShadowID = \"{");
        if (pos0 == string::npos) continue;

        size_t pos1 = line.find("}\"");
        if (pos1 == string::npos) continue;

        result = line;
        return true;
    }
    return false;
}

int main(int argc, char **argv) {

    eRetCode ret;
    string output;

    string create_shadow_copy = "wmic shadowcopy call create volume=c:\\";
    string dircmd = "dir";

    ret = ExecCommand(create_shadow_copy, output);
    if (ret == RET_OK) {
        string shadowid;
        if (ParseString_ShadowID(output, shadowid)) {
            cout << "Data read from child process: " << endl;
            cout << shadowid << endl;
        }
    }
    else {
        cout << "Cannot exec command" << endl;
    }

    return 0;
}

// Create a child process that uses the previously created pipes for STDIN and STDOUT.
void CreateChildProcess() {
    TCHAR szCmdline[]=TEXT("child");
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    BOOL bSuccess = FALSE;

    // Set up members of the PROCESS_INFORMATION structure.
    ZeroMemory( &pi, sizeof(PROCESS_INFORMATION) );

    // Set up members of the STARTUPINFO structure.
    // This structure specifies the STDIN and STDOUT handles for redirection.
    ZeroMemory( &si, sizeof(STARTUPINFO) );
    si.cb = sizeof(STARTUPINFO);
    si.hStdError = g_hChildStd_OUT_Wr;
    si.hStdOutput = g_hChildStd_OUT_Wr;
    si.hStdInput = g_hChildStd_IN_Rd;
    si.dwFlags |= STARTF_USESTDHANDLES;

    // Create the child process.
    bSuccess = CreateProcess(NULL, szCmdline, // command line
                             NULL,            // process security attributes
                             NULL,            // primary thread security attributes
                             TRUE,            // handles are inherited
                             0,               // creation flags
                             NULL,            // use parent's environment
                             NULL,            // use parent's current directory
                             &si,             // STARTUPINFO pointer
                             &pi);    // receives PROCESS_INFORMATION

    // If an error occurs, exit the application.
    if ( ! bSuccess ) {
        ErrorExit("CreateProcess");
    }
    else {
        // Close handles to the child process and its primary thread.
        // Some applications might keep these handles to monitor the status
        // of the child process, for example.
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        // Close handles to the stdin and stdout pipes no longer needed by the child process.
        // If they are not explicitly closed, there is no way to recognize that the child process has ended.
        CloseHandle(g_hChildStd_OUT_Wr);
        CloseHandle(g_hChildStd_IN_Rd);
    }
}

// Read from a file and write its contents to the pipe for the child's STDIN.
// Stop when there is no more data.
void WriteToPipe(void) {
    DWORD dwRead, dwWritten;
    CHAR chBuf[BUFSIZE];
    BOOL bSuccess = FALSE;

    for (;;) {
        bSuccess = ReadFile(g_hInputFile, chBuf, BUFSIZE, &dwRead, NULL);
        if ( ! bSuccess || dwRead == 0 ) break;

        bSuccess = WriteFile(g_hChildStd_IN_Wr, chBuf, dwRead, &dwWritten, NULL);
        if ( ! bSuccess ) break;
    }

    // Close the pipe handle so the child process stops reading.
    if ( ! CloseHandle(g_hChildStd_IN_Wr) ) {
        ErrorExit("StdInWr CloseHandle");
    }
}

// Read output from the child process's pipe for STDOUT
// and write to the parent process's pipe for STDOUT.
// Stop when there is no more data.
void ReadFromPipe(void) {
    DWORD dwRead, dwWritten;
    CHAR chBuf[BUFSIZE];
    BOOL bSuccess = FALSE;
    HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

    for (;;) {
        bSuccess = ReadFile( g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
        if( ! bSuccess || dwRead == 0 ) break;

        bSuccess = WriteFile(hParentStdOut, chBuf, dwRead, &dwWritten, NULL);
        if (! bSuccess ) break;
    }
}

// Format a readable error message, display a message box,
// and exit from the application.
void ErrorExit(const char* lpszFunction) {
    printf("ErrorExit: %s\n", lpszFunction);
    ExitProcess(1);
}
