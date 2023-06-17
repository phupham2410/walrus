#ifndef __SYSHEMHEADER_H__
#define __SYSHEMHEADER_H__

#include <windows.h>
#include <ntdddisk.h>
#include <ntddscsi.h>
#include <ntddstor.h>

#ifdef USE_MINGW
#include "nvme.h"
#include "winioctl.h"
#else
#include <nvme.h>
#include <winioctl.h>
#endif

#endif // __SYSHEMHEADER_H__
