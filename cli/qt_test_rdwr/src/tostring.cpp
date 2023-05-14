#include <sstream>
#include "StorageApi.h"
#include "SystemUtil.h"
#include "HexFrmt.h"
#include "CommonUtil.h"

#include <fileapi.h>
#include <errhandlingapi.h>

#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string>

#include <diskguid.h>
#include <ntdddisk.h>

#include <locale>
#include <codecvt>

using namespace std;
using namespace StorageApi;

#define UB 0x00000001
#define US 0x00000002
#define UK 0x00000004
#define UM 0x00000008
#define UG 0x00000010
#define UT 0x00000020

string ToString(PARTITION_STYLE& ps) {
    stringstream sstr;
    switch(ps) {
    case PARTITION_STYLE_MBR: sstr << "MRB"; break;
    case PARTITION_STYLE_GPT: sstr << "GPT"; break;
    case PARTITION_STYLE_RAW: sstr << "RAW"; break;
    default: sstr << "UNK"; break;
    }
    return sstr.str();
}

string ToValueString(U64 val, U32 code) {
    stringstream sstr;
    if (code & UB) sstr << " " << (val      ) << " (bytes)";
    if (code & UK) sstr << " " << (val >> 10) << " (KB)";
    if (code & UM) sstr << " " << (val >> 20) << " (MB)";
    if (code & UG) sstr << " " << (val >> 30) << " (GB)";
    if (code & UT) sstr << " " << (val >> 40) << " (TB)";
    if (code & US) sstr << " " << (val >>  9) << " (sectors)";
    return sstr.str();
}

string ToString(GUID& g) {
    stringstream sstr;

#define GUIDFRMT(val, w) hex << setfill('0') << setw(w) << ((U32)val)

    sstr << GUIDFRMT(g.Data1, 8) << "-";
    sstr << GUIDFRMT(g.Data2, 4) << "-";
    sstr << GUIDFRMT(g.Data3, 4) << "-";
    for (U32 i = 0; i < 2; i++)
        sstr << GUIDFRMT(g.Data4[i], 2);
    sstr << "-";
    for (U32 i = 2; i < 8; i++)
        sstr << GUIDFRMT(g.Data4[i], 2);
    return sstr.str();
}

string ToPartitionTypeString(GUID& g) {
    string key = ToString(g);

    #define MAP_ITEM(str,name) if (key == str) return #name;
    MAP_ITEM("ebd0a0a2-b9e5-4433-87c0-68b6b72699c7", PARTITION_BASIC_DATA_GUID)
    MAP_ITEM("00000000-0000-0000-0000-000000000000", PARTITION_ENTRY_UNUSED_GUID)
    MAP_ITEM("c12a7328-f81f-11d2-ba4b-00a0c93ec93b", PARTITION_SYSTEM_GUID)
    MAP_ITEM("e3c9e316-0b5c-4db8-817d-f92df00215ae", PARTITION_MSFT_RESERVED_GUID)
    MAP_ITEM("5808c8aa-7e8f-42e0-85d2-e1e90434cfb3", PARTITION_LDM_METADATA_GUID)
    MAP_ITEM("af9b60a0-1431-4f62-bc68-3311714a69ad", PARTITION_LDM_DATA_GUID)
    MAP_ITEM("de94bba4-06d1-4d40-a16a-bfd50179d6ac", PARTITION_MSFT_RECOVERY_GUID)
    #undef MAP_ITEM

    return "PARTITION_GUID_INVALID";
}

string ToGptAttriuteString(U64& attr) {
    stringstream sstr;
    #define MAP_ITEM(val,name) case val: return #name;
    switch(attr) {
        MAP_ITEM(0x0000000000000001, GPT_ATTRIBUTE_PLATFORM_REQUIRED)
        MAP_ITEM(0x8000000000000000, GPT_BASIC_DATA_ATTRIBUTE_NO_DRIVE_LETTER)
        MAP_ITEM(0x4000000000000000, GPT_BASIC_DATA_ATTRIBUTE_HIDDEN)
        MAP_ITEM(0x2000000000000000, GPT_BASIC_DATA_ATTRIBUTE_SHADOW_COPY)
        MAP_ITEM(0x1000000000000000, GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY)
    default: return "GPT_ATTRIBUTE_INVALID";
    }
    #undef MAP_ITEM

    return sstr.str();
}

string ToString(PARTITION_INFORMATION_MBR& pi) {
    stringstream sstr;

    sstr << "MRB::Type: " << (U32) pi.PartitionType << endl;
    sstr << "MRB::Boot: " << (pi.BootIndicator ? "True" : "False") << endl;
    sstr << "MRB::Reco: " << (pi.RecognizedPartition ? "True" : "False") << endl;
    sstr << "MRB::Hsec: " << pi.HiddenSectors;
    return sstr.str();
}

string ToString(PARTITION_INFORMATION_GPT& pi) {
    stringstream sstr;

    wstring wname = wstring(pi.Name);
    using convert_type = std::codecvt_utf8<wchar_t>;
    wstring_convert<convert_type, wchar_t> converter;
    string name = converter.to_bytes(wname);

    sstr << "GPT::Type: " << ToString(pi.PartitionType)
         << " (" << ToPartitionTypeString(pi.PartitionType) << ")" << endl;
    sstr << "GPT::Id:   " << ToString(pi.PartitionType) << endl;
    sstr << "GPT::Attr: 0x" << hex << setw(16) << setfill('0') << pi.Attributes
         << " (" << ToGptAttriuteString(pi.Attributes) << ")" << endl;
    sstr << "GPT::Name: " << name << endl;
    return sstr.str();
}

string ToString(PARTITION_INFORMATION& pi) {
    stringstream sstr;

    sstr << "Style: " << (U32) pi.PartitionType << endl;
    sstr << "Index: " << pi.PartitionNumber << endl;
    sstr << "Rewrt: " << (pi.RewritePartition ? "True" : "False") << endl;
    sstr << "Offset: " << ToValueString(pi.StartingOffset.QuadPart, UB | US | UG) << endl;
    sstr << "Length: " << ToValueString(pi.PartitionLength.QuadPart, UB | US | UG) << endl;
    sstr << "Boot: " << (pi.BootIndicator ? "True" : "False") << endl;
    sstr << "Hsec: " << pi.HiddenSectors << endl;
    sstr << "Reco: " << (pi.RecognizedPartition ? "True" : "False") << endl;

    return sstr.str();
}

string ToString(PARTITION_INFORMATION_EX& pix) {
    stringstream sstr;

    sstr << "Style: " << ToString(pix.PartitionStyle) << endl;
    sstr << "Index: " << pix.PartitionNumber << endl;
    sstr << "Rewrt: " << (pix.RewritePartition ? "True" : "False") << endl;
    sstr << "Offset: " << ToValueString(pix.StartingOffset.QuadPart, UB | US | UG) << endl;
    sstr << "Length: " << ToValueString(pix.PartitionLength.QuadPart, UB | US | UG) << endl;

    switch(pix.PartitionStyle) {
    case PARTITION_STYLE_MBR: sstr << ToString(pix.Mbr) << endl; break;
    case PARTITION_STYLE_GPT: sstr << ToString(pix.Gpt) << endl; break;
    default: break;
    }
    return sstr.str();
}
