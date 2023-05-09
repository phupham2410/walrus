#include <sstream>
#include "StorageApi.h"
#include "SystemUtil.h"
#include "HexFrmt.h"

#include <fileapi.h>
#include <errhandlingapi.h>

#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string>

using namespace std;
using namespace StorageApi;

const string drvname = "\\\\.\\PhysicalDrive2";

#define DUMPERR(msg) \
    cout << msg << ". " \
         << SystemUtil::GetLastErrorString() << endl

void ClearPartTable() {
    HDL hdl;
    if (RET_OK != StorageApi::Open(drvname, hdl)) {
        DUMPERR("Cannot open drive"); return;
    }

    stringstream sstr;
    const int zsize = 4096; U8 zbuf[zsize] = {0};

    if (SetFilePointer((HANDLE)hdl, 0, NULL, 0) == INVALID_SET_FILE_POINTER) {
        DUMPERR("Cannot seek file pointer from start"); return;
    }

    WriteFile((HANDLE) hdl, zbuf, zsize, NULL, NULL);

    if (SetFilePointer((HANDLE)hdl, -zsize, NULL, 2) == INVALID_SET_FILE_POINTER) {
        DUMPERR("Cannot seek file pointer from end"); return;
    }

    WriteFile((HANDLE) hdl, zbuf, zsize, NULL, NULL);
}

int main(int argc, char** argv) {
    (void) argc; (void) argv;

    ClearPartTable();
    return 0;
}
