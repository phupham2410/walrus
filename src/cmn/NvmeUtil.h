#ifndef NvmeUtil_H
#define NvmeUtil_H

#include "StdMacro.h"
#include "StdHeader.h"
#include "CmdBase.h"

#include <windows.h>
#include <winioctl.h>
#include <ntddscsi.h>
#include <nvme.h>

/* ===================================================================== */
/* ======================== Windows-related items ====================== */
/* ===================================================================== */

// Redefine in case of Old MINGW SDK is being used
#ifndef STORAGE_PROTOCOL_STRUCTURE_VERSION
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

//
// Define alignment requirements for variable length components in extended SRB.
// For Win64, need to ensure all variable length components are 8 bytes align
// so the pointer fields within the variable length components are 8 bytes align.
//
#if defined(_WIN64) || defined(_M_ALPHA)
#define STOR_ADDRESS_ALIGN           DECLSPEC_ALIGN(8)
#else
#define STOR_ADDRESS_ALIGN
#endif


//
// Generic structure definition for accessing any STOR_ADDRESS. All
// STOR_ADDRESS must begin with a Type, Port and AddressLength field.
//
typedef struct STOR_ADDRESS_ALIGN _STOR_ADDRESS {
    USHORT Type;
    USHORT Port;
    ULONG AddressLength;
    _Field_size_bytes_(AddressLength) UCHAR AddressData[ANYSIZE_ARRAY];
} STOR_ADDRESS, *PSTOR_ADDRESS;

// Define different storage address types
#define STOR_ADDRESS_TYPE_UNKNOWN   0x0
#define STOR_ADDRESS_TYPE_BTL8      0x1
#define STOR_ADDRESS_TYPE_MAX       0xffff

// Define 8 bit bus, target and LUN address scheme
#define STOR_ADDR_BTL8_ADDRESS_LENGTH    4
typedef struct STOR_ADDRESS_ALIGN _STOR_ADDR_BTL8 {
    _Field_range_(STOR_ADDRESS_TYPE_BTL8, STOR_ADDRESS_TYPE_BTL8)
        USHORT Type;
    USHORT Port;
    _Field_range_(STOR_ADDR_BTL8_ADDRESS_LENGTH, STOR_ADDR_BTL8_ADDRESS_LENGTH)
        ULONG AddressLength;
    UCHAR Path;
    UCHAR Target;
    UCHAR Lun;
    UCHAR Reserved;
} STOR_ADDR_BTL8, *PSTOR_ADDR_BTL8;

// Redefine in case of Old MINGW SDK is being used
#ifndef IOCTL_SCSI_PASS_THROUGH_EX
#define IOCTL_SCSI_PASS_THROUGH_EX          CTL_CODE(IOCTL_SCSI_BASE, 0x0411, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)


typedef struct _SCSI_PASS_THROUGH_EX {
    ULONG Version;
    ULONG Length;                   // size of the structure
    ULONG CdbLength;                // non-zero value should be set by caller
    ULONG StorAddressLength;        // non-zero value should be set by caller
    UCHAR ScsiStatus;
    UCHAR SenseInfoLength;          // optional, can be zero
    UCHAR DataDirection;            // data transfer direction
    UCHAR Reserved;                 // padding
    ULONG TimeOutValue;
    ULONG StorAddressOffset;        // a value bigger than (structure size + CdbLength) should be set by caller
    ULONG SenseInfoOffset;
    ULONG DataOutTransferLength;    // optional, can be zero
    ULONG DataInTransferLength;     // optional, can be zero
    ULONG_PTR DataOutBufferOffset;
    ULONG_PTR DataInBufferOffset;
    _Field_size_bytes_full_(CdbLength) UCHAR Cdb[ANYSIZE_ARRAY];
} SCSI_PASS_THROUGH_EX, *PSCSI_PASS_THROUGH_EX;
#endif


