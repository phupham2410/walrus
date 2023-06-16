#include "StdHeader.h"
#include "DeviceMgr.h"
#include "NvmeUtil.h"
#include<iostream>
#include <stdlib.h>
#include <stddef.h>
#include <tchar.h>
#include <strsafe.h>
#include <io.h>
#include <lmcons.h>
#include <intsafe.h>
#include <windows.h>
#include <windows.h>
#include <ntdddisk.h>
#include <ntddscsi.h>
#include <processthreadsapi.h>

#include "CloneUtil.h"
#include "StorageApi.h"

using namespace std;
using namespace StorageApi;
using namespace DiskCloneUtil;

struct sWorkerParam {
    string srcdrv; // source physical drive
    string dstdrv; // target physical drive
    tAddrArray parr;
    volatile sProgress* prog; // clone progress
};

DWORD WINAPI WorkerThreadFunc(void* param) {
    sWorkerParam* wp = (sWorkerParam*) param;
    StorageApi::CloneDrive(wp->dstdrv, wp->srcdrv, wp->parr, wp->prog);

    // // sample code on progress
    // volatile sProgress* p = wp->prog;
    // p->workload = 1000; p->progress = 0;
    // p->stop = false; p->ready = true;
    // for (U32 i = 0; i < p->workload; i++) {
    //     p->progress++; Sleep(300);
    // }
    // p->done = true; p->rval = RET_OK;

    return 0;
}

static sWorkerParam wp;

eRetCode StartWorkerThread(volatile sProgress* prog)
{
    // Setting source and target drives
    string srcdrv = "\\\\.\\PhysicalDrive0";
    string dstdrv = "\\\\.\\PhysicalDrive2";

    // Select source partition number
    // This is partition_index_number, not index in the src list
    // use diskpart command to get value of partition index
    U32 part_num[] = { 4, 2, 6 };
    U32 part_cnt = sizeof(part_num) / sizeof(part_num[0]);

    // User may increase size of partition on target drive
    // this code add 100MB to partition size
    U64 extsize = 100 << 20;


    // Build partition param
    tDriveArray darr;
    if (RET_OK != ScanDrive(darr)) {
        cout << "Cannot scan drives " << endl;
        return RET_FAIL;
    }

    // select src & dst drive from the list
    sDriveInfo *si = NULL, *di = NULL;
    for (U32 i = 0, maxi = darr.size(); i < maxi; i++) {
        sDriveInfo* d = &darr[i];
        if (d->name == srcdrv) si = d;
        else if (d->name == dstdrv) di = d;
        if (si && di) break;
    }

    if (!si || !di) {
        cout << "source / dest drives not found" << endl;
        return RET_FAIL;
    }

    // build the address list of selected partitions
    tAddrArray parr;
    for (U32 i = 0; i < part_cnt; i++) {
        U32 cur_num = part_num[i];

        for (U32 j = 0, maxj = si->pi.parr.size(); j < maxj; j++) {
            sPartition& pi = si->pi.parr[j];
            if (pi.index != cur_num) continue;

            // increase part size in target drive
            tPartAddr pa = pi.addr; pa.second += extsize;
            parr.push_back(pa);

            if (1) {
                U64 offset = pa.first, length = pa.second;
                if (offset % 512) cout << "Invalid offset (odd) " << endl;
                if (length % 512) cout << "Invalid length (odd) " << endl;

                cout << "Selecting partition " << cur_num << " ";
                cout << "offset: " << (offset >> 9) << " (lbas) ";
                cout << "length: " << (length >> 9) << " (sectors) "
                                   << (length >> 20) << " (MB) ";
                cout << endl;
            }

            break;
        }
    }

    // Start thread to run disk clone:
    wp.parr = parr; wp.prog = prog;
    wp.srcdrv = srcdrv; wp.dstdrv = dstdrv;

    HANDLE hdl = CreateThread(NULL, 0, WorkerThreadFunc, &wp, 0, NULL);
    if (INVALID_HANDLE_VALUE == hdl) {
        cout << "cannot start worker thread" << endl;
        return RET_FAIL;
    }

    // return to manager thread.
    // child thread is working now
    return RET_OK;
}

struct sProgressBar {
    U32 minval, maxval, curval;
    U32 pcnt; // printed count / 100

    void setProgress(U32 cur) {
        curval = cur;
    }

    void setProgress(U32 min, U32 max) {
        minval = min; maxval = max; curval = 0; pcnt = 0;
    }

    void dumpProgress() {
        U32 curr = (curval - minval) * 100 / (maxval - minval);
        U32 diff = curr - pcnt; pcnt = curr;
        if (diff) cout << string(diff, '+');
    }
};

int main(int argc, char** argv) {
    sProgressBar bar;

    volatile sProgress cp; // clone-progress
    if (RET_OK != StartWorkerThread(&cp)) {
        cout << "Cannot start worker thread" << endl;
        return 1;
    }

    // waiting for ready
    while(!cp.ready);

    // since the workload info is very large (U64),
    // we need to normalize the range to U32

    U32 bitshift = 0;
    if ((cp.workload * 100) >> 30) {
        bitshift = 18;
    }

    bar.setProgress(0, cp.workload >> bitshift);

    // polling status
    U64 curr, prev = cp.progress >> bitshift;
    while(!cp.done) {
        curr = cp.progress >> bitshift;
        if (prev != curr) {
            prev = curr;
            bar.setProgress(curr);
            bar.dumpProgress();
        }

        // simulate the stop activity:
        bool stopreq = ((rand() % 2000000) == 99999);
        if (stopreq) {
            cp.stop = true;
            cout << endl << "User request to stop" << endl;
        }

        Sleep(500); // do something here
    }

    cout << endl << "Done!!!!!!!" << endl;
    return 0;
}
