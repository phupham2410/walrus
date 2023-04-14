# walrus
handle storage devices

Code structure:
cli: test programs, create/build using qtcreator or vscode
     sample naming rule: qt_ata_read, qt_disk_clone, qt_fw_upgrade, vs_nvme_scan ....
     sometimes we need to use visual studio because lacking of header files in mingw
gui: sample gui program, demonstrate how to use storageapi
src/cmn: storageapi common code to access device
src/api: storageapi adapter code

src/api/StorageApi.h: StorageApi's utilities
src/api/StorageApiTest.h: Fake implementation (for Gui team)
src/api/StorageApiImpl.h: Real implementation 
src/api/StorageApicmn.h: c-include file, common utilities
src/api/StorageApiSata.h: c-include file, supports SATA devices
src/api/StorageApiNvme.h: c-include file, supports NVMe devices
src/api/StorageApiScsi.h: c-include file, supports SAT devices

