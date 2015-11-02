#ifndef _FILE_SYS_FILTER_H_
#define _FILE_SYS_FILTER_H_

#ifndef _DRIVER_H_
#include "driver.h"
#endif

//获取第一数据扇区的偏移量
NTSTATUS FGetFirstDataSectorOffset(
	IN HANDLE FSDeviceHandle, 
	OUT PULONGLONG FirstDataSector
	);

//设置直接读写的文件
NTSTATUS FSetDirectRWFile(
	IN PEP_DEVICE_EXTENSION fsdevext, 
	IN PUNICODE_STRING FilePath,//文件路径，不包括驱动器
	IN ULONGLONG DataClusterOffset,//数据区簇偏移量
	OUT PLCXL_BITMAP BitmapDirect
	);

#endif