
#ifdef STORAGE_API_TEST

#include "StorageApiCmn.h"

// Fake implementation of StorageApi
using namespace StorageApi;

// ------------------------------------------------------------
// Generate random value /error

// Random error with rate
#define RANDERR00  RandomError(0)  // 0%
#define RANDERR01  RandomError(1)  // 1%
#define RANDERR03  RandomError(3)  // 3%
#define RANDERR05  RandomError(5)  // 5%
#define RANDERR09  RandomError(9)  // 9%
#define RANDERR20  RandomError(20) // 20%
#define RANDERR50  RandomError(50) // 50%
#define RANDERR80  RandomError(80) // 80%

// Generate random number by mode, 16bit, 32bit
#define RANDMOD(n) (rand()%(n))
#define RANDMOD03 RANDMOD(3)
#define RANDMOD20 RANDMOD(20)
#define RANDMOD70 RANDMOD(70)
#define RANDMOD80 RANDMOD(80)
#define RANDMOD90 RANDMOD(90)

// Generate random 16/32 bit values
#define RANDVAL16 PACK16(RANDMOD(128), RANDMOD(256))
#define RANDVAL32 PACK32(RANDMOD(64), RANDMOD(96), RANDMOD(128), RANDMOD(256))

// ------------------------------------------------------------
// Pick random items

// random separated k value in range 0 -> (n-1)
#define RAND_PICK_IDX(max, cnt, idx) RandomIndex(max, cnt, idx)

// random pick items from a sequence, size == seq.size
#define RAND_PICK_SEQ(seq, size, cnt, res) do { \
    std::vector<U32> idx; RAND_PICK_IDX(size, cnt, idx); \
    for (U32 i = 0; i < cnt; i++) res.push_back(seq[idx[i]]); } while(0)

// random pick from array
#define RAND_PICK_FROM_ARRAY(arr, cnt, res) RAND_PICK_SEQ(arr, ARRAY_SIZE(arr), cnt, res)

// random pick from container
#define RAND_PICK_FROM_CONT(con, cnt, res) RAND_PICK_SEQ(con, con.size(), cnt, res)

// Get random item of an array
#define RAND_PICK_ITEM(name) name[RANDMOD(ARRAY_SIZE(name))]

// ------------------------------------------------------------
// Constants for fake storage_api

static const char* gModel[] = {
    "SDSSDXPM1-1T00-G25", "WDS100T2B0B", "CT1000MX500SSD1(Z)", "KOS10015001_EA",
    "MZ-76E500B/AM", "SA2000M8/1000G", "ST2001AHM-300B", "MCR22019DDW",
}; // count: 8

static const char* gSerial[] = {
    "SSDPEKNW010T8X1", "ZP1000CM30001", "THN-TR20Z2400U8(CS)", "ASX8200PNP-1TT-C",
    "2YY42AA#ABC", "SDSSDH3-2T00-G25", "SUV400S37/480G", "WDS500G3X0C",
}; // count 8

static const char* gVersion[] = {
    "RVT02B6Q", "M3CR023", "X61110WD", "S5Z4210020", "X41210RL", "121C", "UP7CR1K3", "512D",
}; // count 8

static const char* gConfID[] = {
    "Conf0(N/A)", "Conf1(N/A)", "Conf2(N/A)", "Conf3(N/A)",
    "Conf4(N/A)", "Conf5(N/A)", "Conf6(N/A)", "Conf7(N/A)",
}; // 8

static F64 gCap[] = {
    511, 890.76, 255.09, 128, 64.8, 127.1, 2043, 116.02,
}; // count 8

U8 gFval0[] = {8, 1, 2, 1, 1, 1, 1, 1, 0, 1};
U8 gFval1[] = {7, 1, 2, 1, 1, 0, 1, 0, 1, 0};
U8 gFval2[] = {6, 0, 2, 0, 1, 0, 1, 1, 0, 1};
U8 gFval3[] = {8, 1, 3, 0, 1, 1, 1, 1, 1, 0};
U8 gFval4[] = {0, 1, 3, 0, 0, 1, 1, 1, 1, 1};
U8 gFval5[] = {0, 0, 1, 1, 0, 1, 1, 0, 0, 1};
U8 gFval6[] = {3, 1, 2, 0, 1, 1, 1, 1, 0, 0};
U8 gFval7[] = {2, 1, 2, 1, 1, 1, 1, 1, 1, 1};

static sFeature gFtr[] = {
    gFval0, gFval1, gFval2, gFval3, gFval4, gFval5, gFval6, gFval7
}; // count 8

// Indexes into existing string array
struct sDataIndex { U32 model, serial, fwver, confid, cap, ftr; };
static sDataIndex gIndex[] = {
    {0, 0, 0, 0, 0, 0},
    {1, 1, 1, 0, 1, 1},
    {2, 2, 0, 0, 2, 2},
    {3, 3, 0, 0, 3, 3},
    {4, 4, 2, 0, 1, 4},
    {1, 5, 2, 0, 3, 0},
    {1, 6, 3, 0, 0, 3},
};

