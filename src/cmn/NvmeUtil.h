#ifndef NvmeUtil_H
#define NvmeUtil_H

#include "StdMacro.h"
#include "StdHeader.h"
#include "SysHeader.h"
#include "CmdBase.h"

// For firmware update
#ifndef STORAGE_HW_FIRMWARE_REQUEST_FLAG_CONTROLLER

// Parameter and data structure for firmware upgrade IOCTLs
// IOCTL_STORAGE_FIRMWARE_GET_INFO, IOCTL_STORAGE_FIRMWARE_DOWNLOAD, IOCTL_STORAGE_FIRMWARE_ACTIVATE

// Indicate the target of the request other than the device handle/object itself.
// This is used in "Flags" field of data structures for firmware upgrade request.
#define STORAGE_HW_FIRMWARE_REQUEST_FLAG_CONTROLLER                     0x00000001

// Indicate that current FW image segment is the last one.
#define STORAGE_HW_FIRMWARE_REQUEST_FLAG_LAST_SEGMENT                   0x00000002

// Indicate that current FW image segment is the first one.
#define STORAGE_HW_FIRMWARE_REQUEST_FLAG_FIRST_SEGMENT                  0x00000004

// Indicate that any existing firmware in slot should be replaced with the downloaded image.
// Only valid for IOCTL_STORAGE_FIRMWARE_ACTIVATE.
#define STORAGE_HW_FIRMWARE_REQUEST_FLAG_REPLACE_EXISTING_IMAGE         0x40000000

// Indicate that the existing firmware in slot should be activated.
// Only valid for IOCTL_STORAGE_FIRMWARE_ACTIVATE.
#define STORAGE_HW_FIRMWARE_REQUEST_FLAG_SWITCH_TO_EXISTING_FIRMWARE    0x80000000

// Input parameter for IOCTL_STORAGE_FIRMWARE_GET_INFO
typedef struct _STORAGE_HW_FIRMWARE_INFO_QUERY {
    DWORD   Version;            // sizeof(STORAGE_FIRMWARE_INFO_QUERY)
    DWORD   Size;               // Whole size of the buffer (in case this data structure being extended to be variable length)
    DWORD   Flags;
    DWORD   Reserved;
} STORAGE_HW_FIRMWARE_INFO_QUERY, *PSTORAGE_HW_FIRMWARE_INFO_QUERY;

// Output parameter for IOCTL_STORAGE_FIRMWARE_GET_INFO
// The total size of returned data is for Firmware Info is:
//   sizeof(STORAGE_HW_FIRMWARE_INFO) + sizeof(STORAGE_HW_FIRMWARE_SLOT_INFO) * (SlotCount - 1).
// If the buffer is not big enough, callee should set the required length in "Size" field of STORAGE_HW_FIRMWARE_INFO,

// Following value maybe used in "PendingActiveSlot" field indicating there is no firmware pending to activate.
#define STORAGE_HW_FIRMWARE_INVALID_SLOT              0xFF

#pragma warning(push)
#pragma warning(disable:4214) // bit fields other than int

#define STORAGE_HW_FIRMWARE_REVISION_LENGTH             16

typedef struct _STORAGE_HW_FIRMWARE_SLOT_INFO {
    DWORD   Version;            // sizeof(STORAGE_HW_FIRMWARE_SLOT_INFO)
    DWORD   Size;               // size the data contained in STORAGE_HW_FIRMWARE_SLOT_INFO.
    BYTE    SlotNumber;
    BYTE    ReadOnly : 1;
    BYTE    Reserved0 : 7;
    BYTE    Reserved1[6];
    BYTE    Revision[STORAGE_HW_FIRMWARE_REVISION_LENGTH];
} STORAGE_HW_FIRMWARE_SLOT_INFO, *PSTORAGE_HW_FIRMWARE_SLOT_INFO;

