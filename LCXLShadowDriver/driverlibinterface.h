#ifndef _DRIVER_LIB_INTERFACE_H_
#define _DRIVER_LIB_INTERFACE_H_

#include "bitmapmgr.h"
#include "driverinterface.h"
//
//  源文件：FilterInterface.h
//  作者：罗澄曦(LCXL)
//  版权：Copyright (c) 2011  LCXL
//  说明：过滤驱动的接口部分，接口的实现部分是在LLShadowMain.c中，因此LLShadowMain.c必须包含此文件

//此导出函数定义在LLShadowMain.c中
extern PDRIVER_OBJECT g_LCXLDriverObject;//导出驱动控制设备
extern RTL_OSVERSIONINFOEXW g_OSVerInfo;//导出系统版本号


//
//  Here is what the major and minor versions should be for the various OS versions:
//
//  OS Name                                 MajorVersion    MinorVersion
//  ---------------------------------------------------------------------
//  Windows 2000                             5                 0
//  Windows XP                               5                 1
//  Windows Server 2003                      5                 2
//

#define IS_WINDOWSXP() \
	(g_OSVerInfo.dwMajorVersion == 5 && g_OSVerInfo.dwMinorVersion == 1)

#define IS_WINDOWSSRV2003_OR_LATER() \
	((g_OSVerInfo.dwMajorVersion == 5 && g_OSVerInfo.dwMinorVersion >= 2) || \
	g_OSVerInfo.dwMajorVersion > 5)

#define IS_WINDOWSVISTA_OR_LATER() \
	(g_OSVerInfo.dwMajorVersion >= 6)

#define IS_WINDOWS2000_OR_OLDER() \
	((g_OSVerInfo.dwMajorVersion == 5 && g_OSVerInfo.dwMinorVersion == 0) || \
	(g_OSVerInfo.dwMajorVersion < 5))

#define DISKPERF_MAXSTR         64


typedef enum _DO_TYPE {
	DO_CONTROL,//控制设备
	DO_FSVOL,//卷过滤设备
	DO_FSCTRL,//文件系统过滤设备
	DO_DISKVOL,//磁盘卷过滤设备
	DO_DISK,//磁盘过滤设备
} DO_TYPE;

//重定向结构
typedef struct _LCXL_TABLE_MAP {
	ULONGLONG	orgIndex;	// 原始簇地址
	ULONGLONG	mapIndex;	// 重定向后的地址
	BOOLEAN		IsVisited;//是否访问过，这个在保存影子系统时有用
	BOOLEAN		IsDiskRef;//是否是磁盘设备重定向
} LCXL_TABLE_MAP, *PLCXL_TABLE_MAP;

//用于磁盘卷驱动，用于标示这个磁盘卷的文件系统是否准备好了
typedef enum _DV_INITIALIZE_STATE {
	DVIS_NONE,//没有初始化过
	DVIS_INITIALIZING,//正在初始化
	DVIS_INITIALIZED,//已经初始化完毕
	DVIS_WORKING,//开始工作
	DVIS_SAVINGDATA,//正在保存影子系统的文件
} DV_INITIALIZE_STATE, *PDV_INITIALIZE_STATE;

//用于磁盘驱动，用于表示这个磁盘卷
typedef enum _DISK_INITIALIZE_STATE {
	DIS_NORMAL,//默认为正常模式
	DIS_MEMORY,//内存重定向
	DIS_NO_VOLUME,//表示此磁盘没有磁盘卷
} DISK_INITIALIZE_STATE, *PDISK_INITIALIZE_STATE;

typedef struct _REDIRECT_RW_MEM {
	LIST_ENTRY ListEntry;
	PVOID buffer;
	ULONGLONG offset;
	ULONG length;
} REDIRECT_RW_MEM, *PREDIRECT_RW_MEM;

typedef struct _APP_REQUEST_LIST_ENTRY {
	LIST_ENTRY ListEntry;
	NTSTATUS status;//应用程序请求结果
	KEVENT FiniEvent;//完成事件
	APP_REQUEST AppRequest;//应用程序请求
} APP_REQUEST_LIST_ENTRY, *PAPP_REQUEST_LIST_ENTRY;

typedef struct _DISK_VOL_FILTER_EXTENT {
	PDEVICE_OBJECT FilterDO;//磁盘上层的磁盘卷过滤设备
	//
	// Specifies the offset and length of this
	// extent relative to the beginning of the
	// disk.
	//
	LARGE_INTEGER StartingOffset;
	LARGE_INTEGER ExtentLength;
} DISK_VOL_FILTER_EXTENT, *PDISK_VOL_FILTER_EXTENT;

