#include "SysHeader.h"
#include "StdHeader.h"
#include "SystemUtil.h"
#include "StorageApi.h"
#include "CloneUtil.h"

using namespace std;
using namespace StorageApi;
using namespace DiskCloneUtil;

#define DBGMODE 1

#define DUMPERR(msg) \
    cout << msg << ". " << SystemUtil::GetLastErrorString() << endl

static string GenScriptFileName() {
    return "sample_script.";
    return string(tmpnam(NULL));
}

// cmdstr: + a windows command: diskpart /s dpscript.scr
// cmdstr: + a windows command: powershell -command "ps command"
//         + a script file named <file>.cmd

static eRetCode ExecCommand(const string& cmdstr, string* rstr = NULL) {
    HANDLE cso_rd = NULL; // child std out handle
    HANDLE cso_wr = NULL; // child std out handle

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

    if (!status) {
        DUMPERR("Cannot create child process");
        return RET_FAIL;
    }

    // read output from pipe
    DWORD readsize;
    const U32 tmpsize = 1024; char tmp[tmpsize];

    if (rstr) {
        for (;;) {
            status = ReadFile(cso_rd, tmp, tmpsize, &readsize, NULL);
            if(!status || !readsize) break;

            *rstr += string((S8*) tmp, readsize);
        }
    }
    WaitForSingleObject( pi.hProcess, INFINITE );

    DWORD err = 0;
    GetExitCodeProcess( pi.hProcess, &err );
    if(err) {
        DUMPERR("Execute command fail");
        return RET_FAIL;
    }

    CloseHandle( pi.hProcess );
    return RET_OK;
}

static eRetCode ExecCommand(const vector<string>& cmdlist, string& rstr) {
    // create a script file:
    stringstream sstr;
    for(auto c : cmdlist) sstr << c << endl;

    string base = GenScriptFileName();
    string fname = "./" + base + "cmd";

    ofstream fstr; fstr.open (fname);
    fstr << sstr.str(); fstr.close();
    eRetCode ret = ExecCommand(fname, &rstr);
    remove(fname.c_str()); return ret;
}

static eRetCode ParseString_ShadowID(const string& str, string& result) {
    stringstream sstr(str);
    string line;
    const string prefix = "ShadowID = \"{";
    const string postfix = "}\"";
    while(getline(sstr, line, '\n')) {
        size_t pos0 = line.find(prefix);
        if (pos0 == string::npos) continue;
        pos0 += prefix.length();
        size_t pos1 = line.find(postfix);
        if (pos1 == string::npos) continue;
        result = line.substr(pos0, pos1 - pos0);
        return RET_OK;
    }
    return RET_NOT_FOUND;
}

static eRetCode ParseString_ShadowVol(const string& str, string& vol) {
    stringstream sstr(str);
    string line;
    const string prefix = "Shadow Copy Volume: ";
    while(getline(sstr, line, '\n')) {
        size_t pos0 = line.find(prefix);
        if (pos0 == string::npos) continue;
        pos0 += prefix.length();
        vol = line.substr(pos0);
        return RET_OK;
    }
    return RET_NOT_FOUND;
}

static eRetCode CreateShadowCopy(vector<sDcPartInfo>& parr) {
    for (U32 i = 0, maxi = parr.size(); i < maxi; i++) {
        sDcPartInfo& info = parr[i];
        if (!info.vi.valid) continue;
        sDcVolInfo& vi = info.vi;

        // create shadow copy
        do {
            stringstream sstr; string output, id;
            sstr << "wmic shadowcopy call create volume=" << vi.letter << ":\\";
            cout << "Executing command1: " << sstr.str() << endl;
            if (RET_OK != ExecCommand(sstr.str(), &output)) break;
            if (RET_OK != ParseString_ShadowID(output, vi.shaid)) break;
        } while(0);

        // get shadow volume
        do {
            stringstream sstr; string output, id;
            sstr << "vssadmin list shadows /shadow=\"{" << vi.shaid << "}\"";
            cout << "Executing command2: " << sstr.str() << endl;
            if (RET_OK != ExecCommand(sstr.str(), &output)) break;
            if (RET_OK != ParseString_ShadowVol(output, vi.shavol)) break;
            cout << "Result command2: " << vi.shavol << endl;
        } while(0);

        // remove old mount point
        do {
            stringstream sstr; string output, id;
            sstr << "rmdir " << vi.shalink << " /Q";
            cout << "Executing command3: " << sstr.str() << endl;
            if (RET_OK != ExecCommand(sstr.str(), NULL)) break;
        } while(0);

        // create mount point
        do {
            stringstream sstr; string output, id;
            sstr << "mklink /d " << vi.shalink << " " << vi.shavol << "\\";
            cout << "Executing command4: " << sstr.str() << endl;
            if (RET_OK != ExecCommand(sstr.str(), NULL)) break;
        } while(0);
    }

    return RET_OK;
}

int main(int argc, char **argv) {
    vector<sDcPartInfo> parr;

    if(0) {
        sDcPartInfo info;
        sDcVolInfo& vi = info.vi;
        vi.valid = true;
        vi.letter = 'C';
        vi.shalink = "C:\\sha_link_c";
        parr.push_back(info);
    }

    if(1) {
        sDcPartInfo info;
        sDcVolInfo& vi = info.vi;
        vi.valid = true;
        vi.letter = 'G';
        vi.shalink = "G:\\sha_link_d";
        parr.push_back(info);
    }

    if (RET_OK != CreateShadowCopy(parr)) {
        cout << "Cannot exec command" << endl; return 1;
    }
    return 0;
}

int main_backup(int argc, char **argv) {
    string output;

    string create_shadow_copy = "wmic shadowcopy call create volume=c:\\";
    string dircmd = "dir";

    if (RET_OK != ExecCommand(create_shadow_copy, &output)) {
        cout << "Cannot exec command" << endl; return 1;
    }
    string shadowid;
    if (RET_OK != ParseString_ShadowID(output, shadowid)) {
        cout << "Command return error" << endl; return 1;
    }
    cout << "Data read from child process: " << endl << shadowid << endl;


    return 0;
}
