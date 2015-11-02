#ifndef _DRIVER_LIB_H_
#define _DRIVER_LIB_H_
//////////////////////////////////////////////////////////////////////////
//源文件：LCXLShadowLib.h
//作者：罗澄曦
//说明：本代码集成3类驱动，3类驱动的请求将根据类别分别分配给磁盘过滤驱动，
//文件过滤驱动，和访问设备

#define DELAY_ONE_MICROSECOND   (-10)
#define DELAY_ONE_MILLISECOND   (DELAY_ONE_MICROSECOND*1000)
#define DELAY_ONE_SECOND        (DELAY_ONE_MILLISECOND*1000)

#ifndef Add2Ptr
#define Add2Ptr(P,I) ((PVOID)((PUCHAR)(P) + (I)))
#endif

//是否是我的设备
#define IS_MY_DEVICE(_devObj)\
	(((_devObj)->DriverObject == g_LCXLDriverObject) && \
	((_devObj)->DeviceExtension != NULL))

//判断设备类型
#define GET_MY_DEVICE_TYPE(_devObj) (((PEP_DEVICE_EXTENSION)(_devObj)->DeviceExtension)->DeviceType)

//
//  Macro to test if this is my device object
//
//
//  判断设备类型，是否是这种设备
//

#define IS_THIS_DEVICE_TYPE(_devObj, _devType) \
	(((_devObj)->DriverObject == g_LCXLDriverObject) && \
	((_devObj)->DeviceExtension != NULL) && \
	(((PEP_DEVICE_EXTENSION)(_devObj)->DeviceExtension)->DeviceType == _devType))

//
//  是否是要过滤的设备
//

#define IS_DESIRED_DEVICE_TYPE(_type) \
	(((_type) == FILE_DEVICE_DISK_FILE_SYSTEM) || \
	((_type) == FILE_DEVICE_CD_ROM_FILE_SYSTEM) || \
	((_type) == FILE_DEVICE_NETWORK_FILE_SYSTEM))

//
//  Macro to test if FAST_IO_DISPATCH handling routine is valid
//

#define VALID_FAST_IO_DISPATCH_HANDLER(_FastIoDispatchPtr, _FieldName) \
	(((_FastIoDispatchPtr) != NULL) && \
	(((_FastIoDispatchPtr)->SizeOfFastIoDispatch) >= \
	(FIELD_OFFSET(FAST_IO_DISPATCH, _FieldName) + sizeof(void *))) && \
	((_FastIoDispatchPtr)->_FieldName != NULL))


//驱动入口
NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
	);

//驱动卸载例程
VOID DriverUnload(
	IN PDRIVER_OBJECT DriverObject
	);


