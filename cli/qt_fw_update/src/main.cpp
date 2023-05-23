#include "StorageApi.h"
#include "SystemUtil.h"
#include "HexFrmt.h"
#include "CommonUtil.h"
#include "SysHeader.h"
#include "StdHeader.h"

using namespace std;
using namespace StorageApi;

const string PHYDRV0 = "\\\\.\\PhysicalDrive0";
const string PHYDRV1 = "\\\\.\\PhysicalDrive1";
const string PHYDRV2 = "\\\\.\\PhysicalDrive2";

const string drvname = PHYDRV2;

#define DUMPERR(msg) \
    cout << msg << ". " \
         << SystemUtil::GetLastErrorString() << endl

int main(int argc, char** argv) {
    (void) argc; (void) argv;

    // TestFileSeek();
    // TestReadDriveSize();
    // ClearPartTable();
    // WriteFullDisk();
    // TestReadSector();
    // TestInitDriveMRB();
    // TestInitDriveGPT();
    // TestGetDriveLayout();
    // TestSetDriveLayoutMRB();
    // TestSetDriveLayoutGPT();
    // TestDiskPart_GenScript();

    // TestDiskClone();

    // TestReadEfi();

    return 0;
}
