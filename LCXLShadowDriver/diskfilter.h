#ifndef _DISK_FILTER_H_
#define _DISK_FILTER_H_
//////////////////////////////////////////////////////////////////////////
//磁盘过滤设备头文件，放着磁盘过滤设备有关的函数
#ifndef _DRIVER_H_
#include "driver.h"
#endif

//创建磁盘线程
NTSTATUS DCreateDiskThread(
	IN PDEVICE_OBJECT DiskFilterDO
	);
//关闭磁盘线程
NTSTATUS DCloseDiskThread(
	IN PDEVICE_OBJECT DiskFilterDO
	);

VOID DReadWriteThread(
	IN PVOID Context//线程上下文，这里是磁盘卷过滤驱动对象
	);

NTSTATUS DHookDeviceWrite(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);

#endif