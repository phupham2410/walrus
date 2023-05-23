#include "cmn/StdHeader.h"
#include "cmn/DeviceMgr.h"
#include "cmn/NvmeUtil.h"
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
//#include "cmn/winioctl.h"

#define DRIVENAME "\\\\.\\PhysicalDrive2"


void *calloc_aligned(size_t num, size_t size, size_t alignment)
{
    //call malloc aligned and memset
    void *zeroedMem = NULL;
    size_t numSize = num * size;
    if (numSize)
    {
        zeroedMem = _aligned_malloc(numSize, alignment);

        if (zeroedMem)
        {
            memset(zeroedMem, 0, numSize);
        }
    }
    return zeroedMem;
}

void calloc_free(void * zeroedMem ) {
    _aligned_free(zeroedMem);
}

void print_Windows_Error_To_Screen(unsigned int windowsError) {
    TCHAR *windowsErrorString = NULL;
    //Originally used: MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)
    //switched to MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US) to keep output consistent with all other verbose output.-TJE
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL, windowsError, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), ((TCHAR*)&windowsErrorString), 0, NULL);
    //This cast is necessary to tell the Windows API to allocate the string, but it is necessary. Without it, this will not work.
    _tprintf_s(TEXT("%u - %s\n"), windowsError, windowsErrorString);
    LocalFree(windowsErrorString);
}


void doFWDownloadFull(HANDLE hHandle) {
    STORAGE_HW_FIRMWARE_INFO fwdlInfo;
    //LPCWSTR fileName = L"D:/FromHiptech/EIFM31.6.bin"; // TODO - change file here
    LPCSTR fileName = "D:/FromHiptech/FW Bin file/Force MP600/EGFM15.1.bin";
    int64_t transferSize = 4096;
    BYTE slotId;
    if (!NvmeUtil::win10FW_GetfwdlInfoQuery(hHandle, &fwdlInfo, TRUE, &slotId)) {
        std::cout << "win10FW_GetfwdlInfoQuery failed:";
        print_Windows_Error_To_Screen(GetLastError());
        return;
    }

    if (fwdlInfo.ImagePayloadAlignment !=0 ) {
        transferSize = fwdlInfo.ImagePayloadAlignment;
    }
    std::cout << "====== FW download: ==========\n";
#if 1
    if (!NvmeUtil::win10FW_TransferFile(hHandle, &fwdlInfo, slotId, fileName, transferSize)) {
        std::cout << "fwWin10Download failed:";
        print_Windows_Error_To_Screen(GetLastError());
        return;
    }

    std::cout << "====== FW Activate: ==========\n";

    if (!NvmeUtil::win10FW_Active(hHandle,   &fwdlInfo, slotId)) {
        std::cout << "FW download failed at activate:";
        print_Windows_Error_To_Screen(GetLastError());
        return;
    }
#endif
    std::cout << "FW download done\n";
}

int main_ref(int argc, char** argv) {
    tHdl iHandle;
    HANDLE hHandle;
    bool status;

    status = DeviceMgr::OpenDevice(DRIVENAME, iHandle);
    if (!status) {
        goto exit_error;
    }

    hHandle = (HANDLE)iHandle;

    //identifyNamespace(hHandle);
    //identifyController(hHandle);
    //getHealthInfoLog(hHandle);
    //doSelftest(hHandle);
    //identifyNSIDList(hHandle);

    doFWDownloadFull(hHandle);

    //reportLuns(hHandle);

    if (hHandle != INVALID_HANDLE_VALUE) {
        DeviceMgr::CloseDevice(iHandle);
    }
    return 0;
exit_error:
    print_Windows_Error_To_Screen(GetLastError());
    std::cout << "Error exit" << std::endl;
    return -1;
}