typedef struct _STORAGE_HW_FIRMWARE_INFO {
    DWORD   Version;            // sizeof(STORAGE_HW_FIRMWARE_INFO)
    DWORD   Size;               // size of the whole buffer including slot[]
    BYTE    SupportUpgrade : 1;
    BYTE    Reserved0 : 7;
    BYTE    SlotCount;
    BYTE    ActiveSlot;
    BYTE    PendingActivateSlot;
    BOOLEAN FirmwareShared;     // The firmware applies to both device and adapter. For example: PCIe SSD.
    BYTE    Reserved[3];
    DWORD   ImagePayloadAlignment;  // Number of bytes. Max: PAGE_SIZE. The transfer size should be multiple of this unit size. Some protocol requires at least sector size. 0 means the value is not valid.
    DWORD   ImagePayloadMaxSize;    // for a single command.
    STORAGE_HW_FIRMWARE_SLOT_INFO Slot[ANYSIZE_ARRAY];
} STORAGE_HW_FIRMWARE_INFO, *PSTORAGE_HW_FIRMWARE_INFO;
#pragma warning(pop)

// Input parameter for IOCTL_STORAGE_FIRMWARE_DOWNLOAD
#pragma warning(push)
#pragma warning(disable:4200)

typedef struct _STORAGE_HW_FIRMWARE_DOWNLOAD {
    DWORD       Version;            // sizeof(STORAGE_HW_FIRMWARE_DOWNLOAD)
    DWORD       Size;               // size of the whole buffer include "ImageBuffer"
    DWORD       Flags;
    BYTE        Slot;               // Slot number that firmware image will be downloaded into.
    BYTE        Reserved[3];
    DWORDLONG   Offset;             // Image file offset, should be aligned to "ImagePayloadAlignment" value from STORAGE_FIRMWARE_INFO.
    DWORDLONG   BufferSize;         // should be multiple of "ImagePayloadAlignment" value from STORAGE_FIRMWARE_INFO.
    BYTE        ImageBuffer[ANYSIZE_ARRAY];     // firmware image file.
} STORAGE_HW_FIRMWARE_DOWNLOAD, *PSTORAGE_HW_FIRMWARE_DOWNLOAD;

typedef struct _STORAGE_HW_FIRMWARE_DOWNLOAD_V2 {
    DWORD       Version;            // sizeof(STORAGE_HW_FIRMWARE_DOWNLOAD_V2)
    DWORD       Size;               // size of the whole buffer include "ImageBuffer"
    DWORD       Flags;
    BYTE        Slot;               // Slot number that firmware image will be downloaded into.
    BYTE        Reserved[3];
    DWORDLONG   Offset;             // Image file offset, should be aligned to "ImagePayloadAlignment" value from STORAGE_FIRMWARE_INFO.
    DWORDLONG   BufferSize;         // should be multiple of "ImagePayloadAlignment" value from STORAGE_FIRMWARE_INFO.
    DWORD       ImageSize;          // Firmware Image size.
    DWORD       Reserved2;
    BYTE        ImageBuffer[ANYSIZE_ARRAY];     // firmware image file.
} STORAGE_HW_FIRMWARE_DOWNLOAD_V2, *PSTORAGE_HW_FIRMWARE_DOWNLOAD_V2;

#pragma warning(pop)

// Input parameter for IOCTL_STORAGE_FIRMWARE_ACTIVATE
typedef struct _STORAGE_HW_FIRMWARE_ACTIVATE {
    DWORD   Version;
    DWORD   Size;
    DWORD   Flags;
    BYTE    Slot;                   // Slot with firmware image to be activated.
    BYTE    Reserved0[3];
} STORAGE_HW_FIRMWARE_ACTIVATE, *PSTORAGE_HW_FIRMWARE_ACTIVATE;

#define STG_S_POWER_CYCLE_REQUIRED _HRESULT_TYPEDEF_(0x00030207L)
#endif

/* ===================================================================== */
/* ============================== Contants/Macros ====================== */
/* ===================================================================== */
#define C_CAST(type, val) (type)(val)

