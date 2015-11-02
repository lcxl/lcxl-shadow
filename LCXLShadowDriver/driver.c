#include "winkernel.h"
#include "driver.h"
#include "diskvolfilter.h"
#include "filesysfilter.h"
PDEVICE_OBJECT g_CDO=NULL;//控制设备
//驱动设置
LCXL_DRIVER_SETTING g_DriverSetting;
//程序运行时设置
LCXL_APP_RUNTIME_SETTING g_AppRuntimeSetting;

//用于簇重定向的后背列表
PAGED_LOOKASIDE_LIST g_TableMapList;
//系统初始化完成事件
KEVENT	g_SysInitEvent;

void LCXLShowDriverLogoText()
{
	PAGED_CODE();
	if (InbvIsBootDriverInstalled()) {
		InbvAcquireDisplayOwnership();
		//InbvResetDisplay();
		//InbvSolidColorFill(0,0,639,479,4); 
		InbvSetTextColor(15);
		InbvInstallDisplayStringFilter((INBV_DISPLAY_STRING_FILTER)NULL);
		InbvEnableDisplayString(TRUE);   
		InbvSetScrollRegion(200,0,639,479); 
		InbvDisplayString("LCXL Shadow is protecting your system\n");
		InbvDisplayString("                              by LCXL\n");
	}
	
}

//设备共同例程
NTSTATUS LCXLDriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
	)
{
	UNICODE_STRING devName;//设备名称
	UNICODE_STRING lnkName;//符号链接
	PEP_DEVICE_EXTENSION pDevExt;//设备扩展
	NTSTATUS status;

	PAGED_CODE();
	UNREFERENCED_PARAMETER( RegistryPath );
	//LCXLShowDriverLogoText();
	
	KdPrint(("SYS:Windows Version(%d.%d Build:%d) ServicePackMajor:%d\n", g_OSVerInfo.dwMajorVersion, g_OSVerInfo.dwMinorVersion, g_OSVerInfo.dwBuildNumber, g_OSVerInfo.wServicePackMajor));
	if (IS_WINDOWS2000_OR_OLDER()) {
		KdPrint(("SYS:Windows OS is too old.\n"));
		status = STATUS_NOT_SUPPORTED;
	}
	//创建控制设备
	RtlInitUnicodeString(&devName, CONTROL_DEVICE_NAME);
	status = IoCreateDevice(
		DriverObject, 
		sizeof(EP_DEVICE_EXTENSION), 
		&devName, 
		FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_SECURE_OPEN, 
		TRUE, 
		&g_CDO);
	if (!NT_SUCCESS(status)) {
		//失败
		KdPrint(("SYS:Create CDO Fail(Error:0x%08x)!\n", status));
		return status;
	}
	g_CDO->Flags |= DO_BUFFERED_IO;
	//创建符号链接
	RtlInitUnicodeString(&lnkName, CONTROL_SYMBOL_LINK);
	status = IoCreateSymbolicLink(&lnkName, &devName);
	if (!NT_SUCCESS(status)) {
		KdPrint(("SYS:Create Symbolic Link Fail!Error Code:0x%08x\n", status));
		IoDeleteDevice(g_CDO);
		return status;
	}

	pDevExt = (PEP_DEVICE_EXTENSION)g_CDO->DeviceExtension;//驱动扩展结构
	RtlZeroMemory(pDevExt, sizeof(EP_DEVICE_EXTENSION));
	pDevExt->DeviceType = DO_CONTROL;

	//OK，注册系统关闭事件
	status = IoRegisterShutdownNotification(g_CDO);
	if (!NT_SUCCESS(status)) {
		KdPrint(("SYS:IoRegisterShutdownNotification Fail!Error Code:0x%08x\n", status));
		IoDeleteDevice(g_CDO);
		return status;
	}
	//初始化簇重定向后背列表，注意的是，后备列表的内存大小为sizeof(RTL_SPLAY_LINKS )+sizeof(LIST_ENTRY )+sizeof(LCXL_TABLE_MAP)
	ExInitializePagedLookasideList(
		&g_TableMapList, 
		NULL, 
		NULL, 
		0, 
		sizeof(RTL_SPLAY_LINKS )+sizeof(LIST_ENTRY )+sizeof(LCXL_TABLE_MAP), 
		'LOOK', 
		0
		);
	//初始化设置获取事件
	KeInitializeEvent(&g_DriverSetting.SettingInitEvent, NotificationEvent, FALSE);
	g_DriverSetting.DriverErrorType = DRIVER_ERROR_SUCCESS;//默认没有错误
	//初始化系统初始化完成事件
	KeInitializeEvent(&g_SysInitEvent, NotificationEvent, FALSE);

	//注册系统初始化完成事件
	IoRegisterDriverReinitialization(DriverObject, LCXLDriverReinitialize, NULL);
	return STATUS_SUCCESS;
}

VOID LCXLDriverUnload(
	IN PDRIVER_OBJECT DriverObject
	)
{
	UNICODE_STRING lnkName;//符号链接

	PAGED_CODE();
	UNREFERENCED_PARAMETER( DriverObject );
	ExDeletePagedLookasideList(&g_TableMapList);
	//删除符号链接
	RtlInitUnicodeString(&lnkName, CONTROL_SYMBOL_LINK);
	IoDeleteSymbolicLink(&lnkName);
	//删除控制设备
	IoDeleteDevice(g_CDO);

}

VOID
	LCXLDriverReinitialize (
	__in struct _DRIVER_OBJECT *DriverObject,
	__in_opt PVOID Context,
	__in ULONG Count
	)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(Context);
	UNREFERENCED_PARAMETER(Count);
	KdPrint(("SYS:LCXLDriverReinitialize(Count=%d)\n", Count));
	KeSetEvent(&g_SysInitEvent, 0, FALSE);
}

NTSTATUS LCXLOpenDriverSettingFile(OUT PHANDLE hFile, IN PDEVICE_OBJECT FSPDO, IN PUNICODE_STRING DiskVolName) 
{
	NTSTATUS status;
	UNICODE_STRING FilePath;
	WCHAR FilePathBuf[MAX_PATH+1];
	OBJECT_ATTRIBUTES oa;
	IO_STATUS_BLOCK IoStatusBlock;

	PAGED_CODE();

	RtlInitEmptyUnicodeString(&FilePath, FilePathBuf, sizeof(FilePathBuf));
	RtlCopyUnicodeString(&FilePath, DiskVolName);
	RtlUnicodeStringCatString(&FilePath, LCXL_SETTING_PATH);
	InitializeObjectAttributes(&oa, &FilePath, OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, NULL);
	//打开配置文件
	status = IoCreateFileSpecifyDeviceObjectHint(
		hFile, 
		GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE,
		&oa, 
		&IoStatusBlock, 
		NULL, 
		FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_ARCHIVE, 
		FILE_SHARE_READ, 
		FILE_OPEN_IF, 
		FILE_SYNCHRONOUS_IO_NONALERT|FILE_NON_DIRECTORY_FILE, 
		NULL, 
		0, 
		CreateFileTypeNone, 
		NULL, 
		0,  
		FSPDO
		);
	if (NT_SUCCESS(status)) {
		FILE_STANDARD_INFORMATION FileStandardInfo;
		//查询文件大小信息
		status = ZwQueryInformationFile(*hFile, &IoStatusBlock, &FileStandardInfo, sizeof(FileStandardInfo), FileStandardInformation);
		if (NT_SUCCESS(status)) {
			LARGE_INTEGER pos;
			PVOID Buffer;
			ULONG BufferSize;

			//检测文件是否完整，完整的判断是文件大小是否等于4K
			if (FileStandardInfo.EndOfFile.QuadPart != SETTING_FILE_SIZE) {
				g_DriverSetting.DriverErrorType = DRIVER_ERROR_SETTING_FILE_NOT_FOUND;
				//有两种情况，1种是文件缺失，一种是文件不完全了，这种情况都要重新初始化设置
				g_DriverSetting.SettingFile.ShadowMode = SM_NONE;
				g_DriverSetting.SettingFile.NeedPass = FALSE;
				g_DriverSetting.SettingFile.CustomProtectVolumeNum = 0;
				//将设置写到数据缓冲区中
				status = LCXLWriteSettingToBuffer(&g_DriverSetting, &Buffer, &BufferSize);
				if (NT_SUCCESS(status)) {
					//通过文件系统写到磁盘中
					pos.QuadPart = 0;
					status = ZwWriteFile(*hFile, NULL, NULL, NULL, &IoStatusBlock, Buffer, BufferSize, &pos, NULL);
					LCXLFreeSettingBuffer(Buffer);
				}
			}
		} else {
			KdPrint(("SYS:LCXLOpenDriverSettingFile:ZwQueryInformationFile(0x%08x) failed\n", status));
		}
		if (!NT_SUCCESS(status)) {
			ZwClose(*hFile);
			*hFile = NULL;
		} else {
			if (g_DriverSetting.FileClusters != NULL) {
				ExFreePool(g_DriverSetting.FileClusters);
				g_DriverSetting.FileClusters = NULL;
			}
			//获取配置文件簇列表
			GetFileClusterList(*hFile, &g_DriverSetting.FileClusters);
		}
	}
	return status;
}

//获取驱动的设置
//DeviceObject:文件系统过滤对象
NTSTATUS LCXLGetDriverSetting(PDEVICE_OBJECT DeviceObject)
{
	/*获取驱动设置的方法：
	 *如果没有发现驱动配置文件\LLShadowS.sys
	 *则通过IrpCreateFile生成一个4KB的文件
	 *然后获取LLShadowS.sys的簇列表，设置为直接读写
	 *以后对配置的写入，都直接通过磁盘卷写入配置，不通过文件系统（这样就不用考虑在文件系统中LLShadowS.sys被删的情况）
	 *--（不需要）对与这片区域，实行保护，未经允许的读写都将被拒绝。
	 *注意：此函数只在挂载第一个文件系统卷时被调用，其他情况都将直接ASSERT
	 */
	PEP_DEVICE_EXTENSION fsdevExt;
	PEP_DEVICE_EXTENSION voldevExt;
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS status;
	FILE_FS_SIZE_INFORMATION fssizeInfo;
	HANDLE hFile;

	PAGED_CODE();
	//到这里应该是正在初始化
	ASSERT(g_DriverSetting.InitState==DS_INITIALIZING);
	
	g_DriverSetting.SettingDO = DeviceObject;
	fsdevExt = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	//必须是文件系统过滤设备
	ASSERT(fsdevExt->DeviceType==DO_FSVOL);
	//下层的磁盘卷过滤驱动必须存在
	ASSERT(fsdevExt->DE_FILTER.DE_FSVOL.DiskVolFilter != NULL);

	voldevExt = (PEP_DEVICE_EXTENSION)fsdevExt->DE_FILTER.DE_FSVOL.DiskVolFilter->DeviceExtension;

	status = LCXLOpenDriverSettingFile(&hFile, fsdevExt->DE_FILTER.PhysicalDeviceObject, &voldevExt->DE_FILTER.PhysicalDeviceName);
	if (NT_SUCCESS(status)) {
		PVOID Buffer;
		ULONG BufferSize;

		BufferSize = SETTING_FILE_SIZE;
		Buffer = ExAllocatePoolWithTag(PagedPool, BufferSize, MEM_SETTING_TAG);
		if (Buffer != NULL) {
			LARGE_INTEGER pos;

			pos.QuadPart = 0;
			//读取
			status = ZwReadFile(hFile, NULL, NULL, NULL, &IoStatusBlock, Buffer, BufferSize, &pos, NULL);
			if (NT_SUCCESS(status)) {
				//将读取的内存写入到设置中
				LCXLReadSettingFromBuffer(Buffer, BufferSize, &g_DriverSetting);
				//设置当前保护模式
				g_DriverSetting.CurShadowMode = g_DriverSetting.SettingFile.ShadowMode;
			} else {
				KdPrint(("SYS:LCXLGetDriverSetting:ZwReadFile failed(0x%08x)\n", status));
			}
			//释放缓冲区内存
			ExFreePool(Buffer);
		} else {
			KdPrint(("SYS:LCXLGetDriverSetting:ExAllocatePoolWithTag failed(0x%08x)\n", status));
		}
		
		
		
		ZwClose(hFile);
	}
	
	
	g_DriverSetting.InitState = DS_INITIALIZED;
	//激活事件
	KeSetEvent(&g_DriverSetting.SettingInitEvent, 0, FALSE);
	if (NT_SUCCESS(status)) {
		OBJECT_ATTRIBUTES oa;
		InitializeObjectAttributes(&oa, &voldevExt->DE_FILTER.PhysicalDeviceName, OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, NULL);
		//打开驱动器
		status = IoCreateFileSpecifyDeviceObjectHint(
			&hFile, 
			GENERIC_READ | SYNCHRONIZE,
			&oa, 
			&IoStatusBlock, 
			NULL, 
			FILE_ATTRIBUTE_NORMAL, 
			FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, 
			FILE_OPEN, 
			FILE_SYNCHRONOUS_IO_NONALERT, 
			NULL, 
			0, 
			CreateFileTypeNone, 
			NULL, 
			0,  
			fsdevExt->DE_FILTER.PhysicalDeviceObject
			);
		if (NT_SUCCESS(status)) {

			//先获取簇(分配单元)信息//获取磁盘卷信息
			status = ZwQueryVolumeInformationFile(hFile,
				&IoStatusBlock,
				&fssizeInfo,
				sizeof(fssizeInfo),
				FileFsSizeInformation);
			if (NT_SUCCESS(status)) {
				g_DriverSetting.BytesPerAllocationUnit =  fssizeInfo.BytesPerSector * fssizeInfo.SectorsPerAllocationUnit;
				status = FGetFirstDataSectorOffset(hFile, &g_DriverSetting.DataClusterOffset);
				g_DriverSetting.DataClusterOffset = 
					g_DriverSetting.DataClusterOffset/fssizeInfo.SectorsPerAllocationUnit
					+(g_DriverSetting.DataClusterOffset%fssizeInfo.SectorsPerAllocationUnit>0);
			}
			ZwClose(hFile);
		}
	}
	
	return status;
}

NTSTATUS LCXLSaveShadowSetting()
{
	NTSTATUS status;
	PVOID Buffer;
	ULONG BufferSize;

	ASSERT(g_DriverSetting.InitState==DS_INITIALIZED);
	ASSERT(g_DriverSetting.SettingDO != NULL);
	//将内存写入到缓冲区中
	status = LCXLWriteSettingToBuffer(&g_DriverSetting, &Buffer, &BufferSize);
	if (NT_SUCCESS(status)) {
		PEP_DEVICE_EXTENSION fsdevext;
		PDEVICE_OBJECT VolDiskFilterDO;
		PEP_DEVICE_EXTENSION voldevext;
		PBYTE bufpos;

		KAPC_STATE ApcState;
		PEPROCESS Process;

		fsdevext = (PEP_DEVICE_EXTENSION)g_DriverSetting.SettingDO->DeviceExtension;
		ASSERT(fsdevext->DeviceType == DO_FSVOL);
		VolDiskFilterDO = fsdevext->DE_FILTER.DE_FSVOL.DiskVolFilter;
		voldevext = (PEP_DEVICE_EXTENSION)VolDiskFilterDO->DeviceExtension;
		ASSERT(voldevext->DeviceType == DO_DISKVOL);
		ASSERT(g_DriverSetting.FileClusters != NULL);

		//如果当前是未保护状态
		if (g_DriverSetting.CurShadowMode == SM_NONE) {
			//
			HANDLE hFile;
			//更新簇列表
			status = LCXLOpenDriverSettingFile(
				&hFile, 
				((PEP_DEVICE_EXTENSION)g_DriverSetting.SettingDO->DeviceExtension)->DE_FILTER.PhysicalDeviceObject, 
				&((PEP_DEVICE_EXTENSION)g_DriverSetting.SettingDO->DeviceExtension)->DE_FILTER.DE_FSVOL.DiskVolDeviceName
				);
			if (NT_SUCCESS(status)) {
				ZwClose(hFile);
			}
		}
		//切换到SYSTEM进程
		status = PsLookupProcessByProcessId((HANDLE)0x4, &Process);
		if (NT_SUCCESS(status)) {
			LARGE_INTEGER PrevVCN;
			//自己定义的数据结构，用来快速读写
			PLCXL_VCN_EXTENT pExtent;
			DWORD i;

			KeStackAttachProcess(Process, &ApcState);
		
			
			bufpos = (PBYTE)Buffer;
			if (g_DriverSetting.FileClusters != NULL) {
				//加入到直接读写位图中
				PrevVCN = g_DriverSetting.FileClusters->StartingVcn;
				for (i = 0, pExtent = (PLCXL_VCN_EXTENT)&g_DriverSetting.FileClusters->Extents[i]; i < g_DriverSetting.FileClusters->ExtentCount; i++, pExtent++) {
					ULONG writelen;
					IO_STATUS_BLOCK iostatus;
					KdPrint(("LCXL Setting:GetFileClusterList(%016I64x->%016I64x)\n", (ULONGLONG)pExtent->Lcn.QuadPart, (ULONGLONG)(pExtent->Lcn.QuadPart+pExtent->NextVcn.QuadPart-PrevVCN.QuadPart)));
					//计算长度
					writelen = (ULONG)((pExtent->NextVcn.QuadPart-PrevVCN.QuadPart)*g_DriverSetting.BytesPerAllocationUnit);
					//直接写入磁盘卷中
					status = VDirectReadWriteDiskVolume(
						voldevext->DE_FILTER.PhysicalDeviceObject, 
						IRP_MJ_WRITE, 
						(ULONGLONG)(pExtent->Lcn.QuadPart+voldevext->DE_FILTER.DE_DISKVOL.FsSetting.DataClusterOffset)*g_DriverSetting.BytesPerAllocationUnit, 
						bufpos, 
						writelen, 
						&iostatus, 
						TRUE); 
					if (NT_SUCCESS(status)) {
						bufpos += writelen;
						BufferSize -= writelen;
					} else {
						KdPrint(("SYS:LCXLSaveShadowSetting:VDirectReadWriteDiskVolume error!0%08x\n", status));
					}
					//获取下一个族列表的开始
					PrevVCN = pExtent->NextVcn;
				}
			} else {
				status = STATUS_FILE_INVALID;
			}
			ASSERT(BufferSize == 0);
			KeUnstackDetachProcess(&ApcState);
		}
		ObDereferenceObject(Process);

		LCXLFreeSettingBuffer(Buffer);
	}
	return status;
}

NTSTATUS LCXLWriteSettingToBuffer(
	IN PLCXL_DRIVER_SETTING DriverSetting, 
	OUT PVOID *Buffer,
	OUT PULONG BufferSize
	)
{
	PAGED_CODE();
	//检查参数是否正确
	ASSERT(DriverSetting!=NULL);
	ASSERT(Buffer != NULL);
	ASSERT(BufferSize != NULL);
	//设置缓冲区大小
	*BufferSize = SETTING_FILE_SIZE;
	*Buffer = ExAllocatePoolWithTag(PagedPool, *BufferSize, MEM_SETTING_TAG);
	if (*Buffer==NULL) {
		//内存不足
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	RtlCopyMemory(*Buffer, &DriverSetting->SettingFile, sizeof(DriverSetting->SettingFile));
	return STATUS_SUCCESS;
}

NTSTATUS LCXLReadSettingFromBuffer(
	IN PVOID Buffer,
	IN ULONG BufferSize,
	IN OUT PLCXL_DRIVER_SETTING DriverSetting
	)
{
	PAGED_CODE();

	ASSERT(DriverSetting!=NULL);
	ASSERT(Buffer != NULL);
	ASSERT(BufferSize == SETTING_FILE_SIZE);

	UNREFERENCED_PARAMETER(BufferSize);
	if (BufferSize == SETTING_FILE_SIZE) {
		RtlCopyMemory(&DriverSetting->SettingFile, Buffer, sizeof(DriverSetting->SettingFile));
		return STATUS_SUCCESS;
	} else {
		return STATUS_REQUEST_NOT_ACCEPTED;
	}
	
}

BOOLEAN LCXLVolumeNeedProtect(PDEVICE_OBJECT FSFilterDO)
{
	PEP_DEVICE_EXTENSION fsdevExt;
	PEP_DEVICE_EXTENSION voldevExt;

	PAGED_CODE();
	//设置必须初始化过
	ASSERT(g_DriverSetting.InitState==DS_INITIALIZED);

	fsdevExt = (PEP_DEVICE_EXTENSION)FSFilterDO->DeviceExtension;
	//必须是文件系统过滤设备
	ASSERT(fsdevExt->DeviceType==DO_FSVOL);
	//下层的磁盘卷过滤驱动必须存在
	ASSERT(fsdevExt->DE_FILTER.DE_FSVOL.DiskVolFilter != NULL);
	voldevExt = (PEP_DEVICE_EXTENSION)fsdevExt->DE_FILTER.DE_FSVOL.DiskVolFilter->DeviceExtension;
	//保护模式
	switch (g_DriverSetting.CurShadowMode) {
	case SM_NONE://不保护
		return FALSE;
	case SM_SYSTEM://保护系统盘
		return FSFilterDO==g_DriverSetting.SettingDO;
	case SM_ALL://保护所有盘
		return TRUE;
	case SM_CUSTOM:
		{
			ULONG i;
			UNICODE_STRING VolumeName;
			//如果是系统盘，则保护
			if (FSFilterDO == g_DriverSetting.SettingDO) {
				return TRUE;
			}
			for (i = 0; i<g_DriverSetting.SettingFile.CustomProtectVolumeNum; i++) {
				RtlInitUnicodeString(&VolumeName, g_DriverSetting.SettingFile.CustomProtectVolume[i]);
				if (RtlCompareUnicodeString(&voldevExt->DE_FILTER.PhysicalDeviceName, &VolumeName, TRUE)==0) {
					return TRUE;
				}
			}
		}
		return FALSE;
	default:
		ASSERT(0);
		return FALSE;
	}
}

//插入到重定向读写列表中
void LCXLInsertToRedirectRWMemList(IN PREDIRECT_RW_MEM RedirectListHead, IN PVOID buffer, IN ULONGLONG offset, IN ULONG length)
{
	PREDIRECT_RW_MEM RedirectListEntry;//重定向表项
	PREDIRECT_RW_MEM NewListEntry;

	//创建一个列表项
	NewListEntry = AllocateRedirectIrpListEntry(length, offset, 'LCXL');
	RtlCopyMemory(NewListEntry->buffer, buffer, length);
	RedirectListEntry = RedirectListHead;
	//将写入重定向，并尽可能的合并列表
	for (RedirectListEntry = (PREDIRECT_RW_MEM)RedirectListEntry->ListEntry.Flink; RedirectListEntry != RedirectListHead;) {
		PREDIRECT_RW_MEM TmpListEntry;
		//开始判断吧，boy!
		if (NewListEntry->offset+NewListEntry->length<=RedirectListEntry->offset) {
			//|----------| NewListEntry
			//            |---------| RedirectListEntry
			//情况0
			//退出
			break;
		} else {
			if (NewListEntry->offset<=RedirectListEntry->offset) {
				if (NewListEntry->offset+NewListEntry->length<RedirectListEntry->offset+RedirectListEntry->length) {
					//|----------| NewListEntry
					//    |---------| RedirectListEntry
					//情况1
					//摘除此项
					RedirectListEntry->ListEntry.Blink->Flink = RedirectListEntry->ListEntry.Flink;
					RedirectListEntry->ListEntry.Flink->Blink = RedirectListEntry->ListEntry.Blink;
					//开始合并
					TmpListEntry = AllocateRedirectIrpListEntry(
						(ULONG)(RedirectListEntry->offset-NewListEntry->offset+RedirectListEntry->length), 
						NewListEntry->offset, 
						'LCXL'
						);

					RtlCopyMemory(
						&((PBYTE)TmpListEntry->buffer)[NewListEntry->length], 
						&((PBYTE)RedirectListEntry->buffer)[NewListEntry->offset+NewListEntry->length-RedirectListEntry->offset], 
						(size_t)(RedirectListEntry->offset+RedirectListEntry->length-NewListEntry->offset-NewListEntry->length)
						);
					RtlCopyMemory(
						TmpListEntry->buffer, 
						NewListEntry->buffer, 
						NewListEntry->length
						);

					//释放内存

					FreeRedirectIrpListEntry(NewListEntry);
					NewListEntry = TmpListEntry;
					//RedirectListEntry设置为原RedirectListEntry的后一个元素
					TmpListEntry = (PREDIRECT_RW_MEM)RedirectListEntry->ListEntry.Flink;
					FreeRedirectIrpListEntry(RedirectListEntry);
					RedirectListEntry = TmpListEntry;
					//之后就不可能有可以合并的列表了，退出
					break;
				} else {
					//|----------| NewListEntry
					//    |-----| RedirectListEntry
					//情况2

					//摘除此项
					RedirectListEntry->ListEntry.Blink->Flink = RedirectListEntry->ListEntry.Flink;
					RedirectListEntry->ListEntry.Flink->Blink = RedirectListEntry->ListEntry.Blink;

					//RedirectListEntry设置为原RedirectListEntry的后一个元素
					TmpListEntry = (PREDIRECT_RW_MEM)RedirectListEntry->ListEntry.Flink;
					//释放内存
					FreeRedirectIrpListEntry(RedirectListEntry);
					RedirectListEntry = TmpListEntry;
				}
			} else {
				if (NewListEntry->offset+NewListEntry->length<=RedirectListEntry->offset+RedirectListEntry->length) {
					//   |----------| NewListEntry
					// |--------------| RedirectListEntry
					//情况3

					//摘除此项
					RedirectListEntry->ListEntry.Blink->Flink = RedirectListEntry->ListEntry.Flink;
					RedirectListEntry->ListEntry.Flink->Blink = RedirectListEntry->ListEntry.Blink;

					//这种情况下，无需申请内存
					RtlCopyMemory(
						&((PBYTE)RedirectListEntry->buffer)[NewListEntry->offset-RedirectListEntry->offset], 
						NewListEntry->buffer, 
						NewListEntry->length
						);

					//释放内存
					FreeRedirectIrpListEntry(NewListEntry);
					NewListEntry = RedirectListEntry;
					//RedirectListEntry设置为原RedirectListEntry的后一个元素
					RedirectListEntry = (PREDIRECT_RW_MEM)RedirectListEntry->ListEntry.Flink;
					//之后就不可能有可以合并的列表了，退出
					break;
				} else {
					if (NewListEntry->offset<RedirectListEntry->offset+RedirectListEntry->length) {
						//   |----------| NewListEntry
						// |---------| RedirectListEntry
						//情况4

						//摘除此项
						RedirectListEntry->ListEntry.Blink->Flink = RedirectListEntry->ListEntry.Flink;
						RedirectListEntry->ListEntry.Flink->Blink = RedirectListEntry->ListEntry.Blink;

						//开始合并
						TmpListEntry = AllocateRedirectIrpListEntry(
							(ULONG)(NewListEntry->offset+NewListEntry->length-RedirectListEntry->offset), 
							RedirectListEntry->offset, 
							'LCXL'
							);

						RtlCopyMemory(
							TmpListEntry->buffer, 
							RedirectListEntry->buffer, 
							(size_t)(NewListEntry->offset-RedirectListEntry->offset)
							);
						RtlCopyMemory(
							&((PBYTE)TmpListEntry->buffer)[NewListEntry->offset-RedirectListEntry->offset], 
							NewListEntry->buffer, 
							NewListEntry->length
							);
						//释放内存

						FreeRedirectIrpListEntry(NewListEntry);
						NewListEntry = TmpListEntry;
						//RedirectListEntry设置为原RedirectListEntry的后一个元素
						TmpListEntry = (PREDIRECT_RW_MEM)RedirectListEntry->ListEntry.Flink;
						FreeRedirectIrpListEntry(RedirectListEntry);
						RedirectListEntry = TmpListEntry;
					} else {
						//            |----------| NewListEntry
						// |---------| RedirectListEntry
						//情况5
						//不做任何事
						RedirectListEntry = (PREDIRECT_RW_MEM)RedirectListEntry->ListEntry.Flink;
					}
				}
			}
		}
	}
	//插入到重定向列表中
	NewListEntry->ListEntry.Blink = RedirectListEntry->ListEntry.Blink;
	NewListEntry->ListEntry.Flink =  &RedirectListEntry->ListEntry;

	NewListEntry->ListEntry.Flink->Blink = &NewListEntry->ListEntry;
	NewListEntry->ListEntry.Blink->Flink = &NewListEntry->ListEntry;
}

//从重定向读写列表中读取
void LCXLReadFromRedirectRWMemList(IN PREDIRECT_RW_MEM RedirectListHead, IN PVOID buffer, IN ULONGLONG offset, IN ULONG length)
{
	PREDIRECT_RW_MEM RedirectListEntry;

	RedirectListEntry = RedirectListHead;
	//循环查找我们读取的内容中有没有被重定向了
	for (RedirectListEntry = (PREDIRECT_RW_MEM)RedirectListEntry->ListEntry.Flink; RedirectListEntry != RedirectListHead; RedirectListEntry = (PREDIRECT_RW_MEM)RedirectListEntry->ListEntry.Flink) {
		//开始判断吧，boy!
		if (offset+length<=RedirectListEntry->offset) {
			//|----------| buffer
			//            |---------| RedirectListEntry
			//情况0
			//退出
			break;
		} else {
			if (offset<=RedirectListEntry->offset) {
				if (offset+length<=RedirectListEntry->offset+RedirectListEntry->length) {
					//|----------| buffer
					//    |---------| RedirectListEntry
					//情况1
					RtlCopyMemory(
						&((PBYTE)buffer)[RedirectListEntry->offset-offset], 
						RedirectListEntry->buffer, 
						(size_t)(offset+length-RedirectListEntry->offset)
						);
				} else {
					//|----------| buffer
					//    |-----| RedirectListEntry
					//情况2
					RtlCopyMemory(
						&((PBYTE)buffer)[RedirectListEntry->offset-offset], 
						RedirectListEntry->buffer, 
						RedirectListEntry->length
						);
				}
			} else {
				if (offset+length<=RedirectListEntry->offset+RedirectListEntry->length) {
					//   |----------| buffer
					// |--------------| RedirectListEntry
					//情况3
					RtlCopyMemory(
						buffer, 
						&((PBYTE)RedirectListEntry->buffer)[offset - RedirectListEntry->offset], 
						length
						);
				} else {
					if (offset < RedirectListEntry->offset+RedirectListEntry->length) {
						//   |----------| buffer
						// |---------| RedirectListEntry
						//情况4
						RtlCopyMemory(
							buffer, 
							&((PBYTE)RedirectListEntry->buffer)[offset-RedirectListEntry->offset], 
							(size_t)(RedirectListEntry->offset+RedirectListEntry->length-offset)
							);
					} else {
						//            |----------| buffer
						// |---------| RedirectListEntry
						//情况5
						//不做任何事

					}
				}
			}
		}
	}
	//RtlCopyMemory(buffer, RWBuffer->Buffer, length);
}

NTSTATUS LCXLChangeDriverIcon(PDEVICE_OBJECT VolDiskPDO)
{
	HANDLE	keyHandle;
	UNICODE_STRING	keyPath;
	OBJECT_ATTRIBUTES	objectAttributes;
	ULONG		ulResult;
	NTSTATUS	status;

	PAGED_CODE();
	
	RtlInitUnicodeString( &keyPath, L"\\Registry\\Machine\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\DriveIcons");   

	//初始化objectAttributes 
	InitializeObjectAttributes(&objectAttributes,   
		&keyPath,   
		OBJ_CASE_INSENSITIVE| OBJ_KERNEL_HANDLE,//对大小写不敏感
		NULL,
		NULL);

	status = ZwCreateKey( &keyHandle,   
		KEY_ALL_ACCESS,   
		&objectAttributes,   
		0,   
		NULL,   
		REG_OPTION_VOLATILE,   // 重启后无效
		&ulResult);

	if (NT_SUCCESS(status)) {
		HANDLE	subKey;
		UNICODE_STRING DosName;
		//查询磁盘卷的DOS名称

		status = IoVolumeDeviceToDosName(VolDiskPDO, &DosName);
		if (NT_SUCCESS(status)) {
			DosName.Length = sizeof(WCHAR);
			DosName.Buffer[1] = L'\0';
			InitializeObjectAttributes(&objectAttributes,   
				&DosName,   
				OBJ_CASE_INSENSITIVE| OBJ_KERNEL_HANDLE,//对大小写不敏感
				keyHandle,
				NULL);
			status = ZwCreateKey(
				&subKey,   
				KEY_ALL_ACCESS,   
				&objectAttributes,   
				0,   
				NULL,   
				REG_OPTION_VOLATILE,   // 重启后无效
				&ulResult
				);
			if (NT_SUCCESS(status)) {
				HANDLE	subsubKey;
				RtlInitUnicodeString( &keyPath, L"DefaultIcon");

				InitializeObjectAttributes(&objectAttributes,   
					&keyPath,   
					OBJ_CASE_INSENSITIVE| OBJ_KERNEL_HANDLE,//对大小写不敏感     
					subKey,
					NULL);

				status = ZwCreateKey( &subsubKey,   
					KEY_ALL_ACCESS,   
					&objectAttributes,   
					0,   
					NULL,   
					REG_OPTION_VOLATILE,   // 重启后无效
					&ulResult
				);

				if (NT_SUCCESS(status)) {
					UNICODE_STRING	keyName;
					UNICODE_STRING IconPath;

					RtlInitUnicodeString(&keyName, L"");
					if (IS_WINDOWSVISTA_OR_LATER()) {
						RtlInitUnicodeString(&IconPath, L"%SystemRoot%\\System32\\drivers\\LCXLShadowDriver.sys,1");
					} else {
						RtlInitUnicodeString(&IconPath, L"%SystemRoot%\\System32\\drivers\\LCXLShadowDriver.sys,0");
					}

					status = ZwSetValueKey(subsubKey, &keyName, 0,REG_SZ, IconPath.Buffer, IconPath.Length+ sizeof(WCHAR));

					ZwClose(subsubKey);
				}

				ZwClose(subKey);
			}
			//释放内存
			ExFreePool(DosName.Buffer);
		}
		ZwClose(keyHandle);
	}
	return status;
}

//控制设备
NTSTATUS CDeviceCreate(
	IN PDEVICE_OBJECT pDevObj, 
	IN PIRP pIrp
	)
{
	PIO_STACK_LOCATION stack;
	NTSTATUS status;
	UNREFERENCED_PARAMETER( pDevObj );
	PAGED_CODE();

	status = STATUS_UNSUCCESSFUL;
	stack = IoGetCurrentIrpStackLocation(pIrp);
	if (stack->FileObject != NULL) {
		PLCXL_FILE_CONTEXT FContext;
		FContext = (PLCXL_FILE_CONTEXT)ExAllocatePoolWithTag(PagedPool, sizeof(LCXL_FILE_CONTEXT), 'AKEY');
		if (FContext!=NULL) {
			FContext->IsCredible = FALSE;
			FContext->PasswordRight = !g_DriverSetting.SettingFile.NeedPass;
			stack->FileObject->FsContext2 = FContext;
			status = STATUS_SUCCESS;
		}
	}
	
	//还没有完成
	return LLCompleteRequest(STATUS_SUCCESS, pIrp, 0);
}

NTSTATUS CGetDeviceNamebySymbolicLink(
	IN OUT PUNICODE_STRING DeviceName
	)
{
	OBJECT_ATTRIBUTES	oa;
	NTSTATUS status;
	HANDLE linkHandle;

	InitializeObjectAttributes(&oa,
		DeviceName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);
	while (status = ZwOpenSymbolicLinkObject(&linkHandle, GENERIC_READ, &oa), NT_SUCCESS(status)) {
		ULONG ret;
		status = ZwQuerySymbolicLinkObject(linkHandle, DeviceName, &ret);
		ZwClose(linkHandle);
	}
	return STATUS_SUCCESS;
}

//通过DOS名称获取磁盘卷过滤设备
NTSTATUS CGetDiskVolFilterDOByDosName(IN LPWSTR DosName, OUT PDEVICE_OBJECT *DiskVolFilterDO)
{
	NTSTATUS status;
	UNICODE_STRING dosname;
	WCHAR dosnamebuf[MAX_PATH+1];

	PAGED_CODE();

	*DiskVolFilterDO = NULL;
	RtlStringCbCopyNW(dosnamebuf, sizeof(dosnamebuf), DosName, 64*sizeof(WCHAR));
	RtlInitUnicodeString(&dosname, dosnamebuf);
	dosname.MaximumLength = sizeof(dosnamebuf);
	//获取符号链接所指向的设备名称
	status = CGetDeviceNamebySymbolicLink(&dosname);
	if (NT_SUCCESS(status)) {
		status = CGetDiskVolFilterDOByVolumeName(&dosname, DiskVolFilterDO);
	}
	return status;
}

NTSTATUS CGetDiskVolFilterDOByVolumeName(IN PUNICODE_STRING VolumeName, OUT PDEVICE_OBJECT *DiskVolFilterDO)
{
	NTSTATUS status;
	PDEVICE_OBJECT dev;
	PEP_DEVICE_EXTENSION devext;

	PAGED_CODE()

	dev = g_CDO->DriverObject->DeviceObject;
	//循环查找
	while (dev != NULL) {
		//循环查找DO_DISKVOL类型的过滤设备
		devext = (PEP_DEVICE_EXTENSION)dev->DeviceExtension;
		if (devext->DeviceType == DO_DISKVOL) {
			if (RtlCompareUnicodeString(&devext->DE_FILTER.PhysicalDeviceName, VolumeName, TRUE)==0) {
				break;
			}
		}
		dev = dev->NextDevice;
	}
	if (dev != NULL) {
		status = STATUS_SUCCESS;
		*DiskVolFilterDO = dev;
	} else {
		//没有找到
		status = STATUS_OBJECT_PATH_NOT_FOUND;
	}
	return status;
}

NTSTATUS CDeviceControl(
	IN PDEVICE_OBJECT pDevObj, 
	IN PIRP pIrp
	)
{
	PIO_STACK_LOCATION stack;
	NTSTATUS status;
	ULONG cbin;//输入缓冲区大小
	PVOID pin;//输入指针
	ULONG cbout;//输出缓冲区大小
	PVOID pout;//输出指针
	ULONG code;//IOCTL码
	ULONG_PTR info = 0;//操作了多少字节
	PLCXL_FILE_CONTEXT filecontext;//文件对象上下文

	UNREFERENCED_PARAMETER( pDevObj );
	PAGED_CODE();
	status = STATUS_UNSUCCESSFUL;
	//得到当前堆栈
	stack = IoGetCurrentIrpStackLocation(pIrp);
	//得到输入缓冲区大小
	cbin = stack->Parameters.DeviceIoControl.InputBufferLength;
	//得到输出缓冲区大小
	cbout = stack->Parameters.DeviceIoControl.OutputBufferLength;
	//输入输出缓冲区
	pin = pout = pIrp->AssociatedIrp.SystemBuffer;
	//得到IOCTL码
	code = stack->Parameters.DeviceIoControl.IoControlCode;

	filecontext = NULL;
	//检查文件对象的上下文
	if (stack->FileObject != NULL && stack->FileObject->FsContext2!=NULL) {
		filecontext = (PLCXL_FILE_CONTEXT)stack->FileObject->FsContext2;
	} else {
		return LLCompleteRequest(STATUS_UNSUCCESSFUL, pIrp, 0);
	}
	if (code != IOCTL_GET_RANDOM_ID&&code != IOCTL_VALIDATE_CODE&&code != IOCTL_IN_SHADOW_MODE) {
		//程序不可信？
		if (!filecontext->IsCredible) {
			return LLCompleteRequest(STATUS_ACCESS_DENIED, pIrp, 0);
		}
	}

	switch (code) {
		//不需要AES验证
	case IOCTL_GET_RANDOM_ID://获取KEY
		//使用128bit AES KEY
		if (cbout == 16) {
			LARGE_INTEGER CurTick;

			//获取开机时间1
			KeQueryTickCount(&CurTick);
			//KeQueryTimeIncrement
			*(PLARGE_INTEGER)filecontext->aeskey = CurTick;
			//获取开机时间2
			KeQueryTickCount(&CurTick);
			*((PLARGE_INTEGER)filecontext->aeskey+1) = CurTick;
			RtlCopyMemory(pout, filecontext->aeskey, 16);
			info = cbout;
			status = STATUS_SUCCESS;
		} else {
			status = STATUS_INFO_LENGTH_MISMATCH;
		}
		
		break;
	case IOCTL_VALIDATE_CODE://验证KEY
		 {
			aes_ctx ctx;
			//生成解密上下文
			if (aes_dec_key(filecontext->aeskey, sizeof(filecontext->aeskey)/sizeof(BYTE), &ctx)==aes_good) {
				IDINFO decidinfo;
				if (aes_dec_mem(pin, cbin, &decidinfo, sizeof(decidinfo), &ctx)==aes_good) {
					
					BOOLEAN resu;
					//检查硬件信息是否一致
					resu = RtlCompareMemory(
							decidinfo.wVendorUnique, 
							g_DriverSetting.DiskInfo.DiskIDInfo.wVendorUnique, 
							sizeof(decidinfo.wVendorUnique)
						) == sizeof(decidinfo.wVendorUnique)&&
						RtlCompareMemory(
							decidinfo.sSerialNumber, 
							g_DriverSetting.DiskInfo.DiskIDInfo.sSerialNumber, 
							sizeof(decidinfo.sSerialNumber)
						) == sizeof(decidinfo.sSerialNumber)&&
						RtlCompareMemory(
							decidinfo.sModelNumber, 
							g_DriverSetting.DiskInfo.DiskIDInfo.sModelNumber, 
							sizeof(decidinfo.sModelNumber)
						) == sizeof(decidinfo.sModelNumber);
					if (resu) {
						
						filecontext->IsCredible = TRUE;
						status = STATUS_SUCCESS;
					} else {
						KdPrint(("SYS:IOCTL_VALIDATE_CODE Validate Code Fault\n"));
						status = STATUS_ACCESS_DENIED;
					}
					
				} else {
					status = STATUS_ACCESS_DENIED;
				}
			} else {
				status = STATUS_ACCESS_DENIED;
			}
		}
		break;
	case IOCTL_IN_SHADOW_MODE:
		if (g_DriverSetting.CurShadowMode != SM_NONE) {
			status = STATUS_SUCCESS;
		} else {
			status = STATUS_UNSUCCESSFUL;
		}
		break;
		//--不需要AES验证
	case IOCTL_GET_DISK_LIST:
		//获取磁盘列表
		if (cbout%sizeof(VOLUME_DISK_INFO)==0) {
			PVOLUME_DISK_INFO curdiskinfo;
			PDEVICE_OBJECT dev;

			status = STATUS_SUCCESS;
			curdiskinfo = (PVOLUME_DISK_INFO)pout;
			dev = pDevObj->DriverObject->DeviceObject;
			//循环查找
			while (dev != NULL) {
				PEP_DEVICE_EXTENSION devext;

				//循环查找DO_DISKVOL类型的过滤设备
				devext = (PEP_DEVICE_EXTENSION)dev->DeviceExtension;
				if (devext->DeviceType == DO_DISKVOL) {
					UNICODE_STRING dosname;

					if ((ULONG_PTR)(curdiskinfo+1)-(ULONG_PTR)pout <= cbout) {
						//查询驱动器符号
						status = IoVolumeDeviceToDosName(devext->DE_FILTER.PhysicalDeviceObject, &dosname);
						if (NT_SUCCESS(status)) {
							RtlCopyMemory(curdiskinfo->DosName, dosname.Buffer, dosname.Length);
							curdiskinfo->DosName[dosname.Length/sizeof(WCHAR)] = L'\0';
							ExFreePool(dosname.Buffer);
						} else {
							curdiskinfo->DosName[0] = L'\0';
						}
						//复制磁盘卷信息
						curdiskinfo->IsProtect = devext->DE_FILTER.DE_DISKVOL.IsProtect;
						curdiskinfo->IsSaveShadowData = devext->DE_FILTER.DE_DISKVOL.IsSaveShadowData;
						RtlCopyMemory(curdiskinfo->VolumeDiskName, devext->DE_FILTER.PhysicalDeviceNameBuffer, devext->DE_FILTER.PhysicalDeviceName.Length);
						curdiskinfo->VolumeDiskName[devext->DE_FILTER.PhysicalDeviceName.Length/sizeof(WCHAR)] = L'\0';
						curdiskinfo++;
					} else {
						status = STATUS_INFO_LENGTH_MISMATCH;
						break;
					}
				}
				dev = dev->NextDevice;
			}
			if (NT_SUCCESS(status)) {
				info = (ULONG_PTR)curdiskinfo-(ULONG_PTR)pout;
			}
		} else {
			status = STATUS_INFO_LENGTH_MISMATCH;
		}
		break;
	case IOCTL_GET_DRIVER_SETTING:
		//检查输出缓冲区大小
		if (cbout==sizeof(APP_DRIVER_SETTING)) {
			((PAPP_DRIVER_SETTING)pout)->CurShadowMode = g_DriverSetting.CurShadowMode;
			((PAPP_DRIVER_SETTING)pout)->NextShadowMode = g_DriverSetting.SettingFile.ShadowMode;
			info = sizeof(APP_DRIVER_SETTING);
			status = STATUS_SUCCESS;
		} else {
			status = STATUS_INFO_LENGTH_MISMATCH;
		}
		break;
	case IOCTL_SET_NEXT_REBOOT_SHADOW_MODE://设置下次启动的影子模式
		if (cbin == sizeof(LCXL_SHADOW_MODE)) {
			if (filecontext->PasswordRight) {
				g_DriverSetting.SettingFile.ShadowMode = *(PLCXL_SHADOW_MODE)pin;
				info = sizeof(LCXL_SHADOW_MODE);
				//保存设置
				status = LCXLSaveShadowSetting();
			} else {
				//密码失败
				status = STATUS_LOGON_FAILURE;
			}
			
		} else {
			status = STATUS_INFO_LENGTH_MISMATCH;
		}
		break;
	case IOCTL_GET_CUSTOM_DISK_LIST:
		if (cbout%(MAX_DEVNAME_LENGTH*sizeof(WCHAR))==0) {
			ULONG diskbuflen;
			PWCHAR diskpos;
			ULONG i;

			diskbuflen = cbout/(MAX_DEVNAME_LENGTH*sizeof(WCHAR));
			diskpos = (PWCHAR)pout;
			if (diskbuflen<g_DriverSetting.SettingFile.CustomProtectVolumeNum) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			for (i = 0; i < g_DriverSetting.SettingFile.CustomProtectVolumeNum; i++) {
				UNICODE_STRING VolumeName;
				PDEVICE_OBJECT dev;
				PEP_DEVICE_EXTENSION devext;

				RtlInitUnicodeString(&VolumeName, g_DriverSetting.SettingFile.CustomProtectVolume[i]);
				dev = pDevObj->DriverObject->DeviceObject;
				//循环查找
				while (dev != NULL) {
					//循环查找DO_DISKVOL类型的过滤设备
					devext = (PEP_DEVICE_EXTENSION)dev->DeviceExtension;
					if (devext->DeviceType == DO_DISKVOL) {
						if (RtlCompareUnicodeString(&devext->DE_FILTER.PhysicalDeviceName, &VolumeName, TRUE)==0) {
							break;
						}
					}
					dev = dev->NextDevice;
				}
				//如果找到了
				if (dev != NULL) {
					if (NT_SUCCESS(RtlStringCbCopyNW(
						diskpos, 
						MAX_DEVNAME_LENGTH*sizeof(MAX_DEVNAME_LENGTH), 
						VolumeName.Buffer, 
						VolumeName.Length
						))) {
						diskpos+=MAX_DEVNAME_LENGTH;
					}
				}
			}
			status = STATUS_SUCCESS;
			info = (ULONG_PTR)diskpos-(ULONG_PTR)pout;
		} else {
			status = STATUS_INFO_LENGTH_MISMATCH;
		}
		break;
	case IOCTL_SET_CUSTOM_DISK_LIST:
		//设置自定义保护磁盘列表
		if (cbin%(MAX_DEVNAME_LENGTH*sizeof(WCHAR))==0) {
			if (filecontext->PasswordRight) {
				ULONG disklen;
				PWCHAR disklist;
				ULONG i;

				disklen = cbin/(MAX_DEVNAME_LENGTH*sizeof(WCHAR));
				disklist = (PWCHAR)pin;
				g_DriverSetting.SettingFile.CustomProtectVolumeNum = 0;
				for (i = 0; i < disklen; i++) {

					if (NT_SUCCESS(RtlStringCbCopyNW(
						g_DriverSetting.SettingFile.CustomProtectVolume[g_DriverSetting.SettingFile.CustomProtectVolumeNum], 
						sizeof(g_DriverSetting.SettingFile.CustomProtectVolume[g_DriverSetting.SettingFile.CustomProtectVolumeNum]), 
						disklist, 
						MAX_DEVNAME_LENGTH*sizeof(MAX_DEVNAME_LENGTH)
						))) {
							g_DriverSetting.SettingFile.CustomProtectVolumeNum++;
							disklist+=MAX_DEVNAME_LENGTH;
					}

				}
				info = g_DriverSetting.SettingFile.CustomProtectVolumeNum*sizeof(WCHAR);
				//保存设置
				status = LCXLSaveShadowSetting();
			} else {
				status = STATUS_LOGON_FAILURE;
			}
		} else {
			status = STATUS_INFO_LENGTH_MISMATCH;
		}
		break;
	case IOCTL_APP_RQUESET:
		if (cbout == sizeof(APP_REQUEST)&&cbin == sizeof(APP_REQUEST)) {
			PDEVICE_OBJECT DiskVolFilterDO;
			PAPP_REQUEST AppReq;
			UNICODE_STRING VolumeName;

			AppReq = (PAPP_REQUEST)pout;
			RtlInitUnicodeString(&VolumeName, AppReq->VolumeName);
			status = CGetDiskVolFilterDOByVolumeName(&VolumeName, &DiskVolFilterDO);
			if (NT_SUCCESS(status)) {
				PAPP_REQUEST_LIST_ENTRY AppReqListEntry;
				//不能是分页内存，因为要使用自旋锁
				AppReqListEntry = (PAPP_REQUEST_LIST_ENTRY)ExAllocatePoolWithTag(NonPagedPool, sizeof(APP_REQUEST_LIST_ENTRY), 'APPR');
				if (AppReqListEntry!=NULL) {
					PEP_DEVICE_EXTENSION voldevext;

					voldevext = (PEP_DEVICE_EXTENSION)DiskVolFilterDO->DeviceExtension;
					//拷贝数据
					RtlCopyMemory(&AppReqListEntry->AppRequest, AppReq, sizeof(APP_REQUEST));
					//初始化事件
					KeInitializeEvent(&AppReqListEntry->FiniEvent, SynchronizationEvent, FALSE);
					//加入到列表中
					ExInterlockedInsertTailList(
						&voldevext->DE_FILTER.DE_DISKVOL.AppRequsetListHead.ListEntry,
						&AppReqListEntry->ListEntry,
						&voldevext->DE_FILTER.DE_DISKVOL.RequestListLock
						);
					KeSetEvent(&voldevext->DE_FILTER.DE_DISKVOL.RequestEvent, 0, FALSE);
					//等待
					status = KeWaitForSingleObject(&AppReqListEntry->FiniEvent, UserRequest, KernelMode, FALSE, NULL);
					if (NT_SUCCESS(status)) {
						status = AppReqListEntry->status;
						if (NT_SUCCESS(status)) {
							//拷贝结果
							RtlCopyMemory(AppReq, &AppReqListEntry->AppRequest, sizeof(APP_REQUEST));
							info = cbout;
						}
					}
					//释放内存
					ExFreePool(AppReqListEntry);
				} else {
					//系统资源不足
					status = STATUS_INSUFFICIENT_RESOURCES;
				}
			}
		} else {
			status = STATUS_INFO_LENGTH_MISMATCH;
		}
		break;
	case IOCTL_START_SHADOW_MODE:
		if (cbin == sizeof(LCXL_SHADOW_MODE)) {
			if (filecontext->PasswordRight) {
				LCXL_SHADOW_MODE  CurShadowMode;

				CurShadowMode = *(PLCXL_SHADOW_MODE)pin;
				if (CurShadowMode != SM_NONE && g_DriverSetting.CurShadowMode == SM_NONE) {
					PEP_DEVICE_EXTENSION devext;
					PDEVICE_OBJECT dev;

					g_DriverSetting.CurShadowMode = CurShadowMode;
					dev = pDevObj->DriverObject->DeviceObject;
					//循环查找
					while (dev != NULL) {
						//循环查找DO_DISKVOL类型的过滤设备
						devext = (PEP_DEVICE_EXTENSION)dev->DeviceExtension;
						if (devext->DeviceType == DO_DISKVOL) {
							//重置磁盘影子状态
							devext->DE_FILTER.DE_DISKVOL.InitializeStatus = DVIS_NONE;
							//开始获取文件系统信息
							status = VGetFSInformation(dev);
							if (devext->DE_FILTER.DE_DISKVOL.IsProtect) {
								//更改图标等操作
								VPostProtectedFSInfo(devext);
							}
						}
						dev = dev->NextDevice;
						info = cbin;
					}
				} else {
					status = STATUS_INVALID_DEVICE_REQUEST;
				}
			} else {
				status = STATUS_LOGON_FAILURE;
			}
			
		} else {
			status = STATUS_INFO_LENGTH_MISMATCH;
		}
		break;
	case IOCTL_SET_SAVE_DATA_WHEN_SHUTDOWN:
		if (cbin == sizeof(SAVE_DATA_WHEN_SHUTDOWN)) {
			if (filecontext->PasswordRight) {
				PSAVE_DATA_WHEN_SHUTDOWN SaveData;
				PDEVICE_OBJECT DiskVolFilterDO;

				SaveData = (PSAVE_DATA_WHEN_SHUTDOWN)pin;
				status = CGetDiskVolFilterDOByDosName(SaveData->VolumeName, &DiskVolFilterDO);
				if (NT_SUCCESS(status)) {
					PEP_DEVICE_EXTENSION voldevext;

					voldevext = (PEP_DEVICE_EXTENSION)DiskVolFilterDO->DeviceExtension;
					voldevext->DE_FILTER.DE_DISKVOL.IsSaveShadowData = SaveData->IsSaveData;
					info = cbin;
				}
			} else {
				status = STATUS_LOGON_FAILURE;
			}
			
		} else {
			status = STATUS_INFO_LENGTH_MISMATCH;
		}
		break;
	case IOCTL_VERIFY_PASSWORD:
		if (cbin == sizeof(g_DriverSetting.SettingFile.Passmd5)) {
			UNICODE_STRING password1;
			UNICODE_STRING password2;

			password1.Buffer = (PWCH)pin;
			password1.MaximumLength = password1.Length = (USHORT)cbin;

			password2.Buffer = g_DriverSetting.SettingFile.Passmd5;
			password2.MaximumLength = password2.Length = (USHORT)cbin;
			//比较MD5值
			if (RtlCompareUnicodeString(&password1, &password2, TRUE)==0) {
				filecontext->PasswordRight = TRUE;
				status = STATUS_SUCCESS;
			} else {
				//失败
				status = STATUS_LOGON_FAILURE;
			}
		} else {
			if (filecontext->PasswordRight&&cbin == 0) {
				status = STATUS_SUCCESS;
			} else {
				status = STATUS_INFO_LENGTH_MISMATCH;
			}
			
		}
		break;
	case IOCTL_SET_PASSWORD:
		if (filecontext->PasswordRight) {
			if (cbin == 0) {
				g_DriverSetting.SettingFile.NeedPass = FALSE;
				//保存设置
				status = LCXLSaveShadowSetting();
			} else {
				if (cbin == sizeof(g_DriverSetting.SettingFile.Passmd5)) {
					g_DriverSetting.SettingFile.NeedPass = TRUE;
					RtlCopyMemory(g_DriverSetting.SettingFile.Passmd5, pin, cbin);
					//保存设置
					status = LCXLSaveShadowSetting();
				} else {
					status = STATUS_INFO_LENGTH_MISMATCH;
				}
			}
		} else {
			status = STATUS_LOGON_FAILURE;
		}
		break;
	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}
	return LLCompleteRequest(status, pIrp, info);
}

NTSTATUS CDeviceClose(
	IN PDEVICE_OBJECT pDevObj, 
	IN PIRP pIrp
	)
{
	PIO_STACK_LOCATION stack;

	UNREFERENCED_PARAMETER(pDevObj);
	stack = IoGetCurrentIrpStackLocation(pIrp);
	if (stack->FileObject != NULL) {
		if (stack->FileObject->FsContext2 != NULL) {
			ExFreePool(stack->FileObject->FsContext2);
		}
	}
	return LLCompleteRequest(STATUS_SUCCESS, pIrp, 0);
}

NTSTATUS CShutdown(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	)
{
	PDEVICE_OBJECT DiskVolFilterDO;
	PEP_DEVICE_EXTENSION voldevext;
	NTSTATUS status;
	BOOLEAN IsFirst;

	PAGED_CODE();

	UNREFERENCED_PARAMETER(DeviceObject);
	KdPrint(("SYS:CShutdown\n"));
	status = STATUS_SUCCESS;
	IsFirst = TRUE;
	//查找设备类型为DO_DISKVOL的设备，即查找磁盘卷过滤设备
	for (DiskVolFilterDO = g_CDO->DriverObject->DeviceObject; DiskVolFilterDO != NULL; DiskVolFilterDO = DiskVolFilterDO->NextDevice) {
		voldevext = (PEP_DEVICE_EXTENSION)DiskVolFilterDO->DeviceExtension;
		//如果是磁盘卷过滤设备
		if (voldevext->DeviceType==DO_DISKVOL) {
			//如果此磁盘有持久化标记
			if ((voldevext->DE_FILTER.DE_DISKVOL.InitializeStatus==DVIS_INITIALIZED||voldevext->DE_FILTER.DE_DISKVOL.InitializeStatus==DVIS_WORKING)&&
				voldevext->DE_FILTER.DE_DISKVOL.IsProtect&&voldevext->DE_FILTER.DE_DISKVOL.IsSaveShadowData&&!voldevext->DE_FILTER.DE_DISKVOL.IsRWError) {
					ULONG i;
					BOOLEAN IsChanged;

					IsChanged = FALSE;
					for (i = 0; i < voldevext->DE_FILTER.DE_DISKVOL.DiskFilterNum; i++) {
						PEP_DEVICE_EXTENSION diskdevext;

						diskdevext = (PEP_DEVICE_EXTENSION)voldevext->DE_FILTER.DE_DISKVOL.DiskFilterDO[i]->DeviceExtension;
						if (diskdevext->DE_FILTER.DE_DISK.IsVolumeChanged) {
							IsChanged = TRUE;
							break;
						}
					}
					//如果磁盘卷所在的磁盘已经改变，那么就不能保存数据
					if (IsChanged) {
						continue;
					}
					if (IsFirst) {
						IsFirst = FALSE;
						//如果是Windows Vista以后的操作系统，则不显示
						//if (!IS_WINDOWSVISTA_OR_LATER()) {
							if (InbvIsBootDriverInstalled()) {
								InbvAcquireDisplayOwnership();
								InbvResetDisplay();
								InbvSolidColorFill(0,0,639,479,4); 
								InbvSetTextColor(15);
								InbvEnableDisplayString(TRUE);  
								InbvSetScrollRegion(0,0,500,800); 
							}
							InbvDisplayString("========================LCXLShadow Saving Mode========================\n");
							InbvDisplayString("!!!!!Please DON'T SHUTDOWN your computer, it will be self-closing!!!!!\n");
						//}
					}
					//工作线程切换到数据保存模式
					voldevext->DE_FILTER.DE_DISKVOL.InitializeStatus = DVIS_SAVINGDATA;
					//激活事件，让工作线程开始执行影子持久化操作
					KeSetEvent(&voldevext->DE_FILTER.DE_DISKVOL.RequestEvent, 0, FALSE);
					//等待影子持久化完成
					KeWaitForSingleObject(&voldevext->DE_FILTER.DE_DISKVOL.SaveShadowDataFinishEvent, UserRequest, KernelMode, FALSE, NULL);
					KdPrint(("DVIS_SAVINGDATA-OK\n"));
			}
		}
	}
	InbvNotifyDisplayOwnershipLost(NULL);
	//完成IRP
	return LLCompleteRequest(STATUS_SUCCESS, Irp, 0);
}