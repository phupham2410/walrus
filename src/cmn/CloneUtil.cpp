
#include "SystemUtil.h"
#include "CloneUtil.h"
#include "CoreUtil.h"
#include "FsUtil.h"
#include "ProcUtil.h"

#include <windows.h>
#include "winioctl.h"

using namespace std;
using namespace FsUtil;
using namespace ProcUtil;
using namespace StorageApi;
using namespace DiskCloneUtil;

#include <cstdio>
#include <string.h>
#include <diskguid.h>
#include <ntdddisk.h>
#include <processthreadsapi.h>

#define DBGMODE 1

// ----------------------------------------------------------------------------
// Temporaryly put here

#define INIT_PROGRESS(prog, load)     do { if (prog) prog->init(load); } while(0)
#define UPDATE_PROGRESS(prog, weight) do { if (prog) prog->progress += weight; } while(0)
#define CLOSE_PROGRESS(prog)          do { if (prog) prog->done = true; } while(0)
#define UPDATE_RETCODE(prog, code)    do { if (prog) prog->rval = code; } while(0)
#define UPDATE_INFO(prog, infostr)    do { if (prog) prog->info = infostr; } while(0)
#define FINALIZE_PROGRESS(prog, code) do { \
UPDATE_RETCODE(prog, code); CLOSE_PROGRESS(prog); } while(0)
#define RETURN_IF_STOP(prog, code)    do { if (prog && prog->stop) { \
    FINALIZE_PROGRESS(prog, code); return code; }} while(0)
#define FINALIZE_IF_STOP(prog, code)    do { if (prog && prog->stop) { \
    FINALIZE_PROGRESS(prog, code); return code; }} while(0)

#define SKIP_AND_CONTINUE(prog, weight) \
if (prog) { prog->progress += weight; continue; }

#define UPDATE_AND_RETURN_IF_STOP(p, w, ret) \
    UPDATE_PROGRESS(p, w); FINALIZE_IF_STOP(p, ret)

#define TRY_TO_EXECUTE_COMMAND(hdl, cmd) \
    (cmd.executeCommand((int) hdl) && (CMD_ERROR_NONE == cmd.getErrorStatus()))

#define SET_CURSTEP(p, v) if (p) do { p->priv.clone.curstep = v; } while(0)

// ----------------------------------------------------------------------------

static string GenScriptBaseName() {
    string tmp(tmpnam(NULL)); size_t len = tmp.length();
    for (U32 i = 0; i < len; i++) {
        char c = tmp[i];
        if (!isalnum(c)) tmp[i] = 'a' + (c % 24);
    }
    return tmp;
}

static string GenScriptFileName(const string& ext) {
    return GenScriptBaseName() + "." + ext;
}

static void UtilSetGuid(GUID& g, U32 d1, U16 d2, U16 d3, U64 d4) {
    g.Data1 = d1; g.Data2 = d2; g.Data3 = d3;
    for (int i = 7; i >= 0; i--) {
        g.Data4[7-i] = (d4 >> (8*i)) & 0xFF;
    }
}

static eRetCode GetDriveIndex(const std::string& name, U32& index) {
    const char* p = name.c_str();
    while(*p && !INRANGE('0', '9' + 1, *p)) p++;
    if (!*p) return RET_INVALID_ARG;
    sscanf(p, "%d", &index); return RET_OK;
}

static string GetDriveName(U32 index) {
    stringstream sstr;
    sstr << "\\\\.\\PhysicalDrive" << index;
    return sstr.str();
}

static eDcPartType ConvertGptPartitionType(GUID& src) {
    #define MAP_ITEM(d1, d2, d3, d4, val) \
    { GUID g; UtilSetGuid(g, d1, d2, d3, d4); if (src == g) return val; }
    MAP_ITEM(0xebd0a0a2, 0xb9e5, 0x4433, 0x87c068b6b72699c7, DPT_PARTITION_BASIC_DATA_GUID)
    MAP_ITEM(0x00000000, 0x0000, 0x0000, 0x0000000000000000, DPT_PARTITION_ENTRY_UNUSED_GUID)
    MAP_ITEM(0xc12a7328, 0xf81f, 0x11d2, 0xba4b00a0c93ec93b, DPT_PARTITION_SYSTEM_GUID)
    MAP_ITEM(0xe3c9e316, 0x0b5c, 0x4db8, 0x817df92df00215ae, DPT_PARTITION_MSFT_RESERVED_GUID)
    MAP_ITEM(0x5808c8aa, 0x7e8f, 0x42e0, 0x85d2e1e90434cfb3, DPT_PARTITION_LDM_METADATA_GUID)
    MAP_ITEM(0xaf9b60a0, 0x1431, 0x4f62, 0xbc683311714a69ad, DPT_PARTITION_LDM_DATA_GUID)
    MAP_ITEM(0xde94bba4, 0x06d1, 0x4d40, 0xa16abfd50179d6ac, DPT_PARTITION_MSFT_RECOVERY_GUID)
    #undef MAP_ITEM
    return DPT_INVALID;
}

