
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string>

#define wszVolumeSrc L"\\\\.\\F:"
#define wszVolumeDest L"\\\\.\\G:"

static HANDLE OpenDisk(LPWSTR wszPath) {
    HANDLE hDevice = INVALID_HANDLE_VALUE;  // handle to the drive to be examined
    BOOL bResult   = FALSE;                 // results flag
    DWORD junk     = 0;                     // discard results

    hDevice = CreateFile(wszPath,          // drive to open
                         GENERIC_WRITE | GENERIC_READ, //FILE_ALL_ACCESS,,                // no access to the drive
                         FILE_SHARE_READ | // share mode
                             FILE_SHARE_WRITE,
                         NULL,             // default security attributes
                         OPEN_EXISTING,    // disposition
                         0,                // file attributes
                         NULL);            // do not copy file attributes

    if (hDevice == INVALID_HANDLE_VALUE)    // cannot open the drive
    {
        return (INVALID_HANDLE_VALUE);
    }

    return hDevice;

}

static BOOL UnmountVolume(HANDLE hDrive)
{
    DWORD size;

    if (!DeviceIoControl(hDrive, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &size, NULL)) {
        printf("Could not unmount drive: 0x%08lX\n", GetLastError());
        return FALSE;
    }
    return TRUE;
}

#define MB_SIZE 1024*1024
void copy( HANDLE hDeviceSrc,  HANDLE hDeviceDest, LONGLONG totalBytes ) {
    DWORD read_bytes = 0;
    LONGLONG current_offset = 0;
    DWORD write_bytes;
    uint8_t buf[4096];
    DWORD sector_size = sizeof(buf);
    while(current_offset < totalBytes) {
        if (!ReadFile(hDeviceSrc, buf, sector_size, &read_bytes, NULL)) {
            printf("Read error: 0x%08lX", GetLastError());
            return;
        }
        if (read_bytes != sector_size) {
            printf("Read not enough: %d", read_bytes);
            return;
        }

        if (!WriteFile(hDeviceDest, buf, sector_size, &write_bytes, NULL)) {
            printf("Write error: 0x%08lX", GetLastError());
            return;
        }

        if (write_bytes != sector_size) {
            printf("Write not enough: %d", write_bytes);
            return;
        }
        current_offset+=sector_size;
        if (current_offset % (MB_SIZE * 32) == 0) {
            printf("Copied %d MBytes\n", int((current_offset/MB_SIZE)));
        }
    }
}

void CopyDisk() {
    HANDLE hDeviceSrc = OpenDisk((LPWSTR)wszVolumeSrc);
    HANDLE hDeviceDest = OpenDisk((LPWSTR)wszVolumeDest);
    DWORD rs = 0;
    uint64_t rb;
    uint8_t buf[4096];
    PARTITION_INFORMATION_EX pdg;
    LARGE_INTEGER totalBytes;
    BOOL r;

    if (!UnmountVolume(hDeviceDest)) {
        goto out;
    }

    r = DeviceIoControl(hDeviceSrc, IOCTL_DISK_GET_PARTITION_INFO_EX,
                        NULL, 0, &pdg, sizeof(pdg), &rs, NULL);

    if (r && rs > 0) {
        printf("size is %llu bytes\n",pdg.PartitionLength.QuadPart);
        totalBytes = pdg.PartitionLength;
    } else {
        printf("error when get size: 0x%08lX\n", GetLastError());
        goto out;
    }

    copy(hDeviceSrc, hDeviceDest, totalBytes.QuadPart);


out:
    CloseHandle(hDeviceSrc);
    CloseHandle(hDeviceDest);
}

int main(int argc, char** argv) {
    CopyDisk();
    return 0;
}
