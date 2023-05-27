
#ifndef FsUtil_H
#define FsUtil_H

#include "StorageApi.h"
#include "CoreData.h"

#include "StdHeader.h"

#include <guiddef.h>

namespace FsUtil { // FileSystem Util
    // Called from StorageApi
    StorageApi::eRetCode HandleUpdateVolumeInfo(StorageApi::tDriveArray& da);

    StorageApi::eRetCode TestUpdateVolumeInfo();
}

#endif