// For firmware update
#ifndef STORAGE_HW_FIRMWARE_REQUEST_FLAG_CONTROLLER
//
// Parameter and data structure for firmware upgrade IOCTLs
// IOCTL_STORAGE_FIRMWARE_GET_INFO, IOCTL_STORAGE_FIRMWARE_DOWNLOAD, IOCTL_STORAGE_FIRMWARE_ACTIVATE
//

//
// Indicate the target of the request other than the device handle/object itself.
// This is used in "Flags" field of data structures for firmware upgrade request.
//
#define STORAGE_HW_FIRMWARE_REQUEST_FLAG_CONTROLLER                     0x00000001

//
// Indicate that current FW image segment is the last one.
//
#define STORAGE_HW_FIRMWARE_REQUEST_FLAG_LAST_SEGMENT                   0x00000002

//
// Indicate that current FW image segment is the first one.
//
#define STORAGE_HW_FIRMWARE_REQUEST_FLAG_FIRST_SEGMENT                  0x00000004

//
// Indicate that any existing firmware in slot should be replaced with the downloaded image.
// Only valid for IOCTL_STORAGE_FIRMWARE_ACTIVATE.
//
#define STORAGE_HW_FIRMWARE_REQUEST_FLAG_REPLACE_EXISTING_IMAGE         0x40000000

//
// Indicate that the existing firmware in slot should be activated.
// Only valid for IOCTL_STORAGE_FIRMWARE_ACTIVATE.
//
#define STORAGE_HW_FIRMWARE_REQUEST_FLAG_SWITCH_TO_EXISTING_FIRMWARE    0x80000000

//
// Input parameter for IOCTL_STORAGE_FIRMWARE_GET_INFO
//
typedef struct _STORAGE_HW_FIRMWARE_INFO_QUERY {
    DWORD   Version;            // sizeof(STORAGE_FIRMWARE_INFO_QUERY)
    DWORD   Size;               // Whole size of the buffer (in case this data structure being extended to be variable length)
    DWORD   Flags;
    DWORD   Reserved;
} STORAGE_HW_FIRMWARE_INFO_QUERY, *PSTORAGE_HW_FIRMWARE_INFO_QUERY;

//
// Output parameter for IOCTL_STORAGE_FIRMWARE_GET_INFO
// The total size of returned data is for Firmware Info is:
//   sizeof(STORAGE_HW_FIRMWARE_INFO) + sizeof(STORAGE_HW_FIRMWARE_SLOT_INFO) * (SlotCount - 1).
// If the buffer is not big enough, callee should set the required length in "Size" field of STORAGE_HW_FIRMWARE_INFO,
//

//
// Following value maybe used in "PendingActiveSlot" field indicating there is no firmware pending to activate.
//
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


//
// Input parameter for IOCTL_STORAGE_FIRMWARE_DOWNLOAD
//
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

//
// Input parameter for IOCTL_STORAGE_FIRMWARE_ACTIVATE
//
typedef struct _STORAGE_HW_FIRMWARE_ACTIVATE {

    DWORD   Version;
    DWORD   Size;

    DWORD   Flags;
    BYTE    Slot;                   // Slot with firmware image to be activated.
    BYTE    Reserved0[3];

} STORAGE_HW_FIRMWARE_ACTIVATE, *PSTORAGE_HW_FIRMWARE_ACTIVATE;

#define STG_S_POWER_CYCLE_REQUIRED       _HRESULT_TYPEDEF_(0x00030207L)
#endif
/* ===================================================================== */
/* ============================== Contants/Macros ====================== */
/* ===================================================================== */
#define C_CAST(type, val) (type)(val)

// Big endian parameter order, little endian value
#define M_BytesTo4ByteValue(b3, b2, b1, b0)                    (        \
    C_CAST(uint32_t, (  (C_CAST(uint32_t, b3) << 24) | (C_CAST(uint32_t, b2) << 16) |          \
                 (C_CAST(uint32_t, b1) <<  8) | (C_CAST(uint32_t, b0) <<  0)  )         \
                                                                   ) )