// Big endian parameter order, little endian value
#define M_BytesTo4ByteValue(b3, b2, b1, b0) \
        (C_CAST(uint32_t, ((C_CAST(uint32_t, b3) << 24) | (C_CAST(uint32_t, b2) << 16) | \
            (C_CAST(uint32_t, b1) <<  8) | (C_CAST(uint32_t, b0) <<  0)  )))

// Big endian parameter order, little endian value
    #define M_BytesTo8ByteValue(b7, b6, b5, b4, b3, b2, b1, b0) \
        (C_CAST(uint64_t, ((C_CAST(uint64_t, b7) << 56) | (C_CAST(uint64_t, b6) << 48) | \
            (C_CAST(uint64_t, b5) << 40) | (C_CAST(uint64_t, b4) << 32) | \
                (C_CAST(uint64_t, b3) << 24) | (C_CAST(uint64_t, b2) << 16) | \
                   (C_CAST(uint64_t, b1) <<  8) | (C_CAST(uint64_t, b0) <<  0))))

// Max & Min Helpers
#define  M_Min(a,b)    (((a)<(b))?(a):(b))
#define  M_Max(a,b)    (((a)>(b))?(a):(b))

#define M_ToBool(expression) ((expression) > 0 ? true : false)

#define M_BitN(n)   (UINT64_C(1) << n)
#define M_SET_BIT(val, bitNum) (val |= M_BitN(bitNum))
#define M_CLEAR_BIT(val, bitNum) (val &= (~M_BitN(bitNum)))
#define M_2sCOMPLEMENT(val) (~(val) + 1)
#define M_Byte(val, byteNum) ( (uint8_t) ( ( (val) & UINT64_C(0x00000000FF000000) ) >> (8*(byteNum)) ) )

#define RESERVED 0
#define OBSOLETE 0

// Defined by SPC3 as the maximum sense length
#define SPC3_SENSE_LEN  UINT8_C(252)
#define SCSI_OPERATION_CODE (0)
#define DOUBLE_BUFFERED_MAX_TRANSFER_SIZE   16384

#define safe_Free(mem) if(mem){ free(mem); mem = NULL; }

enum eSelfTestCode {
    STC_SHORT    = 0x01, // short test
    STC_EXTENDED = 0x02, // start extended test
    STC_SPECIFIC = 0x0E, // vendor specific
    STC_ABORT    = 0x0F, // abort test
};

class NvmeUtil
{
public:
    static BOOL IdentifyNamespace(HANDLE hDevice, DWORD dwNSID, PNVME_IDENTIFY_NAMESPACE_DATA pNamespaceIdentify);
    static BOOL IdentifyController(HANDLE hDevice, PNVME_IDENTIFY_CONTROLLER_DATA pControllerIdentify);
    static BOOL GetHealthInfoLog(HANDLE hDevice, PNVME_HEALTH_INFO_LOG pHealthInfoLog);
    static BOOL GetHealthInfoLog(HANDLE hDevice, DWORD dwNSID, PNVME_HEALTH_INFO_LOG hlog);

    // Firmware update
    static BOOL win10FW_GetfwdlInfoQuery(HANDLE hHandle, PSTORAGE_HW_FIRMWARE_INFO fwdlInfo, BOOL isNvme, PBYTE pFwupdateSlotId);
    static BOOL win10FW_TransferFile(HANDLE hHandle, PSTORAGE_HW_FIRMWARE_INFO fwdlInfo, BYTE slotId, LPCSTR fwFileName, int64_t transize);
    static BOOL win10FW_Active(HANDLE hHandle, PSTORAGE_HW_FIRMWARE_INFO fwdlInfo, BYTE slotId);

    // Self-test via NVME PCI-E
    static BOOL GetSelfTestLog(HANDLE hDevice, PNVME_DEVICE_SELF_TEST_LOG pSelfTestLog);
    static BOOL DeviceSelfTest(HANDLE hDevice, eSelfTestCode selftestCode);
};

#endif // NvmeUtil_H