struct sAttrDef { U8 value; std::string name; };
static sAttrDef gAttr[] = {
    {1  , "Raw_Read_Error_Rate"         }, {5  , "Reallocated_Sector_Ct"       },
    {9  , "Power_On_Hours"              }, {12 , "Power_Cycle_Count"           },
    {192, "Sudden_Power_Lost_Count"     }, {194, "Temperature_Celsius"         },
    {195, "Hardware_ECC_Recovered"      }, {196, "Reallocated_Event_Count"     },
    {197, "Current_Pending_Sector_Count"}, {198, "Offline_Uncorrectable"       },
    {199, "UDMA_CRC_Error_Count"        }, {241, "Total_LBAs_Written"          },
    {242, "Total_LBAs_Read"             }, {248, "Remaining_Life_Left"         },
    {249, "Remaining_Spare_Block_Count" }, {160, "Uncorrectable_Sector_Count"  },
    {161, "Valid_Spare_Block"           }, {163, "Initial_Invalid_Block"       },
    {164, "Total_Erase_Count"           }, {165, "Maximum_Erase_Count"         },
    {166, "Minimum_Erase_Count"         }, {167, "Average_Erase_Count"         },
    {169, "Remaining_Life_Left"         }, {181, "Total_Program_Fail"          },
    {182, "Total_Erase_Fail"            },
};

struct sPartDef { U32 f0, f1, f2, f3; }; // cap factors 0, 1, 2, 3
static sPartDef gPart[] = {
    { 40, 30, 10, 20 },
    { 40,  0, 10,  0 },
    { 40, 10, 60, 20 },
    { 40,  0, 10,  0 },
    {  0, 20, 10, 20 },
    { 40, 20,  0, 20 },
    { 40,  0, 30,  0 },
    { 40, 40, 10, 20 },
};

static const char* gPartName[] = {
    "Windows", "Storage", "Working", "Backup",
    "Local", "Media", "Linux", "", "", "", "",
}; // 8

static tDriveArray gDrvArr;

// ------------------------------------------------------------
// Static utilities

static bool RandomError(U32 erate)  {
    return (erate >= 100) ? true : ((U32)(rand() % 100) < erate);
}

static bool RandomIndex(U32 max, U32 count, std::vector<U32>& v) {
    v.clear(); if (max < count) return false;
    for (U32 i = 0, r; i < count; i++) {
        r = RANDMOD(max);
        while(std::find(v.begin(), v.end(), r) != v.end())
            r = RANDMOD(max);
        v.push_back(r);
    }
    return true;
}

static void RandomSmartAttr(sSmartAttr& attr, sAttrDef& d) {
    attr.id = d.value;
    attr.name = d.name;
    attr.note = "N/A";
    attr.value = 100;
    attr.worst = 100;
    attr.threshold = RANDERR05 ? RANDMOD(80) : 0;
    attr.loraw = RANDERR09 ? RANDVAL16 : 0;
    attr.hiraw = RANDERR09 ? RANDVAL16 : 0;
}

static std::string GenDriveName(U32 i) {
    std::stringstream sstr;
    sstr << "\\\\.\\PhysicalDrive" << i;
    return sstr.str();
}

#define SET_PARTITON(i, f, names, offset) do { \
    if (f < 0.01) break; \
    sPartition p; p.index = i; p.name = names[i]; \
    p.cap = cap * f; p.addr.second = p.cap * 1024 * 1024 * 2; \
    p.addr.first = offset; offset += p.addr.second; pi.parr.push_back(p); } while(0)

static void RandomPartition(const sPartDef& pdef, F64 cap, sPartInfo& pi) {
    U32 offset = 0; std::vector<std::string> names;
    F32 sum = pdef.f0 + pdef.f1 + pdef.f2 + pdef.f3;
    sum = sum * 1.1; // add some spare space
    pi.parr.clear(); if (sum < 1) return;
    RAND_PICK_FROM_ARRAY(gPartName, 4, names);
    SET_PARTITON(0, pdef.f0 / sum, names, offset);
    SET_PARTITON(1, pdef.f1 / sum, names, offset);
    SET_PARTITON(2, pdef.f2 / sum, names, offset);
    SET_PARTITON(3, pdef.f3 / sum, names, offset);
}

static void InitDriveInfo(sDriveInfo& drv, sDataIndex& idx) {
    // Fill Identify info
    sIdentify& id = drv.id;
    id.model = gModel[idx.model];
    id.serial = gSerial[idx.serial];
    id.version = gVersion[idx.fwver];
    id.confid = gConfID[idx.confid];
    id.cap = gCap[idx.cap];
    id.features = gFtr[idx.ftr];

    // Fill Smart info
    tAttrMap& sm = drv.si.amap; sm.clear();

    // If SMART feature supported
    if (id.features.smart) {
        for (U32 i = 0, maxi = ARRAY_SIZE(gAttr); i < maxi; i++) {
            sSmartAttr attr; RandomSmartAttr(attr, gAttr[i]); sm[attr.id] = attr;
        }
    }

    // Fill partition info:
    RandomPartition(RAND_PICK_ITEM(gPart), id.cap, drv.pi);

    // Read some values from sm
    drv.temp = RANDMOD(70);
    drv.tread = RANDMOD(900000);
    drv.twrtn = RANDMOD(18000000);

    // overwrite values in smart-map
    SetSmartRaw(sm, 194, drv.temp, 0);
    SetSmartRaw(sm, 242, drv.tread & MASK32B, (drv.tread >> 32) & MASK32B);
    SetSmartRaw(sm, 241, drv.twrtn & MASK32B, (drv.twrtn >> 32) & MASK32B);
}

