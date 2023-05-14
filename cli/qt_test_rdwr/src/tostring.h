#ifndef __to_string_h__
#define __to_string_h__

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

#define UB 0x00000001
#define US 0x00000002
#define UK 0x00000004
#define UM 0x00000008
#define UG 0x00000010
#define UT 0x00000020

extern std::string ToString(PARTITION_STYLE& ps);
extern std::string ToValueString(U64 val, U32 code);
extern std::string ToString(GUID& g);
extern std::string ToPartitionTypeString(GUID& g);
extern std::string ToGptAttributesString(U64& attr);
extern std::string ToPartitionStyleString(U32 s);
extern std::string ToString(PARTITION_INFORMATION_MBR& pi);
extern std::string ToString(PARTITION_INFORMATION_GPT& pi);
extern std::string ToString(PARTITION_INFORMATION& pi);
extern std::string ToString(PARTITION_INFORMATION_EX& pix);
extern std::string ToString(DRIVE_LAYOUT_INFORMATION_EX& d);

#endif