//设备扩展区
typedef struct _EP_DEVICE_EXTENSION {
	//设备类型
	DO_TYPE DeviceType;
	union {
		//是过滤设备
		struct _DE_FILTER {
			//下层驱动设备对象
			PDEVICE_OBJECT LowerDeviceObject;
			//要绑定的物理驱动设备对象
			PDEVICE_OBJECT PhysicalDeviceObject;
			//注意：在文件系统中，这里的物理设备名称是磁盘卷名称！并不是真正的物理设备名称，文件系统设备是没有名字的！
			//物理设备名称
			UNICODE_STRING PhysicalDeviceName;
			//物理设备名称缓冲区
			WCHAR PhysicalDeviceNameBuffer[MAX_DEVNAME_LENGTH];
			
			union {
				/*
				//DO_FSCTRL 文件系统结构
				struct _DE_FSCTRL {
				
				} DE_FSCTRL;
				*/
				//DO_FSVOL 文件系统过滤设备扩展
				struct _DE_FSVOL {
					//磁盘卷对象
					PDEVICE_OBJECT DiskVolDeviceObject;
					//磁盘卷名称
					UNICODE_STRING DiskVolDeviceName;
					//磁盘设备名称缓冲区
					WCHAR DiskVolDeviceNameBuffer[MAX_DEVNAME_LENGTH];
					//--------------driverlib.c文件初始化标量到此，下面是driver.c的事情了
					
					//文件卷下层所对应的磁盘卷过滤设备
					PDEVICE_OBJECT DiskVolFilter;
				} DE_FSVOL;
				
				//DO_DISKVOL磁盘卷过滤设备扩展
				struct _DE_DISKVOL {
					//文件系统相关设置是否初始化过，还是正在初始化
					//是否已经初始化过
					//当文件系统成功的挂载到此磁盘卷设备时
					//就代表初始化成功
					//注意：此时如果文件系统被卸载了，并且IsProtect=TRUE，则磁盘卷不允许任何的写操作
					DV_INITIALIZE_STATE InitializeStatus;
					//是否保护此磁盘
					BOOLEAN IsProtect;
					//读写是否成功，如果有错误，则无法保存磁盘
					BOOLEAN IsRWError;
					//是否要保存影子系统下所修改的数据
					BOOLEAN IsSaveShadowData;
					//正在保存影子系统文件的事件（通知事件）
					//  代表读写进程已经保存完成
					KEVENT SaveShadowDataFinishEvent;
					//磁盘卷过滤设备对对应的上层文件卷过滤设备
					//当文件系统被卸载时，此对象被置空(NULL)
					PDEVICE_OBJECT FSVolFilter;

					//在文件系统上获取的数据
					//当文件系统被卸载时，此对象依然存在
					//如果文件系统被卸载后又被挂载，则当IsProtect&IsInitialized=TRUE时，不更新FsSetting域，否则更新
					struct _FS_SETTING {
						ULONG BytesPerSector;//每个扇区包含的字节数
						ULONG SectorsPerAllocationUnit;//分配单元中包含的扇区数
						ULONG BytesPerAllocationUnit;//分配单元中包含的字节数，为BytesPerSector*SectorsPerAllocationUnit
						ULONGLONG TotalAllocationUnits;//总共有多少分配单元
						ULONGLONG AvailableAllocationUnits;//还有多少分配单元，实际还有多少分配空间
						ULONGLONG DataClusterOffset;//数据簇的偏移量
						//这里的位图都是以簇/分配空间为单位，如果以扇区为单位，则500G的硬盘有可能会消耗掉128M左右的内存！
						//而使用簇为单位，则会消耗掉16MB的内存。
						LCXL_BITMAP BitmapUsed;//已经分配的位图
						LCXL_BITMAP BitmapRedirect;//重定向的位图，被重定向过的位图为1
						LCXL_BITMAP BitmapDirect;//直接放过读写的位图，直接放过的位图为1
						//簇的重定向表
						RTL_GENERIC_TABLE RedirectTable;
						// 上次扫描到的空闲簇的位置，默认为0，即从0bit开始扫描，因为FAT文件系统数据区不是从磁盘卷起始位置开始计算，因此需要格外注意
						ULONGLONG LastScanIndex;
					} FsSetting;//一些文件系统的设置
					struct _REDIRECT_MEM{
						//内存重定向列表头
						REDIRECT_RW_MEM RedirectListHead;
					} RedirectMem;//在内存中重定向读写
					
					//线程句柄
					HANDLE ThreadHandle;
					//线程对象
					PETHREAD ThreadObject;
					//线程事件
					//  当有读写操作、线程终止以及一系列需要线程工作的操作时，激活此事件
					KEVENT RequestEvent;
					//是否终止线程
					BOOLEAN ToTerminateThread;
					//这个卷上的保护系统使用的读写请求队列
					LIST_ENTRY RWListHead;
					//应用程序请求队列
					APP_REQUEST_LIST_ENTRY AppRequsetListHead;
					//这个卷上的保护系统使用的请求队列的锁
					KSPIN_LOCK RequestListLock;

					//与磁盘过滤设备关联
					PDEVICE_OBJECT DiskFilterDO[64];//磁盘下层的磁盘过滤设备
					ULONG DiskFilterNum;//磁盘下层的磁盘卷过滤设备数量
					//BOOLEAN IsPartitionVoluem;//是否是基本磁盘的分区卷
				} DE_DISKVOL;
				
				//DO_DISK磁盘过滤设备扩展
				struct _DE_DISK {
					DISK_INITIALIZE_STATE InitializeStatus;
					ULONG DiskNumber;//磁盘序号
					BOOLEAN IsVolumeChanged;//磁盘卷已经改变，通常是受保护的磁盘卷被删除时会将此值置为TRUE，当磁盘卷被更改时，新增的磁盘卷将不被认为是磁盘卷
					STORAGE_BUS_TYPE BusType;//总线类型

					//线程句柄
					HANDLE ThreadHandle;
					//线程对象
					PETHREAD ThreadObject;
					//线程事件
					//  当有读写操作、线程终止以及一系列需要线程工作的操作时，激活此事件
					KEVENT RequestEvent;
					//是否终止线程
					BOOLEAN ToTerminateThread;
					//这个卷上的保护系统使用的读写请求队列
					LIST_ENTRY RWListHead;
					//这个磁盘上的保护系统使用的请求队列的锁
					KSPIN_LOCK RequestListLock;

					//与磁盘卷过滤设备关联
					DISK_VOL_FILTER_EXTENT DiskVolFilterDO[64];//磁盘上层的磁盘卷过滤设备
					ULONG DiskVolFilterNum;//磁盘上层的磁盘卷过滤设备数量

					//内存重定向列表头
					REDIRECT_RW_MEM RedirectListHead;
					DISK_GEOMETRY DiskGeometry;//查询磁盘的扇区数等信息
				} DE_DISK;
				
			};
			
		} DE_FILTER;
		//DO_CONTROL 控制设备扩展
		struct _DE_CONTROL {
			//注册表回调函数Cookie
			LARGE_INTEGER  RegCookie;
		} DE_CONTROL;
	};
} EP_DEVICE_EXTENSION, *PEP_DEVICE_EXTENSION;//设备扩展