// Big endian parameter order, little endian value
    #define M_BytesTo8ByteValue(b7, b6, b5, b4, b3, b2, b1, b0)    (        \
    C_CAST(uint64_t, ( (C_CAST(uint64_t, b7) << 56) | (C_CAST(uint64_t, b6) << 48) |           \
                (C_CAST(uint64_t, b5) << 40) | (C_CAST(uint64_t, b4) << 32) |           \
                (C_CAST(uint64_t, b3) << 24) | (C_CAST(uint64_t, b2) << 16) |           \
                (C_CAST(uint64_t, b1) <<  8) | (C_CAST(uint64_t, b0) <<  0)  )          \
                                                                   ) )

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

#define safe_Free(mem)  \
    if(mem)                 \
    {                       \
            free(mem);          \
            mem = NULL;         \
    }                       \


/* ===================================================================== */
/* ================================ Types ============================== */
/* ===================================================================== */
typedef enum _eScsiCmds
{
    REPORT_LUNS_CMD = 0xA0,
    SANITIZE_CMD    = 0x48,
} eScsiCmds;

typedef enum _eCDBLen{
    CDB_LEN_6  = 6,
    CDB_LEN_10 = 10,
    CDB_LEN_12 = 12,
    CDB_LEN_16 = 16,
    CDB_LEN_32 = 32,
    CDB_LEN_UNKNOWN
}eCDBLen;

typedef enum _eDataTransferDirection
{
    XFER_NO_DATA,
    XFER_DATA_IN,     // Transfer from target to host
    XFER_DATA_OUT,    // Transfer from host to target
    XFER_DATA_OUT_IN, // Transfer from host to target, followed by target to host
    XFER_DATA_IN_OUT, // Transfer from target to host, followed by host to target
} eDataTransferDirection;

typedef struct _tScsiPassThroughIOStruct {
    SCSI_PASS_THROUGH           scsiPassthrough;
    ULONG                       padding;
    UCHAR                       senseBuffer[SPC3_SENSE_LEN];
    UCHAR                       dataBuffer[1];
} tScsiPassThroughIOStruct;

typedef struct _tScsiPassThroughEXIOStruct
{
    SCSI_PASS_THROUGH_EX           scsiPassThroughEX;
    UCHAR                           cdbPad[CDB_LEN_32 - 1];
    ULONG                           padding;
    STOR_ADDR_BTL8                  storeAddr;
    UCHAR                           senseBuffer[SPC3_SENSE_LEN];
    UCHAR                           dataInBuffer[DOUBLE_BUFFERED_MAX_TRANSFER_SIZE];
    UCHAR                           dataOutBuffer[DOUBLE_BUFFERED_MAX_TRANSFER_SIZE];
} tScsiPassThroughEXIOStruct;
/* ===================================================================== */
/* ========================  NVME-related items ======================== */
/* ===================================================================== */

// Define selftest code
typedef enum _eDeviceSelftestCode {
    DEVICE_SELFTEST_CODE_SHORT_TEST = 0x1,       // short test
    DEVICE_SELFTEST_CODE_EXTENDED_TEST = 0x2,    // start extended test
    DEVICE_SELFTEST_CODE_VENDOR_SPECIFIC = 0x0E, // vendor specific
    DEVICE_SELFTEST_CODE_ABORT_TEST = 0xF        // abort test
} eDeviceSelftestCode;

typedef struct {
    ULONG NSID[1024];
} NVME_IDENTIFY_ACTIVE_NAMESPACE_LIST, *PNVME_IDENTIFY_ACTIVE_NAMESPACE_LIST;

/* ===================================================================== */
/* ========================  SCSI-related items ======================== */
/* ===================================================================== */
typedef enum _eScsiSanitizeFeature{
    SCSI_SANITIZE_OVERWRITE             = 0x01,
    SCSI_SANITIZE_BLOCK_ERASE           = 0x02,
    SCSI_SANITIZE_CRYPTOGRAPHIC_ERASE   = 0x03,
    SCSI_SANITIZE_EXIT_FAILURE_MODE     = 0x1F
}eScsiSanitizeType;

typedef struct _tScsiStatus
{
    uint8_t         format;
    uint8_t         senseKey;
    uint8_t         asc;
    uint8_t         ascq;
    uint8_t         fru;
} tScsiStatus;

typedef enum _eSenseFormat{
    SCSI_SENSE_NO_SENSE_DATA   = 0,
    SCSI_SENSE_CUR_INFO_FIXED  = 0x70,
    SCSI_SENSE_DEFER_ERR_FIXED = 0x71,
    SCSI_SENSE_CUR_INFO_DESC   = 0x72,
    SCSI_SENSE_DEFER_ERR_DESC  = 0x73,
    SCSI_SENSE_VENDOR_SPECIFIC = 0x7F,
    SCSI_SENSE_UNKNOWN_FORMAT
} eSenseFormat;

typedef struct _tScsiContext {
    HANDLE          hDevice;
    ULONG           alignmentMask;
    UCHAR           srbType;
    SCSI_ADDRESS    scsiAddr;
    uint8_t         cdb[64]; //64 just so if we ever get there.
    uint8_t         cdbLength;
    int8_t          direction;
    uint8_t         *pdata;
    uint32_t        dataLength;
    uint8_t         *psense;
    uint32_t        senseDataSize;//should be reduced to uint8 in the future as sense data maxes at 252Bytes
    uint32_t        timeout; //seconds
    uint8_t         lastCommandSenseData[SPC3_SENSE_LEN]; // The sense data of last command
    unsigned int    lastError; // GetLastError in Windows.
    tScsiStatus     returnStatus;
} tScsiContext;

/* ======================== Public Classes ======================== */
class NvmeUtil
{
public:
    static BOOL IdentifyNamespace(HANDLE hDevice, DWORD dwNSID, PNVME_IDENTIFY_NAMESPACE_DATA pNamespaceIdentify);
    static BOOL IdentifyController(HANDLE hDevice, PNVME_IDENTIFY_CONTROLLER_DATA pControllerIdentify);
    static BOOL GetSMARTHealthInfoLog(HANDLE hDevice, PNVME_HEALTH_INFO_LOG pHealthInfoLog);
    static BOOL GetSelfTestLog(HANDLE hDevice, PNVME_DEVICE_SELF_TEST_LOG pSelfTestLog);
    static BOOL DeviceSelfTest(HANDLE hDevice, eDeviceSelftestCode selftestCode);
    static BOOL IdentifyActiveNSIDList(HANDLE hDevice, PNVME_IDENTIFY_ACTIVE_NAMESPACE_LIST pNsidList);
    static BOOL GetSCSIAAddress(HANDLE hDevice, PSCSI_ADDRESS pScsiAddress);
    static BOOL ScsiReportLuns(tScsiContext *scsiCtx, uint8_t selectReport, uint32_t allocationLength, uint8_t *ptrData);

    // Becarefully with follow santize command - this command is implementing NVM Format commmand function.
    // refer https://learn.microsoft.com/en-us/windows-hardware/drivers/storage/stornvme-command-set-support
    // To query NVME format progress - use IdentifyNamespace() with active NSID (1 for almost case - or use ScsiReportLuns to query format )
    // to query Format Progress Indicator (FPI) field.
    // TODO: test this function
    static BOOL ScsiSanitizeCmd(tScsiContext *scsiCtx, eScsiSanitizeType sanitizeType, bool immediate, bool znr, bool ause, uint16_t parameterListLength, uint8_t *ptrData);

    // Firmware update
    static BOOL win10FW_GetfwdlInfoQuery(HANDLE hHandle, PSTORAGE_HW_FIRMWARE_INFO fwdlInfo);
    static BOOL win10FW_Download(HANDLE hHandle, PSTORAGE_HW_FIRMWARE_INFO fwdlInfo, BYTE slotId, PDWORDLONG pOffset, BOOL firstSegment, BOOL lastSegment, uint8_t *ptrData, DWORD dataSize);
    static BOOL win10FW_Active(HANDLE hHandle, PSTORAGE_HW_FIRMWARE_INFO fwdlInfo, BYTE slotId);
};

#endif // NvmeUtil_H
