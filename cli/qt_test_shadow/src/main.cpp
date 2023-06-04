#include "SysHeader.h"
#include "StdHeader.h"

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

int main(int argc, char **argv) {
    SECURITY_ATTRIBUTES sa;

    printf("\n->Start of parent execution.\n");

    // Set the bInheritHandle flag so pipe handles are inherited.
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    // Create a pipe for the child process's STDOUT.
    if ( ! CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &sa, 0) ) {
        ErrorExit("StdoutRd CreatePipe");
    }

    // Ensure the read handle to the pipe for STDOUT is not inherited.
    if ( ! SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) {
        ErrorExit("Stdout SetHandleInformation");
    }

    // Create a pipe for the child process's STDIN.
    if (! CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &sa, 0)) {
        ErrorExit("Stdin CreatePipe");
    }

    // Ensure the write handle to the pipe for STDIN is not inherited.
    if ( ! SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0) ) {
        ErrorExit("Stdin SetHandleInformation");
    }

    // Create the child process.
    CreateChildProcess();

    // Get a handle to an input file for the parent.
    // This example assumes a plain text file and uses string output to verify data flow.
    if (argc == 1) {
        ErrorExit("Please specify an input file.\n");
    }

    g_hInputFile = CreateFileA(argv[1], GENERIC_READ,
                       0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);

    if ( g_hInputFile == INVALID_HANDLE_VALUE ) {
        ErrorExit("CreateFile");
    }

    // Write to the pipe that is the standard input for a child process.
    // Data is written to the pipe's buffers, so it is not necessary to wait
    // until the child process is running before writing data.
    WriteToPipe();
    printf( "\n->Contents of %S written to child STDIN pipe.\n", argv[1]);

    // Read from pipe that is the standard output for child process.
    printf( "\n->Contents of child process STDOUT:\n\n");
    ReadFromPipe();
    printf("\n->End of parent execution.\n");

    // The remaining open handles are cleaned up when this process terminates.
    // To avoid resource leaks in a larger application, close handles explicitly.

    return 0;
}

// Create a child process that uses the previously created pipes for STDIN and STDOUT.
void CreateChildProcess() {
    TCHAR szCmdline[]=TEXT("child");
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO si;
    BOOL bSuccess = FALSE;

    // Set up members of the PROCESS_INFORMATION structure.
    ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );

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
                             &piProcInfo);    // receives PROCESS_INFORMATION

    // If an error occurs, exit the application.
    if ( ! bSuccess ) {
        ErrorExit("CreateProcess");
    }
    else {
        // Close handles to the child process and its primary thread.
        // Some applications might keep these handles to monitor the status
        // of the child process, for example.
        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);

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
