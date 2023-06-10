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

int main(int argc, char **argv) {
    DiskCloneUtil::TestMisc();
    return 0;
}
