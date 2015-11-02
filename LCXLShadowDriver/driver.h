#ifndef _DRIVER_H_
#define _DRIVER_H_
//////////////////////////////////////////////////////////////////////////
//文件过滤设备，磁盘卷过滤设备，磁盘过滤设备共用的头文件，存放共用的函数
//其源文件还实现了控制设备的功能
#include "driverlibinterface.h"
#include "diskiocode.h"
#include "aesex.h"

#define CONTROL_DEVICE_NAME L"\\FileSystem\\LCXLShadow"//控制设备
#define CONTROL_SYMBOL_LINK L"\\??\\LCXLShadow"//符号链接

#define MEM_TAG_RWBUFFER 'RWBF'//读写线程中的缓冲区TAG
__inline PREDIRECT_RW_MEM AllocateRedirectIrpListEntry(ULONG length, ULONGLONG offset, ULONG Tag) {
	PREDIRECT_RW_MEM NewListEntry;

	NewListEntry = (PREDIRECT_RW_MEM)ExAllocatePoolWithTag(PagedPool, sizeof(REDIRECT_RW_MEM)+length, Tag);
	NewListEntry->buffer = (PBYTE)NewListEntry+sizeof(REDIRECT_RW_MEM);
	NewListEntry->length = length;
	NewListEntry->offset = offset;
	return NewListEntry;
};

__inline VOID FreeRedirectIrpListEntry(PREDIRECT_RW_MEM ListEntry) {
	ExFreePool(ListEntry);
};

typedef enum _REDIRECT_MODE {
	RM_DIRECT,//直接读写
	RM_IN_MEMORY,//内存中读写
	RM_IN_DISK,//磁盘读写
} REDIRECT_MODE, *PREDIRECT_MODE;//读写重定向模式



//如果总线是SCSI，SATA，ATA，RAID，SSA，则认为是本地磁盘
#define DIS_LOCAL_DISK(_BusType) \
	((_BusType) == BusTypeSas||(_BusType) == BusTypeScsi||(_BusType) == BusTypeiScsi||\
	(_BusType) == BusTypeSata||(_BusType) == BusTypeFibre||\
	(_BusType) == BusTypeAta||(_BusType) == BusTypeAtapi||\
	(_BusType) == BusTypeRAID||\
	(_BusType) == BusTypeSsa)


//读写缓冲区为1M
#define DefaultReadWriteBufferSize 0x10000
//读写缓冲区结构
typedef struct _RW_BUFFER_REC {
	PVOID Buffer;
	ULONG BufferSize;
} RW_BUFFER_REC, *PRW_BUFFER_REC;

//初始化
#define RW_INIT_MEM(_RWBuffer) \
	RtlZeroMemory((_RWBuffer), sizeof(RW_BUFFER_REC))

//调整读写缓冲区结构
#define RW_ADJUST_MEM(_RWBuffer, _RWBufferSize)				\
	if ((_RWBuffer)->BufferSize < _RWBufferSize) {			\
	if ((_RWBuffer)->BufferSize > 0) {						\
		ExFreePool((_RWBuffer)->Buffer);					\
	}														\
	(_RWBuffer)->Buffer = NULL;								\
	(_RWBuffer)->BufferSize = _RWBufferSize;				\
	(_RWBuffer)->Buffer = ExAllocatePoolWithTag(PagedPool, (_RWBuffer)->BufferSize, MEM_TAG_RWBUFFER);\
	}

//释放读写缓冲区
#define RW_FREE_MEM(_RWBuffer) \
	if ((_RWBuffer)->Buffer != NULL) {\
		ExFreePoolWithTag((_RWBuffer)->Buffer, MEM_TAG_RWBUFFER);\
	}


//更改驱动图标
NTSTATUS LCXLChangeDriverIcon(
	PDEVICE_OBJECT VolDiskPDO
	);



//获取驱动的设置
//DeviceObject:文件系统过滤对象
NTSTATUS 
	LCXLGetDriverSetting(
	PDEVICE_OBJECT DeviceObject
	);

//保存驱动设置
NTSTATUS LCXLSaveShadowSetting();

//驱动重新初始哈
VOID
	LCXLDriverReinitialize (
	__in struct _DRIVER_OBJECT *DriverObject,
	__in_opt PVOID Context,
	__in ULONG Count
	);
//驱动设置文件
#define LCXL_SETTING_PATH L"\\LLShadowS.sys"

typedef enum _LCXL_DRIVER_SETTING_STATE {DS_NONE, DS_INITIALIZING, DS_INITIALIZED} LCXL_DRIVER_SETTING_STATE;

//#define 
//存储在文件中驱动设置结构
typedef struct _LCXL_FILE_DRIVER_SETTING {
	LCXL_SHADOW_MODE ShadowMode;//磁盘保护模式
	BOOLEAN NeedPass;//是否需要密码
	WCHAR Passmd5[32];//密码的MD5值
	ULONG CustomProtectVolumeNum;//自定义要保护的卷的数量
	WCHAR CustomProtectVolume[26][MAX_DEVNAME_LENGTH];//自定义要保护的卷的设备名称，自定义设置中最大能保护26个磁盘卷
} LCXL_FILE_DRIVER_SETTING, *PLCXL_FILE_DRIVER_SETTING;

typedef struct _LCXL_DISK_INFO {
	IDINFO DiskIDInfo;//磁盘ID信息
	PDRIVER_OBJECT DiskDriverObject;//磁盘驱动对象
	//PDRIVER_DISPATCH MFRead;//原始读函数，用来检测DISK驱动是否被更改
	PDRIVER_DISPATCH OrgMFWrite;//原始写函数，用来检测DISK驱动是否被更改
} LCXL_DISK_INFO, *PLCXL_DISK_INFO;//磁盘信息