static void GenerateDriveList() {
    // Generate list of drive:
    if (gDrvArr.size()) return;
    srand(time(0));
    U32 count = ARRAY_SIZE(gIndex);

    for (U32 i = 0; i < count; i++) {
        sDriveInfo drv;
        drv.name = GenDriveName(i);
        InitDriveInfo(drv, gIndex[i]);
        gDrvArr.push_back(drv);
    }
}

static F64 gSum = 0;
static void DoTask_Calc(U32 n) {
    F64 retval = 1;
    for (U32 i = 0; i < n; i++)
        retval += i * i * i + sqrt(i*i*3.141592633333);
    gSum += retval;
}

static void DoTask_Wait(U32 nsecs) {
    Sleep(nsecs * 1000);
}

static void DoSomeTask(U32 wrate = 20, U32 nsecs = 30) {
    if (RandomError(wrate)) DoTask_Wait(nsecs);   // wait timeout
    else DoTask_Calc((RANDMOD(20)+1) * 10000000); // heavy calculation
}

// ------------------------------------------------------------
// Main StorageApi

eRetCode StorageApi::ScanDrive(tDriveArray &darr, bool rid, bool rsm, volatile sProgress *p) {
    GenerateDriveList();
    darr.clear();

    U32 max = gDrvArr.size(), cnt = rand() % MIN2(6, max) + 1;

    tDriveArray drvlst;
    RAND_PICK_FROM_CONT(gDrvArr, cnt, drvlst);

    INIT_PROGRESS(p, cnt);
    for (U32 i = 0; i < cnt; i++) {
        RETURN_IF_STOP(p, RET_ABORTED);
        darr.push_back(drvlst[i]);

        // wait some seconds:
        DoTask_Wait(RANDMOD(3) + 1);
        UPDATE_PROGRESS(p, 1);
    }

    if (RANDERR03) darr.clear();
    eRetCode rc = darr.size() ? RET_OK: RET_NOT_FOUND;
    UPDATE_RETCODE(p, rc); CLOSE_PROGRESS(p);

    return rc;
}

static eRetCode ProcessTask(volatile sProgress* p, U32 load) {
    INIT_PROGRESS(p, load); // Prepare progress
    for (U32 i = 0; i < load; i++) {
        RETURN_IF_STOP(p, RET_ABORTED);
        DoTask_Wait(1);
        UPDATE_PROGRESS(p, 1);
    }
    CLOSE_PROGRESS(p);
    return RET_OK;
}

eRetCode StorageApi::UpdateFirmware(CSTR& drvname, const U8 *bufptr, U32 bufsize, volatile sProgress* p) {
    (void) drvname; (void) bufptr; (void) bufsize;
    return ((ProcessTask(p, 40) != RET_OK) ?
                RET_ABORTED : (RANDERR09 ? RET_FAIL : RET_OK));
}

eRetCode StorageApi::SetOverProvision(CSTR& drvname, U32 factor, volatile sProgress* p) {
    (void) drvname; (void) factor;
    return ((ProcessTask(p, 5) != RET_OK) ?
                RET_ABORTED : (RANDERR09 ? RET_FAIL : RET_OK));
}

eRetCode StorageApi::OptimizeDrive(CSTR& drvname, volatile sProgress* p) {
    (void) drvname;
    return ((ProcessTask(p, 100) != RET_OK) ?
                RET_ABORTED : (RANDERR09 ? RET_FAIL : RET_OK));
}

eRetCode StorageApi::SecureErase(CSTR& drvname, volatile sProgress* p) {
    (void) drvname;
    return ((ProcessTask(p, 100) != RET_OK) ?
                RET_ABORTED : (RANDERR09 ? RET_FAIL : RET_OK));
}

eRetCode StorageApi::SelfTest(CSTR& drvname, U32 param, volatile sProgress* p) {
    (void) drvname; (void) param;
    return ((ProcessTask(p, 200) != RET_OK) ?
                RET_ABORTED : (RANDERR09 ? RET_FAIL : RET_OK));
}

eRetCode StorageApi::CloneDrive(CSTR &dstdrv, CSTR &srcdrv, tConstAddrArray &parr, volatile sProgress *p) {
    (void) dstdrv; (void) srcdrv; (void) parr;
    return ((ProcessTask(p, 2000) != RET_OK) ?
                RET_ABORTED : (RANDERR09 ? RET_FAIL : RET_OK));
}

// ------------------------------------------------------------
// End

#endif // STORAGE_API_TEST