NTSTATUS LCXLPassThrough(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

NTSTATUS LCXLCreate(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

NTSTATUS LCXLRead(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

NTSTATUS LCXLWrite(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

NTSTATUS LCXLDeviceControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

NTSTATUS LCXLFileSystemControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

NTSTATUS LCXLCleanup(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

NTSTATUS LCXLClose(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

NTSTATUS LCXLPower(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

NTSTATUS LCXLPnp(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

NTSTATUS LCXLShutdown(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

//文件过滤驱动

//当文件系统设备发生变化时，则调用此函数
VOID FDriverFSNotification (
	__in struct _DEVICE_OBJECT *DeviceObject,
	__in BOOLEAN FsActive
	);

//文件系统，加载卷
NTSTATUS FSMountVolume (
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);
//IRP_MN_LOAD_FILE_SYSTEM
NTSTATUS FSLoadFileSystem(
	PDEVICE_OBJECT DeviceObject, 
	PIRP Irp
	);

//判断是不是ShadowCopyVolume卷
NTSTATUS FIsShadowCopyVolume (
	IN PDEVICE_OBJECT StorageStackDeviceObject,
	OUT PBOOLEAN IsShadowCopy
	);
//判断此设备是否已经被我们的设备挂载了
BOOL FIsAttachedDevice (
	IN PDEVICE_OBJECT VolumeObject
	);

/*
 *尝试挂载卷设备
 *注意：如果此卷设备已经被我们的设备所挂载，那么返回的是TRUE
 */
BOOL FTtyAttachVolumeDevice (
	IN PDEVICE_OBJECT VolumeDeviceObject,//卷设备
	IN PDEVICE_OBJECT DiskDeviceObject//磁盘设备
	);

//挂载文件系统设备对象
NTSTATUS FAttachToVolumeDevice (
	IN PDEVICE_OBJECT VolumeDeviceObject,
	IN PDEVICE_OBJECT DiskDeviceObject,
	IN PUNICODE_STRING DiskDeviceName,
	OUT PDEVICE_OBJECT *FilterDeviceObject//过滤的磁盘设备
	);

//挂载文件系统设备对象
NTSTATUS FAttachToFSDevice (
	IN PDEVICE_OBJECT FSDeviceObject,
	IN BOOL IsNoAttachFSRecognizer//是否挂载文件系统识别器
	);

//枚举文件系统中已经加载的卷
NTSTATUS FEnumerateFileSystemVolumes(
	IN PDEVICE_OBJECT FSDeviceObject
	);

VOID FGetBaseDeviceObjectName (
	IN PDEVICE_OBJECT DeviceObject,
	IN OUT PUNICODE_STRING Name
	);
//Fast IO系列函数()

BOOLEAN SfFastIoCheckIfPossible(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN ULONG Length,
	IN BOOLEAN Wait,
	IN ULONG LockKey,
	IN BOOLEAN CheckForReadOperation,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	);

BOOLEAN SfFastIoRead(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN ULONG Length,
	IN BOOLEAN Wait,
	IN ULONG LockKey,
	OUT PVOID Buffer,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	);

BOOLEAN SfFastIoWrite(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN ULONG Length,
	IN BOOLEAN Wait,
	IN ULONG LockKey,
	IN PVOID Buffer,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	);

BOOLEAN SfFastIoQueryBasicInfo(
	IN PFILE_OBJECT FileObject,
	IN BOOLEAN Wait,
	OUT PFILE_BASIC_INFORMATION Buffer,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	);

BOOLEAN SfFastIoQueryStandardInfo(
	IN PFILE_OBJECT FileObject,
	IN BOOLEAN Wait,
	OUT PFILE_STANDARD_INFORMATION Buffer,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	);

BOOLEAN SfFastIoLock(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN PLARGE_INTEGER Length,
	PEPROCESS ProcessId,
	ULONG Key,
	BOOLEAN FailImmediately,
	BOOLEAN ExclusiveLock,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	);

BOOLEAN SfFastIoUnlockSingle(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN PLARGE_INTEGER Length,
	PEPROCESS ProcessId,
	ULONG Key,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	);

BOOLEAN SfFastIoUnlockAll(
	IN PFILE_OBJECT FileObject,
	PEPROCESS ProcessId,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	);

BOOLEAN SfFastIoUnlockAllByKey(
	IN PFILE_OBJECT FileObject,
	PVOID ProcessId,
	ULONG Key,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	);

BOOLEAN SfFastIoDeviceControl(
	IN PFILE_OBJECT FileObject,
	IN BOOLEAN Wait,
	IN PVOID InputBuffer OPTIONAL,
	IN ULONG InputBufferLength,
	OUT PVOID OutputBuffer OPTIONAL,
	IN ULONG OutputBufferLength,
	IN ULONG IoControlCode,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	);

VOID SfFastIoDetachDevice(
	IN PDEVICE_OBJECT SourceDevice,
	IN PDEVICE_OBJECT TargetDevice
	);

BOOLEAN SfFastIoQueryNetworkOpenInfo(
	IN PFILE_OBJECT FileObject,
	IN BOOLEAN Wait,
	OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	);

BOOLEAN SfFastIoMdlRead(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN ULONG Length,
	IN ULONG LockKey,
	OUT PMDL *MdlChain,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	);


BOOLEAN SfFastIoMdlReadComplete(
	IN PFILE_OBJECT FileObject,
	IN PMDL MdlChain,
	IN PDEVICE_OBJECT DeviceObject
	);

BOOLEAN SfFastIoPrepareMdlWrite(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN ULONG Length,
	IN ULONG LockKey,
	OUT PMDL *MdlChain,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	);

BOOLEAN SfFastIoMdlWriteComplete(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN PMDL MdlChain,
	IN PDEVICE_OBJECT DeviceObject
	);

BOOLEAN SfFastIoReadCompressed(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN ULONG Length,
	IN ULONG LockKey,
	OUT PVOID Buffer,
	OUT PMDL *MdlChain,
	OUT PIO_STATUS_BLOCK IoStatus,
	OUT struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
	IN ULONG CompressedDataInfoLength,
	IN PDEVICE_OBJECT DeviceObject
	);

BOOLEAN SfFastIoWriteCompressed(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN ULONG Length,
	IN ULONG LockKey,
	IN PVOID Buffer,
	OUT PMDL *MdlChain,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
	IN ULONG CompressedDataInfoLength,
	IN PDEVICE_OBJECT DeviceObject
	);

BOOLEAN SfFastIoMdlReadCompleteCompressed(
	IN PFILE_OBJECT FileObject,
	IN PMDL MdlChain,
	IN PDEVICE_OBJECT DeviceObject
	);

BOOLEAN SfFastIoMdlWriteCompleteCompressed(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN PMDL MdlChain,
	IN PDEVICE_OBJECT DeviceObject
	);

BOOLEAN SfFastIoQueryOpen(
	IN PIRP Irp,
	OUT PFILE_NETWORK_OPEN_INFORMATION NetworkInformation,
	IN PDEVICE_OBJECT DeviceObject
	);

//
NTSTATUS
	SfPreFsFilterPassThrough (
	IN PFS_FILTER_CALLBACK_DATA Data,
	OUT PVOID *CompletionContext
	);

VOID
	SfPostFsFilterPassThrough (
	IN PFS_FILTER_CALLBACK_DATA Data,
	IN NTSTATUS OperationStatus,
	IN PVOID CompletionContext
	);

//磁盘卷过滤函数
NTSTATUS
	LCXLAddDevice (
	__in struct _DRIVER_OBJECT *DriverObject,
	__in struct _DEVICE_OBJECT *PhysicalDeviceObject
	);

//共同函数

//获取对象名称
NTSTATUS GetObjectName (
	IN PVOID Object,
	IN OUT PUNICODE_STRING Name
	);

#endif