//驱动设置结构
typedef struct _LCXL_DRIVER_SETTING {
	LCXL_DRIVER_SETTING_STATE InitState;//是否初始化过
	KEVENT SettingInitEvent;//初始化成功的事件
	ULONG DriverErrorType;//驱动在运行过程中是否有错误
	LCXL_SHADOW_MODE CurShadowMode;//当前磁盘保护模式
	PDEVICE_OBJECT SettingDO;//驱动设置文件所在的驱动对象
	PRETRIEVAL_POINTERS_BUFFER FileClusters;//配置文件的簇列表
	ULONGLONG DataClusterOffset;//数据簇列表偏移量，用在FAT文件系统中，NTFS文件系统此值为0
	ULONG BytesPerAllocationUnit;//配置文件所在磁盘卷的簇大小
	
	BOOLEAN IsFirstGetDiskInfo;//是否已经获取磁盘信息
	LCXL_DISK_INFO DiskInfo;//磁盘信息
	//下面是要写入到文件的数据
	LCXL_FILE_DRIVER_SETTING SettingFile;
} LCXL_DRIVER_SETTING, *PLCXL_DRIVER_SETTING;

#define MEM_SETTING_TAG 'DSET'
//配置文件的大小，默认为4K
#define SETTING_FILE_SIZE 0x1000
//************************************
// Method:    LCXLWriteSettingToBuffer
// FullName:  LCXLWriteSettingToBuffer
// Access:    public 
// Returns:   NTSTATUS
// Qualifier: 将配置写入缓冲区中
// Parameter: IN PLCXL_DRIVER_SETTING DriverSetting配置数据结构
// Parameter: OUT PVOID * Buffer数据缓冲区，当缓冲区使用完毕时，使用LCXLFreeSettingBuffer释放掉
// Parameter: OUT PULONG BufferSize数据缓冲区大小
//************************************
NTSTATUS LCXLWriteSettingToBuffer(
	IN PLCXL_DRIVER_SETTING DriverSetting, 
	OUT PVOID *Buffer,
	OUT PULONG BufferSize
);

#define LCXLFreeSettingBuffer(_Buffer) ExFreePoolWithTag(_Buffer, MEM_SETTING_TAG)

//************************************
// Method:    LCXLReadSettingFromBuffer
// FullName:  LCXLReadSettingFromBuffer
// Access:    public 
// Returns:   NTSTATUS
// Qualifier: 从缓冲区中读取配置
// Parameter: IN PVOID Buffer
// Parameter: IN ULONG BufferSize
// Parameter: IN OUT PLCXL_DRIVER_SETTING DriverSetting
//************************************
NTSTATUS LCXLReadSettingFromBuffer(
	IN PVOID Buffer,
	IN ULONG BufferSize,
	IN OUT PLCXL_DRIVER_SETTING DriverSetting
	);

//************************************
// Method:    LCXLVolumeNeedProtect
// FullName:  LCXLVolumeNeedProtect
// Access:    public 
// Returns:   BOOLEAN
// Qualifier: 磁盘卷是否需要保护
// Parameter: PDEVICE_OBJECT FSFilterDO 文件系统过滤对象
//************************************
BOOLEAN LCXLVolumeNeedProtect(PDEVICE_OBJECT FSFilterDO);

//插入到重定向读写列表中
void LCXLInsertToRedirectRWMemList(IN PREDIRECT_RW_MEM RedirectListHead, IN PVOID buffer, IN ULONGLONG offset, IN ULONG length);
//从重定向读写列表中读取
void LCXLReadFromRedirectRWMemList(IN PREDIRECT_RW_MEM RedirectListHead, IN PVOID buffer, IN ULONGLONG offset, IN ULONG length);


//应用层序进行时设置结构
typedef struct _LCXL_APP_RUNTIME_SETTING {
	HANDLE PID;//进程PID
} LCXL_APP_RUNTIME_SETTING, *PLCXL_APP_RUNTIME_SETTING;

//VCN_EXTENT中的结构体
typedef struct _LCXL_VCN_EXTENT {
	LARGE_INTEGER NextVcn;
	LARGE_INTEGER Lcn;
} LCXL_VCN_EXTENT, *PLCXL_VCN_EXTENT;

//通过符号链接获取设备名称
NTSTATUS CGetDeviceNamebySymbolicLink(
	IN OUT PUNICODE_STRING DeviceName
	);

//通过DOS名称获取磁盘卷过滤设备
NTSTATUS CGetDiskVolFilterDOByDosName(
	IN LPWSTR DosName, 
	OUT PDEVICE_OBJECT *DiskVolFilterDO
	);
//通过Volume名称获取磁盘卷过滤设备
NTSTATUS CGetDiskVolFilterDOByVolumeName(
	IN PUNICODE_STRING VolumeName, 
	OUT PDEVICE_OBJECT *DiskVolFilterDO
	);

//FileObject有关的结构
//用于验证一个程序是否是可信的
typedef struct _LCXL_FILE_CONTEXT {
	BYTE aeskey[16];//128bit的AES key
	BOOLEAN IsCredible;//是否是可信的
	BOOLEAN PasswordRight;//密码是否正确
} LCXL_FILE_CONTEXT, *PLCXL_FILE_CONTEXT;

//导出g_DriverSetting变量
extern LCXL_DRIVER_SETTING g_DriverSetting;
//导出g_CDO
extern PDEVICE_OBJECT g_CDO;
//系统初始化完成事件
extern KEVENT	g_SysInitEvent;
//用于簇重定向的后背列表
extern PAGED_LOOKASIDE_LIST g_TableMapList;

#endif