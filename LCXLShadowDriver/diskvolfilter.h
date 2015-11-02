#ifndef _DISK_VOL_FILTER_H_
#define _DISK_VOL_FILTER_H_

//////////////////////////////////////////////////////////////////////////
//磁盘卷过滤设备头文件，放着磁盘过滤设备有关的函数
#ifndef _DRIVER_H_
#include "driver.h"
#endif



/*
//IRP列表
typedef struct _IRP_LIST_ENTRY {
	
}IRP_LIST_ENTRY, *PIRP_LIST_ENTRY;
*/
//将内存重定向的数据写入到磁盘中
VOID VWriteMemRedirectDataToDisk(
	IN OUT PEP_DEVICE_EXTENSION voldevext,
	IN OUT PRW_BUFFER_REC RWBuffer,
	IN PVOID ClusterBuffer,
	IN REDIRECT_MODE RedirectMode//重定向模式
	);

//IRP读写处理函数
NTSTATUS VReadWriteIrpDispose(
	IN OUT PEP_DEVICE_EXTENSION voldevext,
	ULONG MajorFunction,
	PVOID buffer,
	ULONG length,
	ULONGLONG offset,
	IN OUT PRW_BUFFER_REC RWBuffer,
	IN REDIRECT_MODE RedirectMode,//重定向模式
	IN PVOID ClusterBuffer//一个簇的内存数据库，当RedirectInMem=FALSE时，ClusterBuffer必须不为空
	);

//保存影子系统的文件到磁盘中
NTSTATUS VSaveShadowData(PEP_DEVICE_EXTENSION voldevext);


//磁盘卷读写线程
VOID VReadWriteThread (
	IN PVOID Context//线程上下文，这里是磁盘卷过滤驱动对象
	);
//处理读写操作
NTSTATUS VHandleReadWrite(
	PEP_DEVICE_EXTENSION VolDevExt,
	IN ULONG MajorFunction,//主功能号
	IN ULONGLONG ByteOffset,//偏移量
	IN OUT PVOID Buffer,//要读取/写入的缓冲区，其数据是簇对齐的
	IN ULONG Length//缓冲区长度
	);

//硬盘没有空间
#define GET_CLUSTER_RESULT_NO_SPACE 0xFFFFFFFFFFFFFFFF
//簇列表超出范围
#define GET_CLUSTER_RESULT_OUT_OF_BOUND 0xFFFFFFFFFFFFFFFE
//************************************
// Method:    VGetRealClusterForRead
// FullName:  VGetRealClusterForRead
// Access:    public 
// Returns:   ULONGLONG 如果是 GET_CLUSTER_RESULT_DENY，表示被保护，无法读取
// Qualifier: 获取要读取写入的真实簇
// Parameter: PEP_DEVICE_EXTENSION VolDevExt
// Parameter: ULONGLONG ClusterIndex
// Parameter: BOOLEAN IsReadOpt 是否是读操作
// Parameter: BOOLEAN CanAccessProtectBitmap 是否可以读取被保护的位图
//************************************
ULONGLONG VGetRealCluster(
	PEP_DEVICE_EXTENSION VolDevExt, 
	ULONGLONG ClusterIndex, 
	BOOLEAN IsReadOpt
	);

//直接读写磁盘卷
NTSTATUS VDirectReadWriteDiskVolume(
	IN PDEVICE_OBJECT DiskVolumeDO,//磁盘卷设备对象 
	IN ULONG MajorFunction,//主功能号
	IN ULONGLONG ByteOffset,//偏移量
	IN OUT PVOID Buffer,//要读取/写入的缓冲区
	IN ULONG Length,//缓冲区长度
	OUT PIO_STATUS_BLOCK iostatus,
	IN BOOLEAN Wait//是否等待
	);


//////////////////////////////////////////////////////////////////////////
//说明：比较例程
//返回值：返回函数执行结果
//参数说明：
//	Table：位图表
//	FirstStruct：第一个簇的结构指针
//	SecondStruct：第二个簇的结构指针
//////////////////////////////////////////////////////////////////////////
RTL_GENERIC_COMPARE_RESULTS NTAPI VCompareRoutine(
	PRTL_GENERIC_TABLE Table,
	PVOID FirstStruct,
	PVOID SecondStruct
	);

//////////////////////////////////////////////////////////////////////////
//说明：申请内存例程（在内核层中的函数一般被称为例程）
//返回值：返回申请的内存指针
//参数说明：
//	Table：内存表；
//	ByteSize：内存申请大小；
//////////////////////////////////////////////////////////////////////////
//LCXL修改，将LONG ByteSize修改为CLONG ByteSize
PVOID NTAPI VAllocateRoutine (
	PRTL_GENERIC_TABLE Table,
	CLONG ByteSize
	);
//释放例程
VOID NTAPI VFreeRoutine (
	PRTL_GENERIC_TABLE Table,
	PVOID Buffer
	);
//创建磁盘卷线程
NTSTATUS VCreateVolumeThread(
	IN PDEVICE_OBJECT DiskVolFilterDO
	);
//关闭磁盘卷线程
NTSTATUS VCloseVolumeThread(
	IN PDEVICE_OBJECT DiskVolFilterDO
	);
//磁盘卷过滤设备卸载挂载事件
VOID VPostDetachDiskVolDevice(
	IN PDEVICE_OBJECT FilterDeviceObject//文件系统过滤设备 
	);

//使磁盘和磁盘卷过滤设备相互关联
NTSTATUS VAssociateDiskFilter(
	IN PDEVICE_OBJECT DiskVolFilterDO, 
	IN LARGE_INTEGER StartingOffset,
	IN LARGE_INTEGER ExtentLength,
	IN PDEVICE_OBJECT DiskFilterDO
	);

//取消此磁盘卷过滤设备与磁盘过滤设备的所有关联
NTSTATUS VDisassociateDiskFilter(
	IN PDEVICE_OBJECT DiskVolFilterDO
	);


//获取磁盘卷上层的文件系统的信息
NTSTATUS VGetFSInformation(
	IN PDEVICE_OBJECT DiskVolDO
	);

NTSTATUS VPostProtectedFSInfo(IN PEP_DEVICE_EXTENSION VolDevExt);

//获取驱动设置的线程
VOID VGetDriverSettingThread(
	IN PVOID Context//线程上下文，这里是文件系统过滤驱动对象
	);

VOID VGetFSInfoThread(
	IN PVOID Context
	);

#endif