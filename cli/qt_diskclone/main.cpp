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
    return 0;
}

static sWorkerParam wp;

static eRetCode BuildPartitionInfo(
    const string& srcdrv, const string& dstdrv,
    const vector<U32>& partnum, U64 extsize,
    tAddrArray& parr) {

    parr.clear();

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
    for (U32 i = 0, maxi = partnum.size(); i < maxi; i++) {
        U32 pnum = partnum[i];
        for (U32 j = 0, maxj = si->pi.parr.size(); j < maxj; j++) {
            sPartition& pi = si->pi.parr[j];
            if (pi.index != pnum) continue;

            // increase part size in target drive
            tPartAddr pa = pi.addr; pa.second += extsize;
            parr.push_back(pa);

            if (1) {
                U64 offset = pa.first, length = pa.second;
                if (offset % 512) cout << "Invalid offset (odd) " << endl;
                if (length % 512) cout << "Invalid length (odd) " << endl;

                cout << "Selecting partition " << pnum << " ";
                cout << "offset: " << (offset >> 9) << " (lbas) ";
                cout << "length: " << (length >> 9) << " (sectors) "
                     << (length >> 20) << " (MB) ";
                cout << endl;
            }

            break;
        }
    }
    return RET_OK;
}

eRetCode StartWorkerThread(volatile sProgress* prog)
{
    // Setting source and target drives
    string srcdrv = "\\\\.\\PhysicalDrive0";
    string dstdrv = "\\\\.\\PhysicalDrive2";

    // Select source partition number
    // use diskpart command to get value of partition index
    // Note: this is partition_index_number, not the index in the scanned list
    vector<U32> partnum = { 4, 2, 6 };

    // Extend sizes of all partitions
    // User may increase the size of partition on target drive
    // This code add 100MB to partition size
    U64 extsize = 100 << 20;

    tAddrArray parr;
    if (RET_OK != BuildPartitionInfo(srcdrv, dstdrv, partnum, extsize, parr)) {
        cout << "cannot build partition info" << endl;
        return RET_FAIL;
    }

    // Start thread to run disk clone:
    wp.srcdrv = srcdrv; wp.dstdrv = dstdrv; wp.parr = parr; wp.prog = prog;
    HANDLE hdl = CreateThread(NULL, 0, WorkerThreadFunc, &wp, 0, NULL);
    if (INVALID_HANDLE_VALUE == hdl) {
        cout << "cannot start worker thread" << endl;
        return RET_FAIL;
    }

    // return to manager thread. Child thread is still working now
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
        if (diff) {
            cout << "\rProgress ["
                 << string(pcnt, '>') << string(100 - pcnt, '_')
                 << "] " << pcnt << "% "
                 << "(" << minval << "/" << maxval << "/" << curval << ")";
        }
    }
};

volatile sProgress cp; // clone-progress

int main(int argc, char** argv) {
    sProgressBar bar;

    if (RET_OK != StartWorkerThread(&cp)) {
        cout << "Cannot start worker thread" << endl;
        return 1;
    }

    // waiting for ready
    while(!cp.ready);

    // since the workload info is very large U48
    // we need to normalize the range to U32 (U30 for safe)
    U32 bitshift = 0, maxwl = (1 << 30);
    if (cp.workload > maxwl) {
        bitshift = log2(cp.workload - maxwl);
    }

    bar.setProgress(0, cp.workload >> bitshift);

    // polling status
    U32 curr, prev = cp.progress >> bitshift;

    while(1) {
        U64 wl = cp.workload;
        U64 pr = cp.progress;

        if (pr == wl) {
            pr = wl;
        }

        curr = pr >> bitshift;
        if (prev != curr) {
            prev = curr;
            bar.setProgress(curr);
            bar.dumpProgress();
        }

        // randomize the stop activity:
        bool stopreq = ((rand() % 99) == 99999);
        if (stopreq) {
            cp.stop = true;
            cout << endl << "User request to stop" << endl;
        }

        if (cp.done) break;
        Sleep(500); // do something here
    }

    cout << endl << "Done!" << endl;
    return 0;
}
