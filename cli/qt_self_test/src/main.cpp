#include "StorageApi.h"
#include "SystemUtil.h"
#include "DeviceMgr.h"
#include "CommonUtil.h"
#include "SysHeader.h"
#include "StdHeader.h"
#include "NvmeUtil.h"

using namespace std;
using namespace StorageApi;

const string PHYDRV0 = "\\\\.\\PhysicalDrive0";
const string PHYDRV1 = "\\\\.\\PhysicalDrive1";
const string PHYDRV2 = "\\\\.\\PhysicalDrive2";

const string drvname = PHYDRV2;

#define DUMPERR(msg) cout << msg << ". " << SystemUtil::GetLastErrorString() << endl

int main(int argc, char** argv) {
    return 0;
}