static eRetCode UtilGetDriveLayout(HDL hdl, U8* buffer, U32 bufsize) {
    if (!buffer) return StorageApi::RET_INVALID_ARG;
    memset(buffer, 0x00, bufsize);

    BOOL ret; DWORD rsize;
    ret = DeviceIoControl((HANDLE) hdl, IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                          NULL, 0, buffer, bufsize, &rsize, NULL);

    return ret ? RET_OK : RET_FAIL;
}

static void ConvertPartInfo(PARTITION_INFORMATION_EX& p, sDcPartInfo& dpi) {
    dpi.pidx = p.PartitionNumber;
    dpi.start = p.StartingOffset.QuadPart;
    dpi.psize = p.PartitionLength.QuadPart;
    dpi.nsize = dpi.psize;

    switch(p.PartitionStyle) {
    case PARTITION_STYLE_RAW:
    case PARTITION_STYLE_MBR: dpi.ptype = DPT_PARTITION_BASIC_DATA_GUID; break;
    case PARTITION_STYLE_GPT: dpi.ptype = ConvertGptPartitionType(p.Gpt.PartitionType); break;
    default: break;
    }
}

static eRetCode UtilOpenFile(U32 drvidx, HDL& hdl) {
    string name = GetDriveName(drvidx);
    if (RET_OK != StorageApi::Open(name, hdl)) return RET_FAIL;
    return RET_OK;
}

static eRetCode UtilSeekFile(HDL hdl, S64 offset, U8 startpos) {
    LARGE_INTEGER low, high; high.QuadPart = 0; low.QuadPart = offset;
    startpos %= 3;
    if (SetFilePointerEx((HANDLE)hdl, low, &high, startpos) == INVALID_SET_FILE_POINTER)
        return RET_FAIL;
    return RET_OK;
}

static bool ToGptPartTypeString(U32 t, string& tstr) {
    tstr = "";
    switch(t) {
        #define MAP_ITEM(key, value)  case key: tstr = value; break;
        MAP_ITEM(DPT_PARTITION_BASIC_DATA_GUID, "primary")
        MAP_ITEM(DPT_PARTITION_SYSTEM_GUID, "efi")
        MAP_ITEM(DPT_PARTITION_MSFT_RESERVED_GUID, "msr")
        default: return false;
        #undef MAP_ITEM
    }
    return true;
}

static bool ToGptPartIdString(U32 t, string& id) {
    id = "";
    switch(t) {
        #define MAP_ITEM(key, value)  case key: id = value; break;
        MAP_ITEM(DPT_PARTITION_MSFT_RECOVERY_GUID, "de94bba4-06d1-4d40-a16a-bfd50179d6ac")
        MAP_ITEM(DPT_PARTITION_ENTRY_UNUSED_GUID, "00000000-0000-0000-0000-000000000000")
        MAP_ITEM(DPT_PARTITION_LDM_METADATA_GUID, "5808c8aa-7e8f-42e0-85d2-e1e90434cfb3")
        MAP_ITEM(DPT_PARTITION_LDM_DATA_GUID, "af9b60a0-1431-4f62-bc68-3311714a69ad")
        default: return false;
        #undef MAP_ITEM
    }
    return true;
}

static void UpdateLinks(sDcVolInfo& vi) {
    char buffer[256]; string tmp = GenScriptBaseName();

    if (1) {
        sprintf(buffer, "%c:\\%s", vi.orgltr, tmp.c_str()); vi.topdir = string(buffer);
        sprintf(buffer, "%s\\sha", vi.topdir.c_str()); vi.shalink = string(buffer);
        sprintf(buffer, "%s\\mnt", vi.topdir.c_str()); vi.mntlink = string(buffer);
    }
}

static void UpdateVolInfo(const tVolArray& va, set<char>& lset, U32 drvidx, sDcPartInfo& pi) {
    U64 minpos = pi.start, maxpos = minpos + pi.psize;
    sDcVolInfo& vi = pi.vi;
    U32 volcount = 0;

    // search for volumes in this partition on drive drvidx
    for (auto& v : va) {
        for (auto& d : v.di) {
            if (d.drvidx != drvidx) continue;
            if (d.offset < minpos) continue;
            if ((d.offset + d.length) > maxpos) continue;

            // the volume is located in this partition
            vi.tarltr = 0;
            vi.orgltr = v.letter; UpdateLinks(vi);
            if (lset.size()) {
                set<char>::iterator first = lset.begin();
                vi.tarltr = *first; lset.erase(first);
            }
            vi.usedsize = v.usedsize; // this value may be larger than partition size
                                      // in the case of expanded volume
            volcount++; continue;
        }
    }
    vi.valid = (volcount == 1);
}

