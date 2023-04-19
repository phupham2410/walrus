#ifndef NvmeUtil_H
#define NvmeUtil_H

#include "StdMacro.h"
#include "StdHeader.h"
#include "CmdBase.h"

#include <windows.h>
#include <winioctl.h>
#include <nvme.h>

// Adding define from latest winsdk's winioctl
// Mingw is not yet up to date
#ifndef STORAGE_PROTOCOL_COMMAND
//
// Parameter for IOCTL_STORAGE_PROTOCOL_COMMAND
// Buffer layout: <STORAGE_PROTOCOL_COMMAND> <Command> [Error Info Buffer] [Data-to-Device Buffer] [Data-from-Device Buffer]
//
#define STORAGE_PROTOCOL_STRUCTURE_VERSION              0x1

typedef struct _STORAGE_PROTOCOL_COMMAND {

    DWORD   Version;                        // STORAGE_PROTOCOL_STRUCTURE_VERSION
    DWORD   Length;                         // sizeof(STORAGE_PROTOCOL_COMMAND)

    STORAGE_PROTOCOL_TYPE  ProtocolType;
    DWORD   Flags;                          // Flags for the request

    DWORD   ReturnStatus;                   // return value
    DWORD   ErrorCode;                      // return value, optional

    DWORD   CommandLength;                  // non-zero value should be set by caller
    DWORD   ErrorInfoLength;                // optional, can be zero
    DWORD   DataToDeviceTransferLength;     // optional, can be zero. Used by WRITE type of request.
    DWORD   DataFromDeviceTransferLength;   // optional, can be zero. Used by READ type of request.

    DWORD   TimeOutValue;                   // in unit of seconds

    DWORD   ErrorInfoOffset;                // offsets need to be pointer aligned
    DWORD   DataToDeviceBufferOffset;       // offsets need to be pointer aligned
    DWORD   DataFromDeviceBufferOffset;     // offsets need to be pointer aligned

    DWORD   CommandSpecific;                // optional information passed along with Command.
    DWORD   Reserved0;

    DWORD   FixedProtocolReturnData;        // return data, optional. Some protocol, such as NVMe, may return a small amount data (DWORD0 from completion queue entry) without the need of separate device data transfer.
    DWORD   Reserved1[3];

    _Field_size_bytes_full_(CommandLength) BYTE  Command[ANYSIZE_ARRAY];

} STORAGE_PROTOCOL_COMMAND, *PSTORAGE_PROTOCOL_COMMAND;

//
// Bit-mask values for STORAGE_PROTOCOL_COMMAND - "Flags" field.
//
#define STORAGE_PROTOCOL_COMMAND_FLAG_ADAPTER_REQUEST    0x80000000     // Flag indicates the request targeting to adapter instead of device.

//
// Status values for STORAGE_PROTOCOL_COMMAND - "ReturnStatus" field.
//
#define STORAGE_PROTOCOL_STATUS_PENDING                 0x0
#define STORAGE_PROTOCOL_STATUS_SUCCESS                 0x1
#define STORAGE_PROTOCOL_STATUS_ERROR                   0x2
#define STORAGE_PROTOCOL_STATUS_INVALID_REQUEST         0x3
#define STORAGE_PROTOCOL_STATUS_NO_DEVICE               0x4
#define STORAGE_PROTOCOL_STATUS_BUSY                    0x5
#define STORAGE_PROTOCOL_STATUS_DATA_OVERRUN            0x6
#define STORAGE_PROTOCOL_STATUS_INSUFFICIENT_RESOURCES  0x7
#define STORAGE_PROTOCOL_STATUS_THROTTLED_REQUEST       0x8

#define STORAGE_PROTOCOL_STATUS_NOT_SUPPORTED           0xFF

//
// Command Length for Storage Protocols.
//
#define STORAGE_PROTOCOL_COMMAND_LENGTH_NVME            0x40            // NVMe commands are always 64 bytes.

//
// Command Specific Information for Storage Protocols - "CommandSpecific" field.
//
#define STORAGE_PROTOCOL_SPECIFIC_NVME_ADMIN_COMMAND    0x01
#define STORAGE_PROTOCOL_SPECIFIC_NVME_NVM_COMMAND      0x02
#endif

// Define selftest code
enum eDeviceSelftestCode {
    DEVICE_SELFTEST_CODE_SHORT_TEST = 0x1,       // short test
    DEVICE_SELFTEST_CODE_EXTENDED_TEST = 0x2,    // start extended test
    DEVICE_SELFTEST_CODE_VENDOR_SPECIFIC = 0x0E, // vendor specific
    DEVICE_SELFTEST_CODE_ABORT_TEST = 0xF        // abort test
};


class NvmeUtil
{
public:
    static BOOL IdentifyNamespace(HANDLE hDevice, DWORD dwNSID, PNVME_IDENTIFY_NAMESPACE_DATA pNamespaceIdentify);
    static BOOL IdentifyController(HANDLE hDevice, PNVME_IDENTIFY_CONTROLLER_DATA pControllerIdentify);
    static BOOL GetSMARTHealthInfoLog(HANDLE hDevice, PNVME_HEALTH_INFO_LOG pHealthInfoLog);
    static BOOL GetSelfTestLog(HANDLE hDevice, PNVME_DEVICE_SELF_TEST_LOG pSelfTestLog);
    static BOOL DeviceSelfTest(HANDLE hDevice, eDeviceSelftestCode selftestCode);
};

#endif // NvmeUtil_H