//下列函数的定义在driver.c中

//设备共同例程
NTSTATUS LCXLDriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
	);

//驱动卸载例程
VOID LCXLDriverUnload(
	IN PDRIVER_OBJECT DriverObject
	);

//磁盘卷过滤设备
VOID VPostAttachDiskVolDevice(
	IN PDEVICE_OBJECT FilterDeviceObject//文件系统过滤设备 
	);

//磁盘卷读取例程
NTSTATUS VDeviceRead(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);
NTSTATUS VDeviceWrite(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);

NTSTATUS VDevicePnp(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);

NTSTATUS VDevicePower(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);

NTSTATUS VDeviceControl(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);

//磁盘过滤设备

VOID DPostAttachDiskDevice(
	IN PDEVICE_OBJECT FilterDeviceObject//文件系统过滤设备 
	);

NTSTATUS DDeviceRead(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);
NTSTATUS DDeviceWrite(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);

NTSTATUS DDevicePnp(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);

NTSTATUS DDeviceControl(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);

//文件过滤设备
NTSTATUS FDeviceCreate(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);

NTSTATUS FDeviceCleanup(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);
NTSTATUS FDeviceClose(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);

//是否需要挂载这个卷设备
//如果返回False，则不挂载此设备
//注意：有可能会出现已挂载的设备又有请求挂载，需要自行辨别
BOOL FPreAttachVolumeDevice(
	IN PDEVICE_OBJECT VolumeDeviceObject,//卷设备对象
	IN PDEVICE_OBJECT DiskDeviceObject,//磁盘设备对象
	IN PUNICODE_STRING DiskDeviceName///磁盘设备名称
	);
//挂载成功时，调用此函数
VOID FPostAttachVolumeDevice(
	IN PDEVICE_OBJECT FilterDeviceObject//文件系统过滤设备
	);

//卸载卷设备
VOID FPostDetachVolumeDevice(
	IN PDEVICE_OBJECT DeviceObject,//我的设备
	IN PDEVICE_OBJECT VolumeDeviceObject//设备卷
	);

//控制设备
NTSTATUS CDeviceCreate(
	IN PDEVICE_OBJECT pDevObj, 
	IN PIRP pIrp
	);

NTSTATUS CDeviceControl(
	IN PDEVICE_OBJECT pDevObj, 
	IN PIRP pIrp
	);

NTSTATUS CDeviceClose(
	IN PDEVICE_OBJECT pDevObj, 
	IN PIRP pIrp
	);

NTSTATUS CShutdown(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

#endif