eRetCode DiskCloneUtil::GetDriveInfo(U32 srcidx, sDcDriveInfo& si) {
    HDL hdl;
    if (RET_OK != UtilOpenFile(srcidx, hdl)) return RET_INVALID_ARG;

    tVolArray va; FsUtil::ScanVolumeInfo(va);
    set<char> used, unused; FsUtil::GetLetterSet(va, used, unused);

    // Read DriveLayout and Partitions
    // Fill in si

    si.drvidx = 0; si.parr.clear();

    eRetCode ret;
    const U32 bufsize = 4096; U8 buffer[bufsize];
    memset(buffer, 0x00, bufsize);

    ret = UtilGetDriveLayout(hdl, buffer, bufsize);
    if (ret != RET_OK) { Close(hdl); return ret; }

    DRIVE_LAYOUT_INFORMATION_EX* dli = (DRIVE_LAYOUT_INFORMATION_EX*) buffer;
    U32 count = dli->PartitionCount;
    PARTITION_INFORMATION_EX* pi = (PARTITION_INFORMATION_EX*) &dli->PartitionEntry[0];

    si.drvidx = srcidx;

    for (U32 i = 0; i < count; i++, pi++) {
        PARTITION_INFORMATION_EX& p = *pi;
        if (!p.PartitionNumber) continue;
        sDcPartInfo dpi;
        ConvertPartInfo(p, dpi);
        UpdateVolInfo(va, unused, srcidx, dpi);
        si.parr.push_back(dpi);
    }

    Close(hdl);
    return RET_OK;
}

static eRetCode MatchPartition(U64 addr, U64 size, const sDcDriveInfo& si, U32& idx) {
    const vector<sDcPartInfo>& parr = si.parr;
    for (U32 i = 0, pcnt = parr.size(); i < pcnt; i++) {
        const sDcPartInfo& p = parr[i];
        if (p.start != addr) continue;
        if (p.psize > size) return RET_CONFLICT;
        idx = i; return RET_OK;
    }
    return RET_FAIL;
}

#define CAP1MB ((U64)1 << 20)
#define CAP1KB ((U64)1 << 10)
#define CAP100KB ((U64)100 << 10)

#define MSK1MB (CAP1MB - 1)

eRetCode DiskCloneUtil::FilterPartition(tConstAddrArray &parr, sDcDriveInfo& si) {
    vector<sDcPartInfo> out;
    set<U64> aset; // address map: address -> size;

    // address matching and filtering
    for (U32 i = 0, pcnt = parr.size(); i < pcnt; i++) {
        const tPartAddr& pa = parr[i];
        U64 addr = pa.first, size = pa.second; U32 idx;
        if (aset.find(addr) != aset.end())
            return StorageApi::RET_CONFLICT;
        if (RET_OK != MatchPartition(addr, size, si, idx))
            return StorageApi::RET_NOT_FOUND;
        aset.insert(addr);
        sDcPartInfo info = si.parr[idx];
        size = ((size + CAP1MB - 1) / CAP1MB) * CAP1MB; // round to MB
        info.nsize = size; out.push_back(info);
    }

    si.parr = out; return out.size() ? RET_OK : RET_EMPTY;
}

// update start & index of target drive
eRetCode DiskCloneUtil::GenDestRange(const sDcDriveInfo& si, U32 dstidx, sDcDriveInfo& di) {
    U64 start = CAP1MB; // start at offset 1MB
    U64 gap = CAP1MB;   // gap 1MB between partitions --> must MB align
    di.drvidx = dstidx; di.parr.clear();
    for (U32 i = 0, maxi = si.parr.size(); i < maxi; i++) {
        const sDcPartInfo& s = si.parr[i];
        sDcPartInfo d = s;
        d.pidx = di.parr.size() + 1; // start from 1
        d.start = start; d.psize = d.nsize = s.nsize;
        di.parr.push_back(d);
        start = start + (d.psize + gap);
        if (DBGMODE) {
            if (d.start & MSK1MB)
                cout << "ERROR: Offset must be MB aligned: " << d.start << endl;
        }
    }
    return di.parr.size() ? RET_OK : RET_EMPTY;
}

eRetCode DiskCloneUtil::RemovePartTable(U32 dstidx) {
    HDL hdl;
    if (RET_OK != UtilOpenFile(dstidx, hdl)) return RET_FAIL;

    stringstream sstr;
    const int zsize = 4096; U8 zbuf[zsize] = {0};

    if (RET_OK != UtilSeekFile(hdl, 0, 0)) return RET_FAIL;
    WriteFile((HANDLE) hdl, zbuf, zsize, NULL, NULL); // Master table

    if (RET_OK != UtilSeekFile(hdl, -zsize, 2)) return RET_FAIL;
    WriteFile((HANDLE) hdl, zbuf, zsize, NULL, NULL); // Slave table

    Close(hdl);
    return RET_OK;
}

eRetCode DiskCloneUtil::GenPrepareScript(const sDcDriveInfo& di, std::string& script) {
    // Prepare script:
    // 1. create mount points for target partitions

    stringstream sstr; U32 count = 0;
    for (U32 i = 0; i < di.parr.size(); i++) {
        const sDcPartInfo& pi = di.parr[i]; string subscr;
        if (!pi.vi.valid) continue;
        if(count++) sstr << " & ";
        sstr << "mkdir " << pi.vi.topdir << " & ";
        sstr << "attrib +H " << pi.vi.topdir << " & ";
        sstr << "mkdir " << pi.vi.mntlink;
    }

    script = sstr.str();

    if (DBGMODE) {
        cout << "Create preparation script: " << endl << script << endl;
    }

    return RET_OK;
}

static eRetCode GenCreatePartScript(const sDcPartInfo& d, string& script) {
    stringstream sstr;
    string typestr, idstr;

    // for dst partition, d.psize == p.nsize

    U64 size_in_mb = ((d.psize + CAP1MB - 1) / CAP1MB);
    U64 offset_in_kb = ((d.start + CAP1KB - 1) / CAP1KB);

    if(DBGMODE) {
        if (d.psize != d.nsize)
            cout << "ERROR: invalid dst partition info" << endl;

        if (size_in_mb != (d.psize >> 20))
            cout << "ERROR: odd d.psize " << d.psize << endl;

        if (offset_in_kb != (d.start >> 10))
            cout << "ERROR: odd d.start " << d.start << endl;
    }

    if (ToGptPartTypeString(d.ptype, typestr)) {
        sstr << "create partition " << typestr
             << " size=" << size_in_mb
             << " offset=" << offset_in_kb;
    }
    else if(ToGptPartIdString(d.ptype, idstr)) {
        sstr << "create partition primary"
             << " id=" << idstr
             << " size=" << size_in_mb
             << " offset=" << offset_in_kb;
    }

    if(d.vi.valid) {
        sstr << endl;
        sstr << "format fs=ntfs quick" << endl;
        sstr << "assign mount=\"" << d.vi.mntlink << "\"";

        if (0) if (d.vi.tarltr) {
            sstr << endl;
            sstr << "assign letter=" << d.vi.tarltr;
        }
    }

    script = sstr.str();
    return RET_OK;
}

eRetCode DiskCloneUtil::GenCreatePartScript(const sDcDriveInfo& di, std::string& script) {
    // DiskPart script:
    // 1. create target drive: di.drvidx
    //    diskpart
    //    select disk dstidx
    //    create partition primary size=?? offset=??
    //    exit

    stringstream sstr;
    sstr << "select disk " << di.drvidx << endl;
    sstr << "clean" << endl;
    sstr << "convert gpt" << endl;
    sstr << "select partition 1" << endl;
    sstr << "delete partition override" << endl;

    for (U32 i = 0; i < di.parr.size(); i++) {
        const sDcPartInfo& pi = di.parr[i]; string subscr;
        if (RET_OK != ::GenCreatePartScript(pi, subscr)) return RET_FAIL;
        sstr << subscr << endl;
    }

    sstr << "exit" << endl;
    script = sstr.str();

    if (DBGMODE) {
        cout << "Create partition script: " << endl << script << endl;
    }

    return RET_OK;
}

eRetCode DiskCloneUtil::VerifyPartition(U32 dstidx, const sDcDriveInfo& di) {
    do {
        sDcDriveInfo ti;
        if (RET_OK != GetDriveInfo(dstidx, ti)) {
            if (DBGMODE) cout << "Cannot get driveinfo" << endl;
            break;
        }

        if (ti.drvidx != di.drvidx) {
            if (DBGMODE) cout << "drvidx mismatched" << endl;
            break;
        }

        U32 count = ti.parr.size(), i = 0;
        if (count != di.parr.size()) {
            if (DBGMODE) cout << "drvcnt mismatched" << endl;
            break;
        }

        for (i = 0; i < count; i++) {
            const sDcPartInfo& t = ti.parr[i];
            const sDcPartInfo& d = di.parr[i];

            if (DBGMODE) do {
                cout << "VerifyPartition " << i << ": ";
                #define MAP_ITEM(v) if (t.v != d.v) do { cout << "Invalid " << #v \
                         << " val: " << t.v << " exp: " << d.v << endl; break; } while(0)
                MAP_ITEM(pidx); MAP_ITEM(ptype); MAP_ITEM(start); MAP_ITEM(psize);
                #undef MAP_ITEM
                cout << "OK" << endl;
            } while(0);

            if ((t.pidx != d.pidx) || (t.ptype != d.ptype)
               || (t.start != d.start) || (t.psize != d.psize)) break;
        }

        if (i != count) break;
        return RET_OK;
    } while(0);

    if (DBGMODE) cout << "Verify partition failed" << endl;
    return RET_FAIL;
}

static eRetCode CopyPartition(
    HDL shdl, U64 sadd, U64 size,
    HDL dhdl, U64 dadd, volatile sProgress* p) {
    // Copy data block from src_drv to dst_drv
    if ((size % 512) || (sadd % 512) || (dadd % 512)) return RET_INVALID_ARG;

    U32 buf_sec = 256, buf_size = buf_sec << 9;
    U8* buf = (U8*) malloc(buf_size);
    if (!buf) return StorageApi::RET_OUT_OF_MEMORY;
    memset(buf, 0x5A, buf_size);

    U32 wrk_sec = buf_sec;
    U64 src_sec = size >> 9, rem_sec = src_sec;

    // Seeking
    bool seekstatus = (RET_OK == UtilSeekFile(shdl, sadd, 0)) &&
                      (RET_OK == UtilSeekFile(dhdl, dadd, 0));
    if (!seekstatus) { SET_CURSTEP(p, 65); return RET_FAIL; }

    // threshold to update progress
    U64 load = 0, thr = rem_sec / 100, progress = 0;

    while(rem_sec) {
        wrk_sec = MIN2(wrk_sec, rem_sec);

        // copy from src_lba to dst_lba
        DWORD tmp_size = 0, read_size = wrk_sec << 9;
        if (TRUE != ReadFile((HANDLE) shdl, buf, read_size, &tmp_size, NULL)) {
            SET_CURSTEP(p, 66); break;
        }
        if (TRUE != WriteFile((HANDLE) dhdl, buf, tmp_size, NULL, NULL)) {
            SET_CURSTEP(p, 67); break;
        }
        if (!tmp_size) { SET_CURSTEP(p, 68); break; }
        wrk_sec = tmp_size >> 9; load += wrk_sec;
        UPDATE_AND_RETURN_IF_STOP(p, wrk_sec, RET_ABORTED);

        if (load > thr) {
            load = 0; progress++;
            if (DBGMODE) cout << "Progress: " << progress << "%" << endl;
        }
        rem_sec -= wrk_sec;
    }
    if (DBGMODE) {
        if (!rem_sec) cout << "Progress: 100%";
        cout << endl;
    }
    return rem_sec ? RET_FAIL : RET_OK;
}

eRetCode DiskCloneUtil::CopyShadow(
    const string& slnk, const string& dlnk,
    tCopyLogMap& reslog, volatile sProgress* p) {

    // copy data from link to link using CopyItem
    S8 cmdbuffer[1024]; string output;
    string src = "\"" + slnk + "\\*\"", dst = "\"" + dlnk + "\"";
    sprintf(cmdbuffer, "powershell Copy-Item -Path %s -Destination %s -Recurse",
            src.c_str(), dst.c_str());

    // start child process to copy shadow
    eRetCode ret = ProcUtil::ExecCommandWithLog(string(cmdbuffer), &output);

    // parse output to get uncopied files:
    ProcUtil::ParseCopyLog(output, reslog);

    return ret;
}

struct sMonitorData {
    eRetCode rval; // return code of monitor thread
                   // will be equal to p->rval
                   // used this value in case p == NULL

    // control monthread
    volatile bool stop;
    volatile bool done;

    // monitor copy progress
    string srclink;
    string dstlink;
    volatile sProgress* p;
    U64 dstused; // current dest size in byte

    // monitor killrequest
    volatile U64 pid; // kill this process if p->stop == true
};

static void ParseUsageLog(const string& str, U64& used, U64& total) {
    stringstream sstr(str); string line, lastline;
    const string key = "of disk in use";
    while(getline(sstr, line, '\n')) {
        if (line.find(key) != string::npos) {
            lastline = line;
        }
    }
    used = total = 0; // byte unit
    string usedstr, totalstr;

    size_t delimpos = lastline.find("/");
    lastline = lastline.substr(0, delimpos);

    size_t p0 = 0, p1;
    while(1) {
        p1 = lastline.find(",", p0);
        if (p1 == string::npos) {
            usedstr += lastline.substr(p0);
            break;
        }
        else {
            usedstr += lastline.substr(p0, p1 - p0);
            p0 = p1 + 1;
        }
    }

    used =  stoull(usedstr.c_str());
    total =  0;
}

DWORD WINAPI MonitorThreadFunc(void* data) {
    sMonitorData* md = (sMonitorData*) data;

    U64 used, total, diff, retry = 0;
    S8 cmdbuffer[1024]; string out, cmd, flog; bool termreq = false;
    sprintf(cmdbuffer, "diskusage %s", md->dstlink.c_str());
    cmd = string(cmdbuffer); flog = md->dstlink + ".log";

    while (!md->stop) {
        if (retry >= 4) {
            termreq = true;
            md->rval = RET_TIME_OUT;
        }
        else if (md->p && md->p->stop) {
            termreq = true;
            md->rval = RET_ABORTED;
            md->p->rval = md->rval;
        }

        if (termreq) {
            HANDLE hdl = (HANDLE) md->pid;
            if (hdl != INVALID_HANDLE_VALUE) {
                TerminateJobObject(hdl, 1);
                CloseHandle(hdl);
            }
            break;
        }

        // get current size of data under dstlink
        if (RET_OK == ProcUtil::ExecDiskUsageScript(cmd, flog, out)) {
            ParseUsageLog(out, used, total);
            diff = used - md->dstused; md->dstused = used;
            if (md->p) md->p->progress += diff >> 9;
            if (!diff) retry++; else retry = 0;
        }
        Sleep(4000);
    }
    md->done = true; return 0;
}

eRetCode DiskCloneUtil::CopyShadowWithMonThread(
    const string& slnk, const string& dlnk,
    tCopyLogMap& rlog, volatile sProgress* p) {

    sMonitorData mon;
    mon.p = p; mon.stop = mon.done = false; mon.pid = (U64)INVALID_HANDLE_VALUE;
    mon.srclink = slnk; mon.dstlink = dlnk; mon.dstused = 0; mon.rval = RET_OK;
    HANDLE mhdl = CreateThread(NULL, 0, MonitorThreadFunc, &mon, 0, NULL);
    if (INVALID_HANDLE_VALUE == mhdl) { SET_CURSTEP(p, 62); return RET_FAIL; }

    // copy data from link to link using CopyItem
    S8 cmdbuffer[1024]; string output, cmd;
    string src = "\"" + slnk + "\\*\"", dst = "\"" + dlnk + "\"";
    sprintf(cmdbuffer, "powershell Copy-Item -Path %s -Destination %s -Recurse",
            src.c_str(), dst.c_str());
    cmd = string(cmdbuffer);

    // start child process to copy items
    eRetCode ret = ProcUtil::ExecCopyItemCommand(cmd, &rlog, &mon.pid);
    mon.stop = true; while(!mon.done); CloseHandle(mhdl);

    // cannot start copy process
    if (ret != RET_OK) { SET_CURSTEP(p, 63); return RET_FAIL; }

    // monthread returned abnormally
    if (mon.rval != RET_OK) { SET_CURSTEP(p, 64); return mon.rval; }

    return RET_OK;
}

eRetCode DiskCloneUtil::ClonePartitions(
    const sDcDriveInfo& si, const sDcDriveInfo& di, volatile sProgress* p) {
    U32 pcnt = si.parr.size();
    if (pcnt != di.parr.size()) return StorageApi::RET_INVALID_ARG;

    HDL shdl, dhdl;
    if (RET_OK != UtilOpenFile(si.drvidx, shdl)) return RET_FAIL;
    if (RET_OK != UtilOpenFile(di.drvidx, dhdl)) { Close(shdl); return RET_FAIL; }

    U32 i; eRetCode ret = RET_OK;
    for (i = 0; i < pcnt; i++) {
        const sDcPartInfo& sp = si.parr[i];
        const sDcPartInfo& dp = di.parr[i];
        if (DBGMODE) cout << "Copy partition " << i << endl;

        if (sp.vi.valid) { // Volume partition --> copy shadow
            tCopyLogMap reslog;
            ret = CopyShadowWithMonThread(sp.vi.shalink, dp.vi.mntlink, reslog, p);
            if (RET_OK != ret) break;
        }
        else { // System partiton --> copy sectors
            ret = CopyPartition(shdl, sp.start, sp.psize, dhdl, dp.start, p);
            if (RET_OK != ret) break;
        }
    }

    Close(shdl); Close(dhdl);
    return ret;
}

static eRetCode InitProgress(const sDcDriveInfo& di, volatile sProgress* p) {
    // calculate copy size and init progress info
    U64 ttl_sec = 0;
    for (U32 i = 0, maxi = di.parr.size(); i < maxi; i++) {
        const sDcPartInfo& p = di.parr[i];
        if (!p.vi.valid) {
            ttl_sec += p.psize >> 9;
        }
        else {
            ttl_sec += p.vi.usedsize >> 9;
        }
    }

    INIT_PROGRESS(p, ttl_sec);
    return RET_OK;
}

// --------------------------------------------------------------------------------
// ShadowCopy implementation

static eRetCode ParseShadowID(const string& str, string& result) {
    stringstream sstr(str);
    string line;
    const string prefix = "ShadowID = \"{";
    const string postfix = "}\"";
    while(getline(sstr, line, '\n')) {
        size_t pos0 = line.find(prefix);
        if (pos0 == string::npos) continue;
        pos0 += prefix.length();
        size_t pos1 = line.find(postfix);
        if (pos1 == string::npos) continue;
        if (line[pos1 - 1] == '\r') pos1--;
        result = line.substr(pos0, pos1 - pos0);
        return RET_OK;
    }
    return RET_NOT_FOUND;
}

static eRetCode ParseShadowVolume(const string& str, string& vol) {
    stringstream sstr(str);
    string line;
    const string prefix = "Shadow Copy Volume: ";
    while(getline(sstr, line, '\n')) {
        size_t pos0 = line.find(prefix);
        if (pos0 == string::npos) continue;
        pos0 += prefix.length();
        vol = line.substr(pos0);
        size_t len = vol.length();
        if (len && (vol[len-1] == '\r')) vol.erase(len - 1);
        return RET_OK;
    }
    return RET_NOT_FOUND;
}

static eRetCode ShadowCopy_Create(vector<sDcPartInfo>& parr) {
    for (U32 i = 0, maxi = parr.size(); i < maxi; i++) {
        sDcPartInfo& info = parr[i];
        if (!info.vi.valid) continue;
        sDcVolInfo& vi = info.vi;
        char cmdbuffer[1024]; string output;

        // create shadow copy
        do {
            sprintf(cmdbuffer, "wmic shadowcopy call create volume=%c:\\", vi.orgltr);
            if (RET_OK != ProcUtil::ExecCommandWithLog(string(cmdbuffer), &output)) return RET_FAIL;
            if (RET_OK != ParseShadowID(output, vi.shaid)) return RET_FAIL;
        } while(0);

        // get shadow volume
        do {
            sprintf(cmdbuffer, "vssadmin list shadows /shadow=\"{%s}\"", vi.shaid.c_str());
            if (RET_OK != ProcUtil::ExecCommandWithLog(string(cmdbuffer), &output)) return RET_FAIL;
            if (RET_OK != ParseShadowVolume(output, vi.shavol)) return RET_FAIL;
        } while(0);
    }

    return RET_OK;
}

static eRetCode ShadowCopy_MakeLink(vector<sDcPartInfo>& parr) {
    stringstream sstr; U32 count = 0; char cmdbuffer[1024];
    for (U32 i = 0, maxi = parr.size(); i < maxi; i++) {
        sDcPartInfo& info = parr[i];
        if (!info.vi.valid) continue;
        sDcVolInfo& vi = info.vi;

        // remove old mount point, create the new one
        sprintf(cmdbuffer, "rmdir %s /S /Q", vi.shalink.c_str());
        if (count++) sstr << " & "; sstr << cmdbuffer;

        sprintf(cmdbuffer, "mklink /d %s %s\\", vi.shalink.c_str(), vi.shavol.c_str());
        if (count++) sstr << " & "; sstr << cmdbuffer;
    }

    return ProcUtil::ExecCommandOnly(sstr.str());
}

static eRetCode ShadowCopy_Cleanup(vector<sDcPartInfo>& parr) {
    stringstream sstr; U32 count = 0; char cmdbuffer[1024];
    for (U32 i = 0, maxi = parr.size(); i < maxi; i++) {
        sDcPartInfo& info = parr[i];
        if (!info.vi.valid) continue;
        sDcVolInfo& vi = info.vi;

        // delete shadow copy & working dir
        sprintf(cmdbuffer, "vssadmin delete shadows /shadow=\"{%s}\" /Quiet", vi.shaid.c_str());
        if (count++) sstr << " & "; sstr << cmdbuffer;

        sprintf(cmdbuffer, "rmdir %s /S /Q", vi.topdir.c_str());
        if (count++) sstr << " & "; sstr << cmdbuffer;
    }

    return ProcUtil::ExecCommandOnly(sstr.str());
}

eRetCode DiskCloneUtil::HandleCloneDrive_ShadowCopy(
    CSTR& dstdrv, CSTR& srcdrv, tConstAddrArray& parr, volatile sProgress* p) {
    U32 sidx, didx;
    if (RET_OK != GetDriveIndex(srcdrv, sidx)) return RET_INVALID_ARG;
    if (RET_OK != GetDriveIndex(dstdrv, didx)) return RET_INVALID_ARG;

    sDcDriveInfo si, di; string script;
    do {
        if (RET_OK != GetDriveInfo(sidx, si)) { SET_CURSTEP(p, 10); break; }
        if (RET_OK != FilterPartition(parr, si)) { SET_CURSTEP(p, 11); break; }
        if (RET_OK != GenDestRange(si, didx, di)) { SET_CURSTEP(p, 12); break; }
        if (RET_OK != RemovePartTable(didx)) { SET_CURSTEP(p, 13); break; }

        if (RET_OK != ShadowCopy_Create(di.parr)) { SET_CURSTEP(p, 20); break; }

        // Step 1. Preparation
        //     + Create mount point for target volumes
        if (RET_OK != GenPrepareScript(di, script)) { SET_CURSTEP(p, 30); break; }
        if (RET_OK != ProcUtil::ExecCommandOnly(script)) { SET_CURSTEP(p, 31); break; }

        // Step 2. Create partitions on target drive
        if (RET_OK != GenCreatePartScript(di, script)) { SET_CURSTEP(p, 40); break; }
        if (RET_OK != ProcUtil::ExecDiskPartScript(script)) { SET_CURSTEP(p, 41); break; }
        if (RET_OK != VerifyPartition(didx, di)) { SET_CURSTEP(p, 42); break; }

        // Step 3. Create shadows objects
        if (RET_OK != ShadowCopy_MakeLink(di.parr)) { SET_CURSTEP(p, 50); break; }

        // Step 4. Start clone process
        if (RET_OK != InitProgress(di, p)) { SET_CURSTEP(p, 60); break; }
        if (RET_OK != ClonePartitions(si, di, p)) break; // CURSTEP updated inside

        // Step 3. Create shadows objects
        if (RET_OK != ShadowCopy_Cleanup(di.parr)) { SET_CURSTEP(p, 70); break; }

        SET_CURSTEP(p, 99); FINALIZE_PROGRESS(p, RET_OK); return RET_OK;
    } while(0);

    ShadowCopy_Cleanup(di.parr); // Force cleanup

    if (p) {
        eRetCode rval = p->rval;
        // no specific error found -> set to fail
        if (rval == RET_OK) rval = RET_FAIL;

        if (!p->workload) INIT_PROGRESS(p, 0);
        FINALIZE_PROGRESS(p, rval);
    }

    if (DBGMODE) cout << "FAIL" << endl;
    return RET_FAIL;
}

// --------------------------------------------------------------------------------
// Test Clone Drive

#define ARRAY_SIZE(name) (sizeof(name) / sizeof(name[0]))

eRetCode DiskCloneUtil::TestCloneDrive(U32 didx, U32 sidx, U64 extsize) {
    string srcdrv = GetDriveName(sidx);
    string dstdrv = GetDriveName(didx);

    // This is partition_index_number, not index in the src list
    U32 part_num[] = { 4, 2, 6 }; U32 part_cnt = ARRAY_SIZE(part_num);

    // read srcdrv to get list of partitions
    // select partitions and build input param
    // call HandleCLoneDrive

    sDcDriveInfo si;
    if (RET_OK != GetDriveInfo(sidx, si)) {
        cout << "Cannot read info from drive " << sidx << endl;
        return RET_FAIL;
    }

    tAddrArray parr;
    for (U32 i = 0; i < part_cnt; i++) {
        U32 cur_num = part_num[i];

        for (U32 j = 0, maxj = si.parr.size(); j < maxj; j++) {
            sDcPartInfo& pi = si.parr[j];
            if (pi.pidx != cur_num) continue;
            // build start & size params
            tPartAddr pa;
            pa.first = pi.start;
            pa.second = pi.psize + extsize; // rounding ?
            parr.push_back(pa);

            if (1) {
                U64 offset = pa.first;
                U64 length = pa.second;

                if (offset % 512) cout << "Invalid offset (odd) " << endl;
                if (length % 512) cout << "Invalid length (odd) " << endl;

                cout << "Selecting partition " << cur_num << " ";
                cout << "offset: " << (offset >> 9) << " (lbas) ";
                cout << "length: " << (length >> 9) << " (sectors) ";
                cout << endl;
            }

            break;
        }
    }

    return HandleCloneDrive_ShadowCopy(dstdrv, srcdrv, parr);
}

eRetCode DiskCloneUtil::TestMisc() {

    string slink = "C:\\sha";
    string dlink = "C:\\target";
    tCopyLogMap rlog;
    CopyShadow(slink, dlink, rlog, NULL);

    for (auto &rl : rlog) {
        cout << "Error: " << rl.first << endl;
        for (auto &it : rl.second) {
            if (it.filename.length()) cout << "   + File: " << it.filename;
            else cout << "   + Message: " << it.message;
            cout << endl;
        }
    }
    return RET_OK;
}
