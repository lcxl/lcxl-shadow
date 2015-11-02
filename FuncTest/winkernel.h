//////////////////////////////////////////////////////////////////////////
//头文件：winkernel.h
//说明：内核头文件
//作者：Explorer Team(LCXL)
//////////////////////////////////////////////////////////////////////////
#pragma once
#include <ntifs.h>
#include <ntimage.h>
#include <ntdddisk.h>
#include <windef.h>
#include <wmilib.h>
#include <mountdev.h>
#include <wmistr.h>
#include <wmidata.h>
#include <wmiguid.h>
#include <ntddvol.h>
#include <Ntstrsafe.h>
//本地
#include "bootvid.h"
//
//  将这些警告视为错误
//
#pragma warning(error:4100)   // Unreferenced formal parameter
#pragma warning(error:4101)   // Unreferenced local variable

typedef PVOID *PPVOID;

#define SEC_IMAGE    0x01000000

typedef struct _SECTION_IMAGE_INFORMATION {
	PVOID EntryPoint;
	ULONG StackZeroBits;
	ULONG StackReserved;
	ULONG StackCommit;
	ULONG ImageSubsystem;
	WORD SubsystemVersionLow;
	WORD SubsystemVersionHigh;
	ULONG Unknown1;
	ULONG ImageCharacteristics;
	ULONG ImageMachineType;
	ULONG Unknown2[3];
} SECTION_IMAGE_INFORMATION, *PSECTION_IMAGE_INFORMATION;

NTSTATUS CreateDllSection(IN IN PUNICODE_STRING pDllName, OUT PHANDLE pSection, OUT PPVOID pBaseAddress);//加载DLL到内存
PVOID GetSectionDllFuncAddr(IN PCHAR lpFunctionName, IN PVOID BaseAddress) ;//获取函数地址

typedef enum _SYSTEM_INFORMATION_CLASS 
{
	SystemBasicInformation,
	SystemProcessorInformation,
	SystemPerformanceInformation,
	SystemTimeOfDayInformation, 
	SystemNotImplemented1,
	SystemProcessesAndThreadsInformation,
	SystemCallCounts, 
	SystemConfigurationInformation, 
	SystemProcessorTimes, 
	SystemGlobalFlag, 
	SystemNotImplemented2, 
	SystemModuleInformation, 
	SystemLockInformation,
	SystemNotImplemented3, 
	SystemNotImplemented4, 
	SystemNotImplemented5, 
	SystemHandleInformation, 
	SystemObjectInformation, 
	SystemPagefileInformation, 
	SystemInstructionEmulationCounts, 
	SystemInvalidInfoClass1, 
	SystemCacheInformation, 
	SystemPoolTagInformation, 
	SystemProcessorStatistics,
	SystemDpcInformation, 
	SystemNotImplemented6,
	SystemLoadImage, 
	SystemUnloadImage, 
	SystemTimeAdjustment, 
	SystemNotImplemented7, 
	SystemNotImplemented8, 
	SystemNotImplemented9,
	SystemCrashDumpInformation, 
	SystemExceptionInformation, 
	SystemCrashDumpStateInformation, 
	SystemKernelDebuggerInformation, 
	SystemContextSwitchInformation, 
	SystemRegistryQuotaInformation, 
	SystemLoadAndCallImage,
	SystemPrioritySeparation, 
	SystemNotImplemented10,
	SystemNotImplemented11, 
	SystemInvalidInfoClass2, 
	SystemInvalidInfoClass3, 
	SystemTimeZoneInformation, 
	SystemLookasideInformation, 
	SystemSetTimeSlipEvent,
	SystemCreateSession,
	SystemDeleteSession, 
	SystemInvalidInfoClass4, 
	SystemRangeStartInformation, 
	SystemVerifierInformation, 
	SystemAddVerifier, 
	SystemSessionProcessesInformation 
} SYSTEM_INFORMATION_CLASS;

PVOID GetInfoTable(IN SYSTEM_INFORMATION_CLASS ATableType);//通过ZwQuerySystemInformation获取对应的信息

typedef struct _SYSTEM_LOAD_AND_CALL_IMAGE {
	UNICODE_STRING  ModuleName;
} SYSTEM_LOAD_AND_CALL_IMAGE, *PSYSTEM_LOAD_AND_CALL_IMAGE;//驱动加载结构

typedef struct _SYSTEM_THREAD 
{
	LARGE_INTEGER           KernelTime;
	LARGE_INTEGER           UserTime;
	LARGE_INTEGER           CreateTime;
	ULONG                   WaitTime;
	PVOID                   StartAddress;
	CLIENT_ID               ClientId;
	KPRIORITY               Priority;
	LONG                    BasePriority;
	ULONG                   ContextSwitchCount;
	ULONG                   State;
	KWAIT_REASON            WaitReason;
} SYSTEM_THREAD, *PSYSTEM_THREAD;

typedef struct _SYSTEM_PROCESS_INFORMATION 
{
	ULONG                   NextEntryOffset;
	ULONG                   NumberOfThreads;
	LARGE_INTEGER           Reserved[3];
	LARGE_INTEGER           CreateTime;
	LARGE_INTEGER           UserTime;
	LARGE_INTEGER           KernelTime;
	UNICODE_STRING          ImageName;
	KPRIORITY               BasePriority;
	HANDLE                  ProcessId;
	HANDLE                  InheritedFromProcessId;
	ULONG                   HandleCount;
	ULONG                   Reserved2[2];
	ULONG                   PrivatePageCount;
	VM_COUNTERS             VirtualMemoryCounters;
	IO_COUNTERS             IoCounters;
	SYSTEM_THREAD           Threads[0];
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

NTSYSAPI NTSTATUS NTAPI ZwQuerySystemInformation(IN SYSTEM_INFORMATION_CLASS SystemInformationClass, IN OUT PVOID SystemInformation, IN ULONG SystemInformationLength, OUT PULONG ReturnLength OPTIONAL);
NTSYSAPI NTSTATUS NTAPI ZwSetSystemInformation(IN SYSTEM_INFORMATION_CLASS  SystemInformationClass, IN OUT PVOID SystemInformation, IN ULONG SystemInformationLength);
typedef struct _PEB_LDR_DATA {
	BYTE Reserved1[8];
	PVOID Reserved2[3];
	LIST_ENTRY InMemoryOrderModuleList;
} PEB_LDR_DATA,  *PPEB_LDR_DATA;

typedef struct _RTL_USER_PROCESS_PARAMETERS {
	BYTE Reserved1[16];
	PVOID Reserved2[10];
	UNICODE_STRING ImagePathName;
	UNICODE_STRING CommandLine;
} RTL_USER_PROCESS_PARAMETERS,  *PRTL_USER_PROCESS_PARAMETERS;

typedef VOID(*PPS_POST_PROCESS_INIT_ROUTINE) (VOID);

typedef struct _PEB {
	BYTE Reserved1[2];
	BYTE BeingDebugged;
	BYTE Reserved2[1];
	PVOID Reserved3[2];
	PPEB_LDR_DATA Ldr;
	PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
	BYTE Reserved4[104];
	PVOID Reserved5[52];
	PPS_POST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;
	BYTE Reserved6[128];
	PVOID Reserved7[1];
	ULONG SessionId;
} PEB, *PPEB;

NTSYSAPI NTSTATUS NTAPI ZwQueryInformationProcess(
	__in HANDLE ProcessHandle,
	__in PROCESSINFOCLASS ProcessInformationClass,
	__out_bcount_opt(ProcessInformationLength) PVOID ProcessInformation,
	__in ULONG ProcessInformationLength,
	__out_opt PULONG ReturnLength
	);

NTSYSAPI NTSTATUS NTAPI ZwAssignProcessToJobObject (
							__in HANDLE JobHandle,
							__in HANDLE ProcessHandle
							);

NTSYSAPI NTSTATUS NTAPI ZwInitiatePowerAction(
	IN POWER_ACTION  SystemAction,
	IN SYSTEM_POWER_STATE  MinSystemState,
	IN ULONG  Flags,
	IN BOOLEAN  Asynchronous);

typedef enum _SYSDBG_COMMAND {
	SysDbgQueryModuleInformation,
	SysDbgQueryTraceInformation,
	SysDbgSetTracepoint,
	SysDbgSetSpecialCall,
	SysDbgClearSpecialCalls,
	SysDbgQuerySpecialCalls,
	SysDbgBreakPoint,
	SysDbgQueryVersion,
	SysDbgReadVirtual,
	SysDbgWriteVirtual,
	SysDbgReadPhysical,
	SysDbgWritePhysical,
	SysDbgReadControlSpace,
	SysDbgWriteControlSpace,
	SysDbgReadIoSpace,
	SysDbgWriteIoSpace,
	SysDbgReadMsr,
	SysDbgWriteMsr,
	SysDbgReadBusData,
	SysDbgWriteBusData,
	SysDbgCheckLowMemory,
	SysDbgEnableKernelDebugger,
	SysDbgDisableKernelDebugger,
	SysDbgGetAutoKdEnable,
	SysDbgSetAutoKdEnable,
	SysDbgGetPrintBufferSize,
	SysDbgSetPrintBufferSize,
	SysDbgGetKdUmExceptionEnable,
	SysDbgSetKdUmExceptionEnable,
	SysDbgGetTriageDump,
	SysDbgGetKdBlockEnable,
	SysDbgSetKdBlockEnable,
} SYSDBG_COMMAND, *PSYSDBG_COMMAND;//ZwSystemDebugControl

typedef struct _SYSTEM_HANDLE_INFORMATION {
	ULONG ProcessId;        //进程标识符
	UCHAR ObjectTypeNumber; //打开的对象的类型
	UCHAR Flags;   //句柄属性标志0x01 = PROTECT_FROM_CLOSE, 0x02 = INHERIT
	USHORT Handle; //句柄数值,在进程打开的句柄中唯一标识某个句柄         
	PVOID Object;  //这个就是句柄对应的EPROCESS的地址
	ACCESS_MASK GrantedAccess;  //句柄对象的访问权限
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;

typedef struct _SYSTEM_HANDLE_INFORMATION_EX {
	ULONG NumberOfHandles;  //句柄数目
	SYSTEM_HANDLE_INFORMATION Information[1];
} SYSTEM_HANDLE_INFORMATION_EX, *PSYSTEM_HANDLE_INFORMATION_EX;

/*
typedef enum _OBJECT_INFORMATION_CLASS {
	ObjectBasicInformation = 0,
	ObjectNameInformation = 1,
	ObjectTypeInformation = 2,
} OBJECT_INFORMATION_CLASS;
*/
#define ObjectNameInformation 1

NTSYSAPI NTSTATUS NTAPI ZwSetSystemTime (
	__in_opt PLARGE_INTEGER SystemTime,
	__out_opt PLARGE_INTEGER PreviousTime
	);

 __inline NTSTATUS EpCompleteRequest(IN NTSTATUS status, IN PIRP Irp, IN ULONG_PTR Information)
{
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = Information;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
};//完成IRP请求

__inline NTSTATUS EpSendToNextDriver(IN PDEVICE_OBJECT TgtDevObj, IN PIRP Irp)
{
	//跳过当前的irp stack
	IoSkipCurrentIrpStackLocation(Irp);
	//调用目标设备来处理这个irp
	return IoCallDriver(TgtDevObj, Irp);
};//调用下级设备驱动

//等待IRP在下层设备的完成
//返回的是IRP的status值
NTSTATUS WaitIRPCompletion(IN PDEVICE_OBJECT TargetDeviceObject, IN OUT PIRP Irp);

//用户共享数据
#define KI_USER_SHARED_DATA         0xffdf0000
#define SharedUserData  ((KUSER_SHARED_DATA * const) KI_USER_SHARED_DATA)

typedef struct _AUX_ACCESS_DATA {
	PPRIVILEGE_SET PrivilegesUsed;
	GENERIC_MAPPING GenericMapping;
	ACCESS_MASK AccessesToAudit;
	ACCESS_MASK MaximumAuditMask;
	//win7蓝屏
	ULONG Unknown[41];
} AUX_ACCESS_DATA, *PAUX_ACCESS_DATA;

NTKERNELAPI
NTSTATUS
NTAPI
	SeCreateAccessState(
	IN PACCESS_STATE AccessState,
	IN PAUX_ACCESS_DATA AuxData,
	IN ACCESS_MASK DesiredAccess,
	IN PGENERIC_MAPPING GenericMapping OPTIONAL
	);

NTKERNELAPI
NTSTATUS
NTAPI
	ObCreateObject (
	IN KPROCESSOR_MODE ProbeMode,
	IN POBJECT_TYPE ObjectType,
	IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
	IN KPROCESSOR_MODE OwnershipMode,
	IN OUT PVOID ParseContext OPTIONAL,
	IN ULONG ObjectBodySize,
	IN ULONG PagedPoolCharge,
	IN ULONG NonPagedPoolCharge,
	OUT PVOID *Object
	);

NTKERNELAPI
NTSTATUS 
	NTAPI 
	ExRaiseHardError( 
	IN NTSTATUS ErrorStatus, 
	IN ULONG NumberOfParameters, 
	IN ULONG UnicodeStringParameterMask, 
	IN PVOID Parameters, 
	IN ULONG ResponseOption, 
	OUT PULONG Response );

NTSTATUS
	IrpCreateFile(
	IN PUNICODE_STRING FileName,
	IN ACCESS_MASK DesiredAccess,
	__out PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG FileAttributes,
	IN ULONG ShareAccess,
	IN ULONG CreateDisposition,
	IN ULONG CreateOptions,
	IN PDEVICE_OBJECT DeviceObject,
	IN PDEVICE_OBJECT RealDevice,
	OUT PFILE_OBJECT *Object
	);

NTSTATUS
	IrpCloseFile(
	IN PDEVICE_OBJECT DeviceObject,
	IN PFILE_OBJECT FileObject
	);

NTSTATUS
	IrpReadFile(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER ByteOffset OPTIONAL,
	IN ULONG Length,
	OUT PVOID Buffer,
	OUT PIO_STATUS_BLOCK IoStatusBlock
	);

NTSTATUS
	IrpWriteFile(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER ByteOffset OPTIONAL,
	IN ULONG Length,
	IN PVOID Buffer,
	OUT PIO_STATUS_BLOCK IoStatusBlock
	);

NTSTATUS
	IrpQueryFile(
	IN PFILE_OBJECT FileObject,
	OUT PVOID FileInformation,
	IN ULONG Length,
	IN FILE_INFORMATION_CLASS FileInformationClass
	);


//************************************
// Method:    GetFileClusterList
// FullName:  GetFileClusterList
// Access:    public 
// Returns:   NTSTATUS
// Qualifier: 获取文件的簇列表
// Parameter: IN HANDLE hFile
// Parameter: OUT PRETRIEVAL_POINTERS_BUFFER * FileClusters 返回簇列表，使用完以后用ExFreePool释放掉
//************************************
NTSTATUS GetFileClusterList(
	IN HANDLE hFile, 
	OUT PRETRIEVAL_POINTERS_BUFFER *FileClusters);

//获取csrss.exe进程
NTSTATUS GetCsrssPid(HANDLE *CsrssPid);
