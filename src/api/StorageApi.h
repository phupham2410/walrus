#ifndef __STORAGEAPI_H__
#define __STORAGEAPI_H__

#include <map>
#include <set>
#include <vector>
#include <string>

#define SECTOR_SIZE 512

namespace StorageApi {
    typedef float F32;
    typedef double F64;
    typedef char S8;
    typedef short S16;
    typedef int S32;
    typedef long long S64;

    typedef int HDL;
    typedef std::string STR;
    typedef const STR CSTR;
    typedef std::wstring WSTR;
    typedef const WSTR CWSTR;
    typedef unsigned char U8;
    typedef unsigned short U16;
    typedef unsigned int U32;
    typedef unsigned long long U64;

    // Return code from StorageApi
    enum eRetCode {
        RET_OK = 0,
        RET_FAIL = -1,
        RET_OUT_OF_MEMORY = (int)0x80000000,
        RET_LOGIC         = (int)0x80000001,
        RET_INVALID_ARG   = (int)0x80000002,
        RET_XDATA         = (int)0x80000003,
        RET_NOT_SUPPORT   = (int)0x80000004,
        RET_OUT_OF_SPACE  = (int)0x80000005,
        RET_CONFLICT      = (int)0x80000006,
        RET_TIME_OUT      = (int)0x80000007,
        RET_ABANDONED     = (int)0x80000008,
        RET_BUSY          = (int)0x80000009,
        RET_EMPTY         = (int)0x8000000A,
        RET_NOT_IMPLEMENT = (int)0x8000000B,
        RET_NOT_FOUND     = (int)0x8000000C,
        RET_ABORTED       = (int)0x80000010,

        // Additional code for handling drives
        RET_SKIP_DRIVE    = (int)0x80000100,
        RET_NO_PERMISSION = (int)0x80000100,
    };

    struct sFeature {
        U8 ataversion;        // (VAL) Supported ATA Version
        U8 lbamode;           // (Y/N) Supported LBA mode (28b or 48b)
        U8 dma;               // (Y/N) Supported DMA mode
        U8 provision;         // (Y/N) Supported OverProvision feature
        U8 trim;              // (Y/N) Supported Optimize feature (TRIM commands)
        U8 smart;             // (Y/N) Supported SMART feature
        U8 security;          // (Y/N) Supported Security feature
        U8 ncq;               // (Y/N) Supported NCQ mode
        U8 test;              // (Y/N) Supported SelfTest
        U8 erase;             // (Y/N) Supported SecureErase
        U8 dlcode;            // (Y/N) Supported Download Microcode

        void reset();
        sFeature();
        sFeature(const U8* val);
    };

    typedef std::pair<U64, U64> tPartAddr;      // pair<start, count>
    typedef std::vector<tPartAddr> tAddrArray;  // List of partition's position
    typedef const tAddrArray tConstAddrArray;   // Constant vector

    struct sPartition {
        WSTR name;            // Partition name
        U32 index;            // Partition index
        F64 cap;              // Capacity in GB (count * 512 bytes)
        tPartAddr addr;       // Partition address (start lba, sector count)

        void reset();
        sPartition();
    };

    typedef std::vector<sPartition> tPartArray;
    typedef const tPartArray tConstPartArray;
    typedef tPartArray::iterator tPartIter;
    typedef tPartArray::const_iterator tPartConstIter;

    struct sPartInfo {
        tPartArray parr;

        void reset();
        sPartInfo();
    };

    struct sIdentify {
        STR model;          // Model string
        STR serial;         // Serial string
        STR version;        // Firmware version
        STR confid;         // Configuration ID
        F64 cap;            // Capacity in GB
        sFeature  features; // Supported features

        void reset();
        sIdentify();
    };

    struct sSmartAttr {
        U8  id, value;
        U8 worst, threshold;
        U32 loraw, hiraw;
        STR name;
        STR note;

        void reset();
        sSmartAttr();
    };

    // mapping id <--> smart attribute
    typedef std::map<U8, sSmartAttr> tAttrMap;
    typedef const tAttrMap tConstAttrMap;
    typedef tAttrMap::iterator tAttrIter;
    typedef tAttrMap::const_iterator tAttrConstIter;

    struct sSmartInfo {
        tAttrMap amap;

        void reset();
        sSmartInfo();
    };

    struct sDriveInfo {
        STR        name;    // Physical drive name
        sIdentify  id;      // Identify info
        sSmartInfo si;      // SMART info
        sPartInfo  pi;      // Partitions info

        S64 temp;
        U64 tread;          // total host read
        U64 twrtn;          // total host write
        U8  bustype;
        U32 maxtfsec;       // Max transfer size in sector

        void reset();
        sDriveInfo();
    };

    typedef std::vector<sDriveInfo> tDriveArray;
    typedef const tDriveArray tConstDriveArray;

    struct sProgress {
        bool ready;   // ready to be read from client thread
        bool stop;    // stop request from client thread
        U32 workload; // max workload of current task
        U32 progress; // current progress ( 0 -> workload)
        STR info;     // output info of some operation
        eRetCode rval;// return value
        bool done;    // complete status of current task

        // special progress info for each operation type
        union {
            struct sScanProgress {
                U64 dummy;   // FIXME: update later
            } scan;

            struct sCloneProgress {
                U64 speed;   // current copying speed
                U64 remsec;  // remaining partition ?
                U64 remsize; // remaining data ?
            } clone;

            struct sEraseProgress {
                U64 state;   // current state
            } erase;

            struct sTestProgress {
                U64 dummy;   // FIXME: update later
            } test;

            struct sUpdateProgress {
                U64 dlsize;  // downloaded size ?
                U64 remsize; // remaining size ?
            } fwup;

            struct sSetopProgress {
                U64 dummy;   // FIXME: update later
            } setop;

            struct sTrimProgress {
                U64 dummy;   // FIXME: update later
            } trim;

            struct sReadProgress {
                U64 speed;   // current read speed (KBs/sec)
            } read;
        } priv;

        sProgress();
        void reset() volatile;
        void init(U32 load) volatile;
    };

    // ToString utilities
    STR ToString(eRetCode code);
    STR ToString(const sDriveInfo& drv, U32 indent = 0);
    STR ToString(const sSmartInfo& sm, U32 indent = 0);
    STR ToString(const sIdentify& id, U32 indent = 0);
    STR ToString(const sFeature& ftr, U32 indent = 0);
    STR ToString(const sSmartAttr& attr, U32 indent = 0);
    STR ToString(const U8* bufptr, U32 bufsize);
    STR ToString(const sPartition& pt, U32 indent = 0);
    STR ToString(const sPartInfo& pi, U32 indent = 0);

    WSTR ToWideString(const sPartition& pt, U32 indent = 0);
    WSTR ToWideString(const sPartInfo& pi, U32 indent = 0);

    STR ToShortString(const sDriveInfo& drv, U32 indent = 0);

    // Get list of plugged-in drives
    // output: drvlst - list of drives
    // input : rid - read identify sector or not
    // input : rsm - read smart sectors or not
    // input : p - progress
    eRetCode ScanDrive(tDriveArray& darr, bool rid = true, bool rsm = true, volatile sProgress* p = NULL);

    // Get list of partitions on drive. No progress param
    eRetCode ScanPartition(HDL handle, sPartInfo& pi);
    eRetCode ScanPartition(CSTR& drvname, sPartInfo& pi);

    // Download firmware to physical drive drvname
    eRetCode UpdateFirmware(CSTR& drvname, const U8* bufptr, U32 bufsize, volatile sProgress* p = NULL);

    // Set over-provision on drive using factor (0 -> 100 ?)
    eRetCode SetOverProvision(CSTR& drvname, U32 factor, volatile sProgress* p = NULL);

    // Run TRIM command on drvname
    eRetCode OptimizeDrive(CSTR& drvname, volatile sProgress* p = NULL);

    // Run Secure-Erase command on drvname
    eRetCode SecureErase(CSTR& drvname, volatile sProgress* p = NULL);

    // Run Self-Test on drive drvname
    eRetCode SelfTest(CSTR& drvname, U32 param = 0, volatile sProgress* p = NULL);

    // Clone list of sections from srcdrv to dstdrv, start at offset 0
    eRetCode CloneDrive(CSTR& dstdrv, CSTR& srcdrv, tConstAddrArray& parr, volatile sProgress* p = NULL);

    // Utility for handling IO commands
    void Delay(U32 mlsec);
    void Close(HDL handle);
    eRetCode Open(CSTR& drvname, HDL& handle);
    eRetCode Read(HDL handle, U64 lba, U32 count, U8* buffer, volatile sProgress* p = NULL);
    eRetCode Write(HDL handle, U64 lba, U32 count, const U8* buffer, volatile sProgress* p = NULL);

    // Handle misc request
    struct sAdapterInfo {
        U32 BusType;          // Check Windows STORAGE_BUS_TYPE
        U32 MaxTransferSector;
    };

    STR ToString(const sAdapterInfo& pi, U32 indent = 0);
    eRetCode GetDeviceInfo(HDL handle, sAdapterInfo& info);
}

#endif //__STORAGEAPI_H__
