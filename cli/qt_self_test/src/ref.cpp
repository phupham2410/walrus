#include "StdHeader.h"
#include "DeviceMgr.h"
#include "NvmeUtil.h"
#include "SysHeader.h"
#include "SystemUtil.h"
#include <stddef.h>
#include <tchar.h>
#include <strsafe.h>
#include <io.h>
#include <lmcons.h>
#include <intsafe.h>

using namespace std;

#define DRIVENAME "\\\\.\\PhysicalDrive2"

string ToString(const NVME_DEVICE_SELF_TEST_LOG& stl, bool showres = false) {
    stringstream sstr;
    sstr << "CurrentOperation.Status: " << (U32)stl.CurrentOperation.Status << endl;
    sstr << "CurrentCompletion: " << (U32)stl.CurrentCompletion.CompletePercent;

    if (showres) {
        sstr << endl;
        sstr << "Newest.Result[0].Status.Result: " << (U32)stl.ResultData[0].Status.Result << endl;
        sstr << "Newest.Result[0].Status.CodeValue: " << (U32)stl.ResultData[0].Status.CodeValue;
    }

    return sstr.str();
}

void doSelftest(HANDLE hdl) {
    eSelfTestCode testcode = STC_EXTENDED;

    NVME_DEVICE_SELF_TEST_LOG stl;

    cout << " - Current status:" << endl;
    if(NvmeUtil::GetSelfTestLog(hdl, &stl)) cout << ToString(stl) << endl;
    else {
        cout << " ===> Fail to read intial selftest log" << endl; return;
    }

    if (stl.CurrentOperation.Status !=0) {
        cout << "===> Another self-test is in-progress" << endl; return;
    }

    cout << " - Start extended test " << endl;
    if(!NvmeUtil::DeviceSelfTest(hdl, testcode)) {
        cout << "===> Fail to start sefl test" << endl; return;
    }

    while(true){
        cout << " ***********************" << endl;
        cout << " - Get new status:" << endl;
        if(NvmeUtil::GetSelfTestLog(hdl, &stl)) cout << ToString(stl, false);
        else {
            cout << "===> Fail to read selftest log for re-check" << endl; return;
        }

        // if a extended test is under ongoing - contnue
        if (stl.CurrentOperation.Status == testcode) {
            // do nothing
        } else if (stl.CurrentOperation.Status == 0x0) {
            cout << "Self-test done" << endl; break;
        }

        Sleep(5000); // Sleep 5 seconds
    }
}

int main_ref(int argc, char** argv) {
    tHdl hdl;
    if (!DeviceMgr::OpenDevice(DRIVENAME, hdl)) {
        cout << SystemUtil::GetLastErrorString() << endl; return 1;
    }
    doSelftest((HANDLE)hdl);
    DeviceMgr::CloseDevice(hdl);
    return 0;
}
