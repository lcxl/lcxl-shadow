#include "winkernel.h"
#include "driver.h"
#include "diskvolfilter.h"
#include "filesysfilter.h"
//磁盘卷过滤设备


NTSTATUS VCreateVolumeThread(
	IN PDEVICE_OBJECT DiskVolFilterDO
	)
{
	PEP_DEVICE_EXTENSION devext;
	NTSTATUS status;

	PAGED_CODE();
	devext = (PEP_DEVICE_EXTENSION)DiskVolFilterDO->DeviceExtension;
	//关闭线程标示为FALSE
	devext->DE_FILTER.DE_DISKVOL.ToTerminateThread = FALSE;
	//读写错误默认为FALSE，
	devext->DE_FILTER.DE_DISKVOL.IsRWError = FALSE;
	//初始化这个卷的请求处理队列
	InitializeListHead(&devext->DE_FILTER.DE_DISKVOL.RWListHead);
	//初始化应用程序请求队列
	InitializeListHead(&devext->DE_FILTER.DE_DISKVOL.AppRequsetListHead.ListEntry);
	//初始化内存重定向列表
	InitializeListHead(&devext->DE_FILTER.DE_DISKVOL.RedirectMem.RedirectListHead.ListEntry);
	//初始化请求处理队列的锁
	KeInitializeSpinLock(&devext->DE_FILTER.DE_DISKVOL.RequestListLock);
	//初始化读写请求事件（同步事件）
	KeInitializeEvent(&devext->DE_FILTER.DE_DISKVOL.RequestEvent, SynchronizationEvent, FALSE);
	//初始化影子保存事件（通知事件）
	KeInitializeEvent(&devext->DE_FILTER.DE_DISKVOL.SaveShadowDataFinishEvent, NotificationEvent, FALSE);
	//卷初始化标志
	devext->DE_FILTER.DE_DISKVOL.InitializeStatus = DVIS_NONE;
	/*
	//查看此时驱动设置有没有初始化完成
	if (g_DriverSetting.InitState =  DS_INITIALIZED) {
		//如果不需要保护
		if (!LCXLVolumeNeedProtect(DiskVolFilterDO)) {
			//直接设定初始化完成
			devext->DE_FILTER.DE_DISKVOL.InitializeStatus = DVIS_INITIALIZED;
			devext->DE_FILTER.DE_DISKVOL.IsProtect = FALSE;
		}
	}
	*/
	//创建系统线程，上下文传入磁盘卷过滤设备的设备对象
	devext->DE_FILTER.DE_DISKVOL.ThreadHandle = NULL;
	status = PsCreateSystemThread(&devext->DE_FILTER.DE_DISKVOL.ThreadHandle, 
		(ACCESS_MASK)0L,
		NULL,
		NULL,
		NULL,
		VReadWriteThread, 
		DiskVolFilterDO);

	if (NT_SUCCESS(status)) {
		//获取线程句柄
		status = ObReferenceObjectByHandle(devext->DE_FILTER.DE_DISKVOL.ThreadHandle,
			0,
			*PsThreadType,
			KernelMode,
			(PVOID *)&devext->DE_FILTER.DE_DISKVOL.ThreadObject,
			NULL
			);
		if (!NT_SUCCESS(status)) {
			//不成功？
			devext->DE_FILTER.DE_DISKVOL.ToTerminateThread = TRUE;
			//激活线程
			KeSetEvent(&devext->DE_FILTER.DE_DISKVOL.RequestEvent, 0, TRUE);
			ASSERT(0);
		}
		KdPrint(("SYS:VCreateVolumeThread:PsCreateSystemThread succeed.(handle = 0x%p)\n", devext->DE_FILTER.DE_DISKVOL.ThreadHandle));
	} else {
		KdPrint(("SYS:VCreateVolumeThread:PsCreateSystemThread failed.(0x%08x)\n", status));
	}
	return status;
}

NTSTATUS VCloseVolumeThread(
	IN PDEVICE_OBJECT DiskVolFilterDO
	)
{
	NTSTATUS status;
	PEP_DEVICE_EXTENSION devext;

	devext = (PEP_DEVICE_EXTENSION)DiskVolFilterDO->DeviceExtension;
	if (devext->DE_FILTER.DE_DISKVOL.ThreadHandle!=NULL) {
		//结束标志设为TRUE
		devext->DE_FILTER.DE_DISKVOL.ToTerminateThread = TRUE;
		//激活线程
		KeSetEvent(&devext->DE_FILTER.DE_DISKVOL.RequestEvent, 0, TRUE);
		
		//等待线程退出
		status = KeWaitForSingleObject(devext->DE_FILTER.DE_DISKVOL.ThreadObject, UserRequest, KernelMode, FALSE, NULL);
		if (!NT_SUCCESS(status)) {
			KdPrint(("SYS:VPostDetachDiskVolDevice:KeWaitForSingleObject failed(0x%08x).\n", status));
		}
		//减少引用
		ObDereferenceObject(devext->DE_FILTER.DE_DISKVOL.ThreadObject);
		//关闭句柄
		ZwClose(devext->DE_FILTER.DE_DISKVOL.ThreadHandle);
		devext->DE_FILTER.DE_DISKVOL.ThreadHandle = NULL;
	}
	return STATUS_SUCCESS;
}

//获取磁盘卷上层的文件系统的信息
NTSTATUS VGetFSInformation(
	IN PDEVICE_OBJECT DiskVolFilterDO
	)
{
	NTSTATUS status;
	//获取卷信息
	HANDLE hFile;
	IO_STATUS_BLOCK ioStatus;
	OBJECT_ATTRIBUTES oa;

	PEP_DEVICE_EXTENSION voldevext;
	PEP_DEVICE_EXTENSION fsdevext;
	PDEVICE_OBJECT FileSyeFilterDO;//文件系统过滤函数

	PAGED_CODE();
	
	voldevext = (PEP_DEVICE_EXTENSION)DiskVolFilterDO->DeviceExtension;
	//必须是没有初始化过的
	ASSERT(voldevext->DE_FILTER.DE_DISKVOL.InitializeStatus==DVIS_NONE);
	FileSyeFilterDO = voldevext->DE_FILTER.DE_DISKVOL.FSVolFilter;
	ASSERT(FileSyeFilterDO!=NULL);
	fsdevext = (PEP_DEVICE_EXTENSION)FileSyeFilterDO->DeviceExtension;

	//设置必须初始化完成
	ASSERT(g_DriverSetting.InitState==DS_INITIALIZED);
	if (g_DriverSetting.InitState!=DS_INITIALIZED) {
		//过滤驱动还没有初始化
		return STATUS_FLT_NOT_INITIALIZED;
	}
	//获取此磁盘是否需要保护
	voldevext->DE_FILTER.DE_DISKVOL.IsProtect = LCXLVolumeNeedProtect(FileSyeFilterDO);
	//如果不需要保护，直接返回成功
	if (!voldevext->DE_FILTER.DE_DISKVOL.IsProtect) {
		voldevext->DE_FILTER.DE_DISKVOL.InitializeStatus = DVIS_INITIALIZED;
		KeSetEvent(&voldevext->DE_FILTER.DE_DISKVOL.RequestEvent, 0, FALSE);
		return STATUS_SUCCESS;
	}
	//如果需要保护，将标示改成“正在初始化”
	voldevext->DE_FILTER.DE_DISKVOL.InitializeStatus = DVIS_INITIALIZING;
	KdPrint(("SYS:VGetFSInformation: Protect this volume(%wZ)\n", &voldevext->DE_FILTER.PhysicalDeviceName));

	InitializeObjectAttributes(&oa, &voldevext->DE_FILTER.PhysicalDeviceName, OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, NULL);
	//打开驱动器
	status = IoCreateFileSpecifyDeviceObjectHint(
		&hFile, 
		GENERIC_READ | SYNCHRONIZE,
		&oa, 
		&ioStatus, 
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
		fsdevext->DE_FILTER.PhysicalDeviceObject
		);
	if (NT_SUCCESS(status)) {
		FILE_FS_SIZE_INFORMATION fssizeInfo;
		IO_STATUS_BLOCK IoStatusBlock;
		//释放位图
		LCXLBitmapFina(&voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BitmapRedirect);
		LCXLBitmapFina(&voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BitmapDirect);
		LCXLBitmapFina(&voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BitmapUsed);
		//先获取簇(分配单元)信息//获取磁盘卷信息
		status = ZwQueryVolumeInformationFile(hFile,
			&IoStatusBlock,
			&fssizeInfo,
			sizeof(fssizeInfo),
			FileFsSizeInformation);
		if (NT_SUCCESS(status)) {
			STARTING_LCN_INPUT_BUFFER StartingLCN;
			ULONG	BitmapSize;
			PVOLUME_BITMAP_BUFFER BitmapInUse;

			KdPrint(("SYS:VGetFSInformation:fssizeInfo.TotalAllocationUnits=%I64d\n", fssizeInfo.TotalAllocationUnits.QuadPart));
			//下一空闲簇的寻找位置默认为0
			voldevext->DE_FILTER.DE_DISKVOL.FsSetting.LastScanIndex = 0;
			// 初始化重定向表
			RtlInitializeGenericTable(&voldevext->DE_FILTER.DE_DISKVOL.FsSetting.RedirectTable, VCompareRoutine, VAllocateRoutine, VFreeRoutine, NULL);
			

			KdPrint(("SYS:VGetFSInformation:fssizeInfo.AvailableAllocationUnits=%I64u\n", fssizeInfo.AvailableAllocationUnits.QuadPart));
			KdPrint(("SYS:VGetFSInformation:fssizeInfo.BytesPerSector=%u\n", fssizeInfo.BytesPerSector));
			KdPrint(("SYS:VGetFSInformation:fssizeInfo.SectorsPerAllocationUnit=%u\n", fssizeInfo.SectorsPerAllocationUnit));
			//获取扇区，分配单元等信息

			voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerSector = fssizeInfo.BytesPerSector;
			voldevext->DE_FILTER.DE_DISKVOL.FsSetting.SectorsPerAllocationUnit = fssizeInfo.SectorsPerAllocationUnit;
			voldevext->DE_FILTER.DE_DISKVOL.FsSetting.TotalAllocationUnits = (ULONGLONG)fssizeInfo.TotalAllocationUnits.QuadPart;
			voldevext->DE_FILTER.DE_DISKVOL.FsSetting.AvailableAllocationUnits = (ULONGLONG)fssizeInfo.AvailableAllocationUnits.QuadPart;
			voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit = fssizeInfo.BytesPerSector*fssizeInfo.SectorsPerAllocationUnit;

			//计算卷的簇位图大小
			BitmapSize = (ULONG)(fssizeInfo.TotalAllocationUnits.QuadPart/8+sizeof(VOLUME_BITMAP_BUFFER));
			KdPrint(("SYS:VGetFSInformation:BitmapSize = %d\n", BitmapSize));
			StartingLCN.StartingLcn.QuadPart = 0;
			//获取卷位图
			do  {
				BitmapInUse = (PVOLUME_BITMAP_BUFFER)ExAllocatePoolWithTag(PagedPool, BitmapSize, 'BMPS');
				// 内存不够用
				if (BitmapInUse==NULL) {
					//LCXL，返回错误信息
					status = STATUS_INSUFFICIENT_RESOURCES;
					break;
				}
				//LCXL，获得卷位示图
				status = ZwFsControlFile( hFile, 
					NULL, 
					NULL, 
					NULL, 
					&IoStatusBlock, 
					FSCTL_GET_VOLUME_BITMAP, 
					&StartingLCN,
					sizeof (StartingLCN),
					BitmapInUse, 
					BitmapSize
					);
				//LCXL，如果申请的缓冲区装不下位示图
				if (STATUS_BUFFER_OVERFLOW == status) {
					//理论上应该是不会出现STATUS_BUFFER_OVERFLOW的情况
					ASSERT(0);
					//LCXL，释放缓冲区，重新申请更大的缓冲区
					ExFreePoolWithTag(BitmapInUse, 'BMPS');
					//8192
					BitmapSize += 1<<13;
					KdPrint(("SYS:VGetFSInformation:ZwFsControlFile:BitmapSize is too small,realloc memory(%u)\n", BitmapSize));
				}
			} while(STATUS_BUFFER_OVERFLOW == status);
			//成功了
			if (NT_SUCCESS(status)) {
				KdPrint(("SYS:VGetFSInformation:BitmapInUse=%I64d, IoStatusBlock.Information=%d\n", BitmapInUse->BitmapSize.QuadPart, IoStatusBlock.Information));
				//获取数据区扇区偏移量
				status = FGetFirstDataSectorOffset(hFile, &voldevext->DE_FILTER.DE_DISKVOL.FsSetting.DataClusterOffset);
				if (NT_SUCCESS(status)) {
					KdPrint(("SYS:VGetFSInformation:FGetFirstDataSectorOffset=%I64d\n", voldevext->DE_FILTER.DE_DISKVOL.FsSetting.DataClusterOffset));
					//获取数据簇的偏移量
					voldevext->DE_FILTER.DE_DISKVOL.FsSetting.DataClusterOffset = 
						voldevext->DE_FILTER.DE_DISKVOL.FsSetting.DataClusterOffset/fssizeInfo.SectorsPerAllocationUnit
						+(voldevext->DE_FILTER.DE_DISKVOL.FsSetting.DataClusterOffset%fssizeInfo.SectorsPerAllocationUnit>0);
					//通过Bitmap创建LCXL位图
					LCXLBitmapCreateFromBitmap(BitmapInUse->Buffer, (ULONGLONG)BitmapInUse->BitmapSize.QuadPart, voldevext->DE_FILTER.DE_DISKVOL.FsSetting.DataClusterOffset, 2<<20, &voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BitmapUsed);
					//数据簇之前的都标志为已使用
					LCXLBitmapSet(&voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BitmapUsed, 0, voldevext->DE_FILTER.DE_DISKVOL.FsSetting.DataClusterOffset, TRUE);
					//设置已经扫描到数据簇的位置
					voldevext->DE_FILTER.DE_DISKVOL.FsSetting.LastScanIndex = voldevext->DE_FILTER.DE_DISKVOL.FsSetting.DataClusterOffset;
					//初始化默认的重定向位图颗粒为2M
					LCXLBitmapInit(&voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BitmapRedirect, voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BitmapUsed.BitmapSize, 2<<20);
					//初始化默认的直接读写位图颗粒为2M
					LCXLBitmapInit(&voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BitmapDirect, voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BitmapUsed.BitmapSize, 2<<20);
					
					//如果是系统盘
					if (g_DriverSetting.SettingDO== FileSyeFilterDO) {
						UNICODE_STRING FilePath;

						RtlInitUnicodeString(&FilePath, L"\\Windows\\bootstat.dat");
						//下面是直接读写的文件
						FSetDirectRWFile(
							fsdevext, 
							&FilePath,
							voldevext->DE_FILTER.DE_DISKVOL.FsSetting.DataClusterOffset,
							&voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BitmapDirect
							);
					}
				}
				ExFreePool(BitmapInUse);
			} else {
				KdPrint(("SYS:VGetFSInformation:ZwFsControlFile:Fail!0x08x\n", status));
			}
		}//ZwQueryVolumeInformationFile
		//关闭文件
		ZwClose(hFile);
	}//IoCreateFileSpecifyDeviceObjectHint
	if (!NT_SUCCESS(status)){
		KdPrint(("SYS(%d):VGetFSInformation (%wZ) is Fail!0x%08x\n", PsGetCurrentProcessId(), &voldevext->DE_FILTER.PhysicalDeviceName, status));
		voldevext->DE_FILTER.DE_DISKVOL.IsProtect = FALSE;
	} else {
		ULONG i;
		//设置所在磁盘为重定向保护模式
		for (i = 0; i < voldevext->DE_FILTER.DE_DISKVOL.DiskFilterNum; i++) {
			PEP_DEVICE_EXTENSION diskdevext;

			diskdevext = (PEP_DEVICE_EXTENSION)voldevext->DE_FILTER.DE_DISKVOL.DiskFilterDO[i]->DeviceExtension;
			diskdevext->DE_FILTER.DE_DISK.InitializeStatus = DIS_MEMORY;
		}
	}
	voldevext->DE_FILTER.DE_DISKVOL.InitializeStatus = DVIS_INITIALIZED;
	//激活事件,通知读写线程可以把
	KeSetEvent(&voldevext->DE_FILTER.DE_DISKVOL.RequestEvent, 0, FALSE);
	
	return status;
}

NTSTATUS VPostProtectedFSInfo(IN PEP_DEVICE_EXTENSION VolDevExt)
{
	NTSTATUS localstatus;

	PAGED_CODE();

	do {
		//等待系统初始化完成
		//LCXLChangeDriverIcon需要等待系统初始化完成才能执行成功，因此需要等待系统初始化完成事件
		KeWaitForSingleObject(&g_SysInitEvent, UserRequest, KernelMode, FALSE, NULL);
		//更换图标
		localstatus = LCXLChangeDriverIcon(VolDevExt->DE_FILTER.PhysicalDeviceObject);
		if (NT_SUCCESS(localstatus)) {
			KdPrint(("SYS:VGetFSInfoThread:Change driver(%wZ)icon succeed!\n", &VolDevExt->DE_FILTER.PhysicalDeviceName));
		} else {
			KdPrint(("SYS:VGetFSInfoThread:Change driver(%wZ)icon failed!(0x%08x)\n", &VolDevExt->DE_FILTER.PhysicalDeviceName, localstatus));
			//如果是这个错误，则是表明注册表还没有初始化完成//Windows Vista在BootReinitilize之后还没有初始化此处的注册表
			if (localstatus == STATUS_OBJECT_NAME_NOT_FOUND) {
				LARGE_INTEGER sleeptime;
				//注册表这个键值无法访问？重新注册驱动初始化函数
				KeClearEvent(&g_SysInitEvent);
				//先等待3s，再去获取，不然。。。额。。。Windows XP下会循环很多遍
				sleeptime.QuadPart = -30000000;//3s
				KeDelayExecutionThread(KernelMode, FALSE, &sleeptime);
				//重新注册
				IoRegisterDriverReinitialization(g_CDO->DriverObject, LCXLDriverReinitialize, NULL);
				KdPrint(("SYS:VGetFSInfoThread(%wZ):Register Boot Reinitilize roution again\n", &VolDevExt->DE_FILTER.PhysicalDeviceName));
			}
		}
	} while (localstatus == STATUS_OBJECT_NAME_NOT_FOUND);
	return localstatus;
}

VOID VGetDriverSettingThread(
	IN PVOID Context
	)
{
	NTSTATUS status;
	PDEVICE_OBJECT FileSyeFilterDO;
	
	PAGED_CODE();
	//设置这个线程的优先级为高实时性
	KeSetPriorityThread(KeGetCurrentThread(), HIGH_PRIORITY);
	FileSyeFilterDO = (PDEVICE_OBJECT)Context;

	status = LCXLGetDriverSetting(FileSyeFilterDO);
	if (NT_SUCCESS(status)) {
		
	} else {
		KdPrint(("SYS:VGetFSInformation:LCXLGetDriverSetting failed(0x%08x)\n", status));
	}
	//结束线程
	PsTerminateSystemThread(status);
}

VOID VGetFSInfoThread(
	IN PVOID Context
	)
{
	NTSTATUS status;
	PDEVICE_OBJECT FileSyeFilterDO;
	PDEVICE_OBJECT DiskVolFilterDO;
	PEP_DEVICE_EXTENSION FsDevExt;
	PEP_DEVICE_EXTENSION VolDevExt;
	PAGED_CODE();
	//设置这个线程的优先级为高实时性
	KeSetPriorityThread(KeGetCurrentThread(), HIGH_PRIORITY);
	FileSyeFilterDO = (PDEVICE_OBJECT)Context;
	FsDevExt = (PEP_DEVICE_EXTENSION)FileSyeFilterDO->DeviceExtension;
	DiskVolFilterDO = FsDevExt->DE_FILTER.DE_FSVOL.DiskVolFilter;
	VolDevExt = (PEP_DEVICE_EXTENSION)DiskVolFilterDO->DeviceExtension;
	KdPrint(("SYS:VGetFSInfoThread(%wZ)\n", &VolDevExt->DE_FILTER.PhysicalDeviceName));
	//等待设置获取完成
	KeWaitForSingleObject(&g_DriverSetting.SettingInitEvent, UserRequest, KernelMode, FALSE, NULL);
	
	//开始获取文件系统设置
	status = VGetFSInformation(DiskVolFilterDO);
	if (!NT_SUCCESS(status)) {
		KdPrint(("SYS:VGetFSInfoThread:VGetFSInformation(%wZ)failed.(0x%08x)\n", &VolDevExt->DE_FILTER.PhysicalDeviceName, status));
	}
	//如果需要保护
	if (VolDevExt->DE_FILTER.DE_DISKVOL.IsProtect) {
		VPostProtectedFSInfo(VolDevExt);
	}
	KdPrint(("SYS:VGetFSInfoThread(%wZ) ended\n", &VolDevExt->DE_FILTER.PhysicalDeviceName));
	//结束线程
	PsTerminateSystemThread(status);
}

//磁盘卷过滤设备挂载事件
VOID VPostAttachDiskVolDevice(
	IN PDEVICE_OBJECT FilterDeviceObject//磁盘卷过滤设备 
	)
{
	PEP_DEVICE_EXTENSION devext;

	PAGED_CODE();
	devext = (PEP_DEVICE_EXTENSION)FilterDeviceObject->DeviceExtension;
	//创建系统线程
	VCreateVolumeThread(FilterDeviceObject);
	KdPrint(("SYS:VPostAttachDiskVolDevice:FilterDeviceObject=%wZ\n", &devext->DE_FILTER.PhysicalDeviceName));
}
//磁盘卷过滤设备卸载挂载事件
VOID VPostDetachDiskVolDevice(
	IN PDEVICE_OBJECT FilterDeviceObject//文件系统过滤设备 
	)
{
	PEP_DEVICE_EXTENSION devext;
	NTSTATUS status;

	PAGED_CODE();
	devext = (PEP_DEVICE_EXTENSION)FilterDeviceObject->DeviceExtension;
	KdPrint(("SYS:VPostDetachDiskVolDevice:FilterDeviceObject=%wZ\n", &devext->DE_FILTER.PhysicalDeviceName));
	
	status = VCloseVolumeThread(FilterDeviceObject);
	if (!NT_SUCCESS(status)) {
		KdPrint(("SYS:VPostDetachDiskVolDevice:VCloseVolumeThread failed(0x%08x)\n"));
	}
}

//使磁盘和磁盘卷过滤设备相互关联
NTSTATUS VAssociateDiskFilter(
	IN PDEVICE_OBJECT DiskVolFilterDO, 
	IN LARGE_INTEGER StartingOffset,
	IN LARGE_INTEGER ExtentLength,
	IN PDEVICE_OBJECT DiskFilterDO
	)
{
	PEP_DEVICE_EXTENSION voldevext;
	PEP_DEVICE_EXTENSION diskdevext;
	NTSTATUS status;

	PAGED_CODE();

	voldevext = (PEP_DEVICE_EXTENSION)DiskVolFilterDO->DeviceExtension;
	diskdevext = (PEP_DEVICE_EXTENSION)DiskFilterDO->DeviceExtension;
	ASSERT(voldevext->DeviceType==DO_DISKVOL);
	ASSERT(diskdevext->DeviceType == DO_DISK);
	
	if (voldevext->DE_FILTER.DE_DISKVOL.DiskFilterNum<64 && diskdevext->DE_FILTER.DE_DISK.DiskVolFilterNum<64) {
		voldevext->DE_FILTER.DE_DISKVOL.DiskFilterDO[voldevext->DE_FILTER.DE_DISKVOL.DiskFilterNum] = DiskFilterDO;
		
		diskdevext->DE_FILTER.DE_DISK.DiskVolFilterDO[diskdevext->DE_FILTER.DE_DISK.DiskVolFilterNum].FilterDO = DiskVolFilterDO;
		diskdevext->DE_FILTER.DE_DISK.DiskVolFilterDO[diskdevext->DE_FILTER.DE_DISK.DiskVolFilterNum].StartingOffset = StartingOffset;
		diskdevext->DE_FILTER.DE_DISK.DiskVolFilterDO[diskdevext->DE_FILTER.DE_DISK.DiskVolFilterNum].ExtentLength = ExtentLength;

		voldevext->DE_FILTER.DE_DISKVOL.DiskFilterNum++;
		diskdevext->DE_FILTER.DE_DISK.DiskVolFilterNum++;
		status = STATUS_SUCCESS;
	} else {
		//溢出
		status = STATUS_BUFFER_OVERFLOW;
	}
	return status;
}

//取消此磁盘卷过滤设备与磁盘过滤设备的所有关联
NTSTATUS VDisassociateDiskFilter(
	IN PDEVICE_OBJECT DiskVolFilterDO
	)
{
	PEP_DEVICE_EXTENSION voldevext;
	ULONG i;
	PAGED_CODE();

	voldevext = (PEP_DEVICE_EXTENSION)DiskVolFilterDO->DeviceExtension;
	ASSERT(voldevext->DeviceType==DO_DISKVOL);
	for (i = 0; i < voldevext->DE_FILTER.DE_DISKVOL.DiskFilterNum; i++) {
		PDEVICE_OBJECT DiskFilterDO;
		PEP_DEVICE_EXTENSION diskdevext;
		ULONG j;

		DiskFilterDO = voldevext->DE_FILTER.DE_DISKVOL.DiskFilterDO[i];
		diskdevext = (PEP_DEVICE_EXTENSION)DiskFilterDO->DeviceExtension;

		if (voldevext->DE_FILTER.DE_DISKVOL.IsProtect) {
			//通知磁盘设备，磁盘上的磁盘卷已经更改
			diskdevext->DE_FILTER.DE_DISK.IsVolumeChanged = TRUE;
		}
		
		for (j = 0; j <diskdevext->DE_FILTER.DE_DISK.DiskVolFilterNum; j++) {
			if (diskdevext->DE_FILTER.DE_DISK.DiskVolFilterDO[j].FilterDO == DiskVolFilterDO) {
				//解除关联
				RtlMoveMemory(&diskdevext->DE_FILTER.DE_DISK.DiskVolFilterDO[j], &diskdevext->DE_FILTER.DE_DISK.DiskVolFilterDO[j+1], sizeof(DISK_VOL_FILTER_EXTENT)*(diskdevext->DE_FILTER.DE_DISK.DiskVolFilterNum-j-1));
				diskdevext->DE_FILTER.DE_DISK.DiskVolFilterNum--;
				break;
			}
		}
	}
	voldevext->DE_FILTER.DE_DISKVOL.DiskFilterNum = 0;
	return STATUS_SUCCESS;
}

static PCHAR MAJOR_FUNCTION_STR[] = {
	"IRP_MJ_CREATE", "IRP_MJ_CREATE_NAMED_PIPE", "IRP_MJ_CLOSE", "IRP_MJ_READ",
	"IRP_MJ_WRITE", "IRP_MJ_QUERY_INFORMATION", "IRP_MJ_SET_INFORMATION", "IRP_MJ_QUERY_EA",
	"IRP_MJ_SET_EA", "IRP_MJ_FLUSH_BUFFERS", "IRP_MJ_QUERY_VOLUME_INFORMATION", "IRP_MJ_SET_VOLUME_INFORMATION",
	"IRP_MJ_DIRECTORY_CONTROL", "IRP_MJ_FILE_SYSTEM_CONTROL", "IRP_MJ_DEVICE_CONTROL", "IRP_MJ_INTERNAL_DEVICE_CONTROL",
	"IRP_MJ_SHUTDOWN", "IRP_MJ_LOCK_CONTROL", "IRP_MJ_CLEANUP", "IRP_MJ_CREATE_MAILSLOT", 
	"IRP_MJ_QUERY_SECURITY", "IRP_MJ_SET_SECURITY", "IRP_MJ_POWER", "IRP_MJ_SYSTEM_CONTROL",
	"IRP_MJ_DEVICE_CHANGE", "IRP_MJ_QUERY_QUOTA", "IRP_MJ_SET_QUOTA", "IRP_MJ_PNP",
};

//inline一下
__inline NTSTATUS VDeviceReadWrite (
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	)
{
	PEP_DEVICE_EXTENSION devext = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	if (devext->DE_FILTER.DE_DISKVOL.InitializeStatus==DVIS_SAVINGDATA) {
		KdPrint(("SYS:VDeviceReadWrite:Another process read/write disk when %wZ is saving data.\n", &devext->DE_FILTER.PhysicalDeviceName));
	}
	//将IRP设置为等待状态
	IoMarkIrpPending(Irp);
	//然后将这个irp放进读写请求队列里
	ExInterlockedInsertTailList(
		&devext->DE_FILTER.DE_DISKVOL.RWListHead,
		&Irp->Tail.Overlay.ListEntry,
		&devext->DE_FILTER.DE_DISKVOL.RequestListLock
		);
	//激活队列的等待事件，通知队列对这个irp进行处理
	KeSetEvent(&devext->DE_FILTER.DE_DISKVOL.RequestEvent, (KPRIORITY)0, FALSE);
	//返回pending状态，这个irp就算部分处理完了
	return STATUS_PENDING;
};

NTSTATUS VDeviceRead(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	)
{
	PAGED_CODE();

	return VDeviceReadWrite(DeviceObject, Irp);
}

NTSTATUS VDeviceWrite(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	)
{
	PAGED_CODE();
	
	return VDeviceReadWrite(DeviceObject, Irp);
}

NTSTATUS VDevicePnp(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	)
{
	PEP_DEVICE_EXTENSION devext = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	NTSTATUS status;
	PIO_STACK_LOCATION isp;
	BOOLEAN IsLocalVolume;//1.是否是本地卷，而不是在移动磁盘中的卷，或者是影子模式下新增的磁盘卷
		
	PAGED_CODE();
	
	isp = IoGetCurrentIrpStackLocation( Irp );
	switch(isp->MinorFunction) 
	{
	case IRP_MN_START_DEVICE:
		KdPrint(("SYS:VDevicePnp:IRP_MN_START_DEVICE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		status = LLWaitIRPCompletion(devext->DE_FILTER.LowerDeviceObject, Irp);
		
		IsLocalVolume = FALSE;
		//成功启动了，
		if (NT_SUCCESS(status)) {
			PVOLUME_DISK_EXTENTS VolumeDiskExtents;//磁盘信息
			ULONG VolumeDiskExtentsSize;

			VolumeDiskExtentsSize = sizeof(VOLUME_DISK_EXTENTS ) + ( 32 - 1) * sizeof(DISK_EXTENT );
			VolumeDiskExtents = (PVOLUME_DISK_EXTENTS)ExAllocatePoolWithTag(PagedPool, VolumeDiskExtentsSize, 'DEXT'); 
			if (VolumeDiskExtents != NULL) {
				KEVENT event;
				PIRP QueryIrp;
				NTSTATUS localstatus;
				IO_STATUS_BLOCK iostatus;

				KeInitializeEvent(&event, NotificationEvent, FALSE);

				//
				// 查询卷所对应的磁盘
				//
				QueryIrp = IoBuildDeviceIoControlRequest(
					IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
					devext->DE_FILTER.LowerDeviceObject,
					NULL,
					0,
					VolumeDiskExtents,
					VolumeDiskExtentsSize,
					FALSE,
					&event,
					&iostatus);
				if (QueryIrp != NULL) {
					localstatus = IoCallDriver(devext->DE_FILTER.LowerDeviceObject, QueryIrp);
					if (localstatus == STATUS_PENDING) {
						KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
						localstatus = iostatus.Status;
					}
					//获取卷信息成功
					if (NT_SUCCESS(localstatus)) {
						ULONG i;
						
						//查看这个卷是否在这个磁盘中
						for (i = 0; i <VolumeDiskExtents->NumberOfDiskExtents; i++) {
							PDEVICE_OBJECT DiskFilterDO;
							

							DiskFilterDO = DeviceObject->DriverObject->DeviceObject;
							while (DiskFilterDO != NULL) {
								PEP_DEVICE_EXTENSION devobjext = NULL;
								devobjext = (PEP_DEVICE_EXTENSION)DiskFilterDO->DeviceExtension;
								//如果是磁盘设备
								if (devobjext->DeviceType == DO_DISK) {
									//找到了这个卷所在的磁盘
									if (VolumeDiskExtents->Extents[i].DiskNumber == devobjext->DE_FILTER.DE_DISK.DiskNumber) {
										break;
									}
								}
								//查询下一个设备
								DiskFilterDO = DiskFilterDO->NextDevice;
							}
							if (DiskFilterDO != NULL) {
								PEP_DEVICE_EXTENSION devobjext;

								devobjext = (PEP_DEVICE_EXTENSION)DiskFilterDO->DeviceExtension;
								ASSERT(devobjext->DeviceType == DO_DISK);
								if (!devobjext->DE_FILTER.DE_DISK.IsVolumeChanged) {
									//关联磁盘过滤驱动
									VAssociateDiskFilter(DeviceObject, VolumeDiskExtents->Extents[i].StartingOffset, VolumeDiskExtents->Extents[i].ExtentLength, DiskFilterDO);
									IsLocalVolume = DIS_LOCAL_DISK(devobjext->DE_FILTER.DE_DISK.BusType);
								} else {
									//这是影子模式下新增的磁盘卷，不能被信任，因此不是本地磁盘卷
									IsLocalVolume = FALSE;
									KdPrint(("SYS: This volume %wZ can't be believed\n", &devext->DE_FILTER.PhysicalDeviceName));
								}
							} else {
								IsLocalVolume = FALSE;
							}
							if (!IsLocalVolume) {
								break;
							}
						}
					} else {
						KdPrint(("SYS:DDevicePnp(%wZ) IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS failed(0x%08x)\n", &devext->DE_FILTER.PhysicalDeviceName, localstatus));
					}
				}
				ExFreePool(VolumeDiskExtents);
			}
		}
		if (!IsLocalVolume) {
			//不是本地卷，则删除过滤设备
			IoDetachDevice(devext->DE_FILTER.LowerDeviceObject);
			//解除与磁盘过滤设备的映射
			VDisassociateDiskFilter(DeviceObject);

			VPostDetachDiskVolDevice(DeviceObject);
			//如果存在过滤设备，就要删除它，这并不是真正的删除，只是减少其引用，所以访问devext是安全的。
			IoDeleteDevice(DeviceObject);
		} else {
			
		}
		return LLCompleteRequest(status, Irp, Irp->IoStatus.Information);
		break;
		
	case IRP_MN_QUERY_REMOVE_DEVICE:
		KdPrint(("SYS:VDevicePnp:IRP_MN_QUERY_REMOVE_DEVICE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_REMOVE_DEVICE:
		KdPrint(("SYS:VDevicePnp:IRP_MN_REMOVE_DEVICE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		/*
		//在这里返回错误值无用，因为分区表已经改了，这个卷相当于已经没有了
		if (devext->DE_FILTER.DE_DISKVOL.IsProtect) {
			//受保护？不能压缩卷
			KdPrint(("SYS:VDevicePnp:IRP_MN_REMOVE_DEVICE(%wZ) is protected, deny to remove\n", &devext->DE_FILTER.PhysicalDeviceName));
			return LLCompleteRequest(STATUS_ACCESS_DENIED, Irp, 0);
		}
		*/
		IoDetachDevice(devext->DE_FILTER.LowerDeviceObject);
		
		//解除与磁盘过滤设备的映射
		VDisassociateDiskFilter(DeviceObject);

		VPostDetachDiskVolDevice(DeviceObject);
		//如果存在过滤设备，就要删除它，这并不是真正的删除，只是减少其引用，所以访问devext是安全的。
		IoDeleteDevice(DeviceObject);
		break;
		/*
	case IRP_MN_CANCEL_REMOVE_DEVICE:
		KdPrint(("SYS:VDevicePnp:IRP_MN_CANCEL_REMOVE_DEVICE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
		*/
	case IRP_MN_STOP_DEVICE:
		KdPrint(("SYS:VDevicePnp:IRP_MN_STOP_DEVICE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		/*
		if (devext->DE_FILTER.DE_DISKVOL.IsProtect) {
			//受保护？不能压缩卷
			KdPrint(("SYS:VDevicePnp:IRP_MN_STOP_DEVICE(%wZ) is protected, deny to remove\n", &devext->DE_FILTER.PhysicalDeviceName));
			return LLCompleteRequest(STATUS_ACCESS_DENIED, Irp, 0);
		}
		*/
		break;
		/*
	case IRP_MN_QUERY_STOP_DEVICE:
		KdPrint(("SYS:VDevicePnp:IRP_MN_QUERY_STOP_DEVICE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_CANCEL_STOP_DEVICE:
		KdPrint(("SYS:VDevicePnp:IRP_MN_CANCEL_STOP_DEVICE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;

	case IRP_MN_QUERY_DEVICE_RELATIONS:
		KdPrint(("SYS:VDevicePnp:IRP_MN_QUERY_DEVICE_RELATIONS-%s(%wZ)\n", qrydevtypestrlst[isp->Parameters.QueryDeviceRelations.Type], &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_INTERFACE:
		KdPrint(("SYS:VDevicePnp:IRP_MN_QUERY_INTERFACE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_CAPABILITIES:
		KdPrint(("SYS:VDevicePnp:IRP_MN_QUERY_CAPABILITIES(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_RESOURCES:
		KdPrint(("SYS:VDevicePnp:IRP_MN_QUERY_RESOURCES(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
		KdPrint(("SYS:VDevicePnp:IRP_MN_QUERY_RESOURCE_REQUIREMENTS(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_DEVICE_TEXT:
		KdPrint(("SYS:VDevicePnp:IRP_MN_QUERY_DEVICE_TEXT(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
		KdPrint(("SYS:VDevicePnp:IRP_MN_FILTER_RESOURCE_REQUIREMENTS(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;

	case IRP_MN_READ_CONFIG:
		KdPrint(("SYS:VDevicePnp:IRP_MN_READ_CONFIG(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_WRITE_CONFIG:
		KdPrint(("SYS:VDevicePnp:IRP_MN_WRITE_CONFIG(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_EJECT:
		KdPrint(("SYS:VDevicePnp:IRP_MN_EJECT(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_SET_LOCK:
		KdPrint(("SYS:VDevicePnp:IRP_MN_SET_LOCK(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_ID:
		KdPrint(("SYS:VDevicePnp:IRP_MN_QUERY_ID(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_PNP_DEVICE_STATE:
		KdPrint(("SYS:VDevicePnp:IRP_MN_QUERY_PNP_DEVICE_STATE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_BUS_INFORMATION:
		KdPrint(("SYS:VDevicePnp:IRP_MN_QUERY_BUS_INFORMATION(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_DEVICE_USAGE_NOTIFICATION:
		KdPrint(("SYS:VDevicePnp:IRP_MN_DEVICE_USAGE_NOTIFICATION(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_SURPRISE_REMOVAL:
		KdPrint(("SYS:VDevicePnp:IRP_MN_SURPRISE_REMOVAL(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
		*/
	}

	return LLSendToNextDriver(devext->DE_FILTER.LowerDeviceObject, Irp);
}

NTSTATUS VDevicePower(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	)
{
	PEP_DEVICE_EXTENSION devExt;
	PIO_STACK_LOCATION irpsp;
	NTSTATUS status;

	PAGED_CODE();

	devExt = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	irpsp = IoGetCurrentIrpStackLocation(Irp);
	if (irpsp->MinorFunction == IRP_MN_SET_POWER) {
		
		switch (irpsp->Parameters.Power.ShutdownType) {
		case PowerActionShutdown://关机
		case PowerActionShutdownOff://关机和关电源
		case PowerActionShutdownReset://重启

			break;
		}
	}
	PoStartNextPowerIrp(Irp);
	IoSkipCurrentIrpStackLocation(Irp);
	status = PoCallDriver(devExt->DE_FILTER.LowerDeviceObject, Irp);
	return status;
}

VOID VWriteMemRedirectDataToDisk(
	IN OUT PEP_DEVICE_EXTENSION voldevext,
	IN OUT PRW_BUFFER_REC RWBuffer,
	IN PVOID ClusterBuffer,
	IN REDIRECT_MODE RedirectMode//重定向模式
	)
{
	PREDIRECT_RW_MEM ListEntry;//重定向表项
	PREDIRECT_RW_MEM TmpListEntry;//重定向表项
#ifdef DBG
	ULONG TotalDataSize = 0;
#endif
	
	PAGED_CODE();
	//必须不能是内存重定向模式，因为要写磁盘
	ASSERT(RedirectMode != RM_IN_MEMORY);

	ListEntry = &voldevext->DE_FILTER.DE_DISKVOL.RedirectMem.RedirectListHead;
	//将写入重定向，并尽可能的合并列表
	for (ListEntry = (PREDIRECT_RW_MEM)ListEntry->ListEntry.Flink; ListEntry != &voldevext->DE_FILTER.DE_DISKVOL.RedirectMem.RedirectListHead;) {
		TmpListEntry = (PREDIRECT_RW_MEM)ListEntry->ListEntry.Flink;
		//摘除自身
		ListEntry->ListEntry.Blink->Flink = ListEntry->ListEntry.Flink;
		ListEntry->ListEntry.Flink->Blink = ListEntry->ListEntry.Blink;

		VReadWriteIrpDispose(voldevext, IRP_MJ_WRITE, ListEntry->buffer, ListEntry->length, ListEntry->offset, RWBuffer, RedirectMode, ClusterBuffer);
#ifdef DBG
		TotalDataSize += ListEntry->length;
#endif
		//释放内存
		FreeRedirectIrpListEntry(ListEntry);
		ListEntry = TmpListEntry;
	}
#ifdef DBG
	DbgPrint("SYS:VWriteMemRedirectDataToDisk:TotalDataSize=%d\n", TotalDataSize);
#endif
}

NTSTATUS VReadWriteIrpDispose(
	IN OUT PEP_DEVICE_EXTENSION voldevext,
	ULONG MajorFunction,
	PVOID buffer,
	ULONG length,
	ULONGLONG offset,

	IN OUT PRW_BUFFER_REC RWBuffer,
	IN REDIRECT_MODE RedirectMode,//重定向模式
	IN PVOID ClusterBuffer
	)
{
	
	NTSTATUS	status;

	PAGED_CODE();

	status = STATUS_SUCCESS;
	//从这里开始，判断是否是在内存中重定向
	switch (RedirectMode) {
	case RM_IN_DISK://在磁盘中重定向
		{
			ULONG				RealLength;
			//第一个簇序号
			ULONGLONG			FirstClusterIndex;
			//未和簇对其的字节数
			ULONG				FirstUnalignedBytes;
			//最后一个簇序号
			ULONGLONG			LastClusterIndex;
			//未和簇对其的字节数
			ULONG				LastUnalignedBytes;
			//簇数量
			ULONG				ClusterNum;

			ASSERT(ClusterBuffer!=NULL);
			//一定是得按扇区大小的倍数读写
			if (length%voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerSector!=0) {
				KdPrint(("SYS:VReadWriteThread:bad length=%d, offset=%I64d, IRP_MJ_WRITE=%d\n", length, offset, MajorFunction));
				return STATUS_UNSUCCESSFUL;
			}
			//获取第一个簇的序号
			FirstClusterIndex = offset/voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit;
			FirstUnalignedBytes = offset%voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit;
			LastClusterIndex = (offset+length)/voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit;
			LastUnalignedBytes = (offset+length)%voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit;
			//获取簇数量
			ClusterNum = (ULONG)(LastClusterIndex-FirstClusterIndex)+(LastUnalignedBytes>0);
			//获取按簇读写的真实长度
			RealLength = ClusterNum*voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit;
			//如果缓冲区长度不够，重新申请内存
			RW_ADJUST_MEM(RWBuffer, RealLength);

			switch (MajorFunction) {
			case IRP_MJ_READ:
				//按簇读写
				status = VHandleReadWrite(
					voldevext, 
					IRP_MJ_READ, 
					offset-FirstUnalignedBytes, 
					RWBuffer->Buffer, 
					RealLength);
				if (NT_SUCCESS(status)) {
					//将读出来的数据中所需要的复制到缓冲区中
					RtlCopyMemory(buffer, &((PBYTE)RWBuffer->Buffer)[FirstUnalignedBytes], length);
				} else {
					length = 0;
				}

				break;
			case IRP_MJ_WRITE:
				status = STATUS_SUCCESS;
				//对于写操作，则需要处理多余的事情
				RtlCopyMemory(&((PBYTE)RWBuffer->Buffer)[FirstUnalignedBytes], buffer, length);
				//如果数据的头部不按簇对齐，则先读取这个簇的全部数据，然后复制到缓冲区中组成按簇对其的数据
				if (FirstUnalignedBytes>0) {
					
					//读取这一簇的数据
					status = VHandleReadWrite(
						voldevext, 
						IRP_MJ_READ, 
						offset-FirstUnalignedBytes, 
						ClusterBuffer, 
						voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit
						);
					if (NT_SUCCESS(status)) {
						//将读出来的数据中所需要的复制到缓冲区中
						RtlCopyMemory(RWBuffer->Buffer, ClusterBuffer, FirstUnalignedBytes);
					} else {
						length = 0;
					}
				}
				//如果数据尾部不按簇不对齐，则读取尾部整个簇的数据
				if (LastUnalignedBytes>0) {
					//这种情况需要读取磁盘
					if (FirstUnalignedBytes==0||ClusterNum > 1) {
						//读取最后一簇的数据
						status = VHandleReadWrite(
							voldevext, 
							IRP_MJ_READ, 
							offset+length-LastUnalignedBytes, 
							ClusterBuffer, 
							voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit
							);
					}
					if (NT_SUCCESS(status)) {
						//将读出来的数据中所需要的复制到缓冲区中
						RtlCopyMemory(&((PBYTE)RWBuffer->Buffer)[FirstUnalignedBytes+length], ClusterBuffer, voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit-LastUnalignedBytes);
					} else {
						length = 0;
					}
				}
				if (NT_SUCCESS(status)) {
					status = VHandleReadWrite(
						voldevext, 
						IRP_MJ_WRITE, 
						offset-FirstUnalignedBytes, 
						RWBuffer->Buffer, 
						RealLength
						);
					if (!NT_SUCCESS(status)) {
						length = 0;
					}
				}
				break;
			default:
				ASSERT(0);
				status = STATUS_UNSUCCESSFUL;
				length = 0;
				break;
			}
		}
		break;
	case RM_IN_MEMORY://在内存中重定向
		{
			IO_STATUS_BLOCK iostatus;

			//如果在内存中重定向
			switch (MajorFunction) {
			case IRP_MJ_READ:
				//判断内存够不够
				RW_ADJUST_MEM(RWBuffer, length);
				status = VDirectReadWriteDiskVolume(voldevext->DE_FILTER.PhysicalDeviceObject, 
					IRP_MJ_READ,
					offset,
					RWBuffer->Buffer,
					length,
					&iostatus,
					TRUE);
				if (NT_SUCCESS(status)) {
					//读取成功

					LCXLReadFromRedirectRWMemList(&voldevext->DE_FILTER.DE_DISKVOL.RedirectMem.RedirectListHead, RWBuffer->Buffer, offset, length);
					RtlCopyMemory(buffer, RWBuffer->Buffer, length);
				}
				break;
			case IRP_MJ_WRITE:
				//内存重定向主要是拦截写的操作
				//也就是到内存重定向结束之后，才开始真正的写入磁盘
				LCXLInsertToRedirectRWMemList(&voldevext->DE_FILTER.DE_DISKVOL.RedirectMem.RedirectListHead, buffer, offset, length);
				status = STATUS_SUCCESS;
				break;
			default:
				ASSERT(0);
				status = STATUS_INVALID_DEVICE_REQUEST;
				length = 0;
				break;
			}
		}
		break;
	case RM_DIRECT://直接读写
		{
			IO_STATUS_BLOCK iostatus;
			//如果缓冲区长度不够，重新申请内存
			RW_ADJUST_MEM(RWBuffer, length);
			switch (MajorFunction) {
			case IRP_MJ_READ:
				//按簇读写

				status = VDirectReadWriteDiskVolume(
					voldevext->DE_FILTER.PhysicalDeviceObject, 
					IRP_MJ_READ, 
					offset, 
					RWBuffer->Buffer, 
					length, 
					&iostatus,
					TRUE);
				if (NT_SUCCESS(status)) {
					//将读出来的数据中所需要的复制到缓冲区中
					RtlCopyMemory(buffer, RWBuffer->Buffer, length);
				} else {
					length = 0;
				}

				break;
			case IRP_MJ_WRITE:
				status = STATUS_SUCCESS;
				//对于写操作，则需要处理多余的事情
				RtlCopyMemory(RWBuffer->Buffer, buffer, length);
				//
				status = VDirectReadWriteDiskVolume(
					voldevext->DE_FILTER.PhysicalDeviceObject, 
					IRP_MJ_WRITE, 
					offset, 
					RWBuffer->Buffer, 
					length, 
					&iostatus,
					TRUE);
				break;
			default:
				ASSERT(0);
				status = STATUS_UNSUCCESSFUL;
				length = 0;
				break;
			}
		}
		break;
	}
	return status;
}

//根据MAPINDEX来查找
PLCXL_TABLE_MAP VFindMapIndex(IN PRTL_GENERIC_TABLE Table, IN ULONGLONG mapIndex)
{
	PVOID RestartKey;
	PLCXL_TABLE_MAP ptr;

	PAGED_CODE();

	RestartKey = NULL;
	//获取重定向表
	for (ptr = (PLCXL_TABLE_MAP)RtlEnumerateGenericTableWithoutSplaying(Table, &RestartKey);
		ptr != NULL;
		ptr = (PLCXL_TABLE_MAP)RtlEnumerateGenericTableWithoutSplaying(Table, &RestartKey)) {
			if (ptr->mapIndex==mapIndex) {
				return ptr;
			}
	}
	return NULL;
}

NTSTATUS VSaveShadowData(PEP_DEVICE_EXTENSION voldevext)
{

	NTSTATUS status;
	PVOID RestartKey;
	PLCXL_TABLE_MAP ptr;
	//重定向表项模拟堆栈
	PLCXL_TABLE_MAP *TableMapStack;
	ULONG			TableMapStackSize;//模拟堆栈的大小
	ULONG			TableMapStackPos;//堆栈位置
	PVOID Buffer;//读写缓冲区
	ULONG BufferLen;//缓冲区长度，也是每个簇的大小
	IO_STATUS_BLOCK iostatus;
	ULONG DataNum;
	ULONG DataI;
	ULONG SavingProgress;

	CHAR strbuf[MAX_PATH];
	PAGED_CODE();
	ASSERT(voldevext != NULL);
	ASSERT(voldevext->DE_FILTER.DE_DISKVOL.IsProtect&&voldevext->DE_FILTER.DE_DISKVOL.IsSaveShadowData);
	status = STATUS_SUCCESS;
	KdPrint(("SYS:VSaveShadowData begin\n"));
	//if (!IS_WINDOWSVISTA_OR_LATER()) {
		RtlStringCbPrintfA(strbuf, sizeof(strbuf), "Disk Volume:%wZ\nPlease wait while getting data information...", &voldevext->DE_FILTER.PhysicalDeviceName);
		InbvDisplayString(strbuf);
	//}
	BufferLen = voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit;
	Buffer = ExAllocatePoolWithTag(PagedPool, BufferLen, 'DATA');
	if (Buffer == NULL) {
		//资源不足
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	DataNum = 0;
	RestartKey = NULL;
	//初始化重定向表中的访问域IsVisited
	for (ptr = (PLCXL_TABLE_MAP)RtlEnumerateGenericTableWithoutSplaying(&voldevext->DE_FILTER.DE_DISKVOL.FsSetting.RedirectTable, &RestartKey);
		ptr != NULL;
		ptr = (PLCXL_TABLE_MAP)RtlEnumerateGenericTableWithoutSplaying(&voldevext->DE_FILTER.DE_DISKVOL.FsSetting.RedirectTable, &RestartKey)) {
			ptr->IsVisited = FALSE;
			DataNum++;
	}
	//if (!IS_WINDOWSVISTA_OR_LATER()) {
		RtlStringCbPrintfA(strbuf, sizeof(strbuf), "OK!Data Number=%d\n0%%-----------------50%%-----------------100%%\n", DataNum);
		InbvDisplayString(strbuf);
	//}
	//初始化进度
	SavingProgress = 0;
	//初始化模拟堆栈
	TableMapStack = NULL;
	TableMapStackSize = 0;
	TableMapStackPos = 0;

	DataI = 0;
	RestartKey = NULL;
	//获取重定向表
	for (ptr = (PLCXL_TABLE_MAP)RtlEnumerateGenericTableWithoutSplaying(&voldevext->DE_FILTER.DE_DISKVOL.FsSetting.RedirectTable, &RestartKey);
		ptr != NULL;
		ptr = (PLCXL_TABLE_MAP)RtlEnumerateGenericTableWithoutSplaying(&voldevext->DE_FILTER.DE_DISKVOL.FsSetting.RedirectTable, &RestartKey)) {
			ULONG j;
			if (ptr->mapIndex != ptr->orgIndex) {
				if (ptr->IsVisited) {
					continue;
				}
				do {
					//压入堆栈中
					if (TableMapStackPos>=TableMapStackSize) {
						PLCXL_TABLE_MAP *tmpstack;

						TableMapStackSize += 1024;
						tmpstack = (PLCXL_TABLE_MAP*)ExAllocatePoolWithTag(PagedPool, TableMapStackSize*sizeof(PLCXL_TABLE_MAP), 'SSSS');
						if (TableMapStackPos!=0) {
							RtlCopyMemory(tmpstack, TableMapStack, TableMapStackPos*sizeof(PLCXL_TABLE_MAP));	
						}
						if (TableMapStack != NULL) {
							ExFreePool(TableMapStack);
						}
						TableMapStack = tmpstack;
						KdPrint(("SAVE TableMapStackPos=%d\n", TableMapStackPos+1));
					}
					//压入堆栈
					TableMapStack[TableMapStackPos] = ptr;
					TableMapStackPos++;
					//查找下一个
					ptr = VFindMapIndex(&voldevext->DE_FILTER.DE_DISKVOL.FsSetting.RedirectTable, ptr->orgIndex);
				} while (ptr != NULL && !ptr->IsVisited);
				for (;TableMapStackPos>0; TableMapStackPos--) {
					//设置为已访问
					ASSERT(TableMapStack[TableMapStackPos-1]!=NULL);
					TableMapStack[TableMapStackPos-1]->IsVisited = TRUE;
					//将重定向的磁盘读写写回到原来的位置
					status = VDirectReadWriteDiskVolume(voldevext->DE_FILTER.PhysicalDeviceObject, IRP_MJ_READ, TableMapStack[TableMapStackPos-1]->mapIndex*BufferLen, Buffer, BufferLen, &iostatus, TRUE);
					if (NT_SUCCESS(status)) {
						status = VDirectReadWriteDiskVolume(voldevext->DE_FILTER.PhysicalDeviceObject, IRP_MJ_WRITE, TableMapStack[TableMapStackPos-1]->orgIndex*BufferLen, Buffer, BufferLen, &iostatus, TRUE);
						if (!NT_SUCCESS(status)) {
							KdPrint(("SYS:VSaveShadowData:VDirectReadWriteDiskVolume(WRITE)Error(0x%08x)\n", status));
						}
					} else {
						KdPrint(("SYS:VSaveShadowData:VDirectReadWriteDiskVolume(READ)Error(0x%08x)\n", status));
					}
					DataI++;
				}
				
			} else {
				if (!ptr->IsVisited) {
					DataI++;
				}
			}
			//更新进度
			
			j = (ULONG)((ULONGLONG)DataI*40/DataNum);
			if (SavingProgress < j) {
				for (; SavingProgress < j; SavingProgress++) {
					//if (!IS_WINDOWSVISTA_OR_LATER()) {
						InbvDisplayString(".");
					//}
				}
			}
			//----
	}
	if (TableMapStack != NULL) {
		ExFreePool(TableMapStack);
	}
	ExFreePool(Buffer);
	InbvDisplayString("OK!\n\n");
	return status;
}

//磁盘卷读写线程
VOID VReadWriteThread (
	IN PVOID Context//线程上下文，这里是磁盘卷过滤驱动对象
	)
{
	//设备对象指针
	PDEVICE_OBJECT DeviceObject;
	//设备扩展
	PEP_DEVICE_EXTENSION voldevext;
	RW_BUFFER_REC RWBuffer;//读写缓冲区结构
	PVOID ClusterBuffer;//簇大小的缓冲区
	//重定向模式
	REDIRECT_MODE		RedirectMode;

	PAGED_CODE();
	//设置这个线程的优先级为低实时性
	KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);

	DeviceObject = (PDEVICE_OBJECT)Context;
	voldevext = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	ASSERT(voldevext->DeviceType == DO_DISKVOL);

	RedirectMode = RM_DIRECT;
	RW_INIT_MEM(&RWBuffer);
	RW_ADJUST_MEM(&RWBuffer, DefaultReadWriteBufferSize);
	if (RWBuffer.Buffer == NULL) {
		KdPrint(("SYS:!!!!!!!!!!!!!!!!VReadWriteThread(%wZ)ReadWriteBuffer=NULL\n", &voldevext->DE_FILTER.PhysicalDeviceName));
		PsTerminateSystemThread(STATUS_INSUFFICIENT_RESOURCES);
	}
	ClusterBuffer = NULL;
	//如果线程结束标志为TRUE，则退出
	while (!voldevext->DE_FILTER.DE_DISKVOL.ToTerminateThread) {
		//请求队列的入口
		PLIST_ENTRY	ReqEntry;
		
		//先等待请求队列同步事件，如果队列中没有irp需要处理，我们的线程就等待在这里，让出cpu时间给其它线程
		KeWaitForSingleObject(
			&voldevext->DE_FILTER.DE_DISKVOL.RequestEvent,
			Executive,
			KernelMode,
			FALSE,
			NULL
			);
		switch (voldevext->DE_FILTER.DE_DISKVOL.InitializeStatus) {
		case DVIS_INITIALIZED:
			voldevext->DE_FILTER.DE_DISKVOL.InitializeStatus = DVIS_WORKING;
			//开始将重定向内存的数据写入到磁盘中
			if (voldevext->DE_FILTER.DE_DISKVOL.IsProtect) {
				ClusterBuffer = ExAllocatePoolWithTag(PagedPool, voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit, MEM_TAG_RWBUFFER);
				RedirectMode = RM_IN_DISK;
			} else {
				RedirectMode = RM_DIRECT;
			}
			VWriteMemRedirectDataToDisk(voldevext, &RWBuffer, ClusterBuffer, RedirectMode);
			break;
			//保存数据模式
		case DVIS_SAVINGDATA:
			VSaveShadowData(voldevext);
			//写入数据后不受保护
			voldevext->DE_FILTER.DE_DISKVOL.IsProtect = FALSE;
			//设置为直接读写模式
			RedirectMode = RM_DIRECT;
			//重新设置为工作状态
			voldevext->DE_FILTER.DE_DISKVOL.InitializeStatus = DVIS_WORKING;
			//激活完成事件
			KeSetEvent(&voldevext->DE_FILTER.DE_DISKVOL.SaveShadowDataFinishEvent, 0, FALSE);
			break;
		}
		//从请求队列的首部拿出一个请求来准备处理，这里使用了自旋锁机制，所以不会有冲突
		while (ReqEntry = ExInterlockedRemoveHeadList(&voldevext->DE_FILTER.DE_DISKVOL.RWListHead, &voldevext->DE_FILTER.DE_DISKVOL.RequestListLock), ReqEntry != NULL) {
			NTSTATUS			status;
			//irp指针
			PIRP				Irp;
			//irp中的数据长度
			ULONG				length;
			//irp要处理的偏移量
			ULONGLONG			offset;
			//irp中包括的读写数据缓冲区地址
			PVOID				buffer;
			//IRP堆栈
			PIO_STACK_LOCATION	IrpSp;

			//从队列的入口里找到实际的irp的地址
			Irp = CONTAINING_RECORD(ReqEntry, IRP, Tail.Overlay.ListEntry);
			IrpSp = IoGetCurrentIrpStackLocation(Irp);
			switch (IrpSp->MajorFunction) {
			case IRP_MJ_READ:
				//如果是读的irp请求，我们在irp stack中取得相应的参数作为offset和length
				offset = (ULONGLONG)IrpSp->Parameters.Read.ByteOffset.QuadPart;
				length = IrpSp->Parameters.Read.Length;
				break;
			case IRP_MJ_WRITE:
				//如果是写的irp请求，我们在irp stack中取得相应的参数作为offset和length
				offset = (ULONGLONG)IrpSp->Parameters.Write.ByteOffset.QuadPart;
				length = IrpSp->Parameters.Write.Length;				
				break;
			default:
				//对于其他请求，是不应该有的
				ASSERT(0);
				offset = 0;
				length = 0;
				break;
			}
			if (Irp->MdlAddress) {
				buffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
			}else if (Irp->UserBuffer) {
				buffer = Irp->UserBuffer;
			} else {
				buffer = Irp->AssociatedIrp.SystemBuffer;
			}
			
			//判断目前获取设置和位图的状态
			switch (voldevext->DE_FILTER.DE_DISKVOL.InitializeStatus) {
			case DVIS_NONE:	
			case DVIS_INITIALIZING:
				//没有或正在初始化
				status = VReadWriteIrpDispose(voldevext, IrpSp->MajorFunction, buffer, length, offset, &RWBuffer, RM_IN_MEMORY, ClusterBuffer);
				break;
			case DVIS_INITIALIZED:
				voldevext->DE_FILTER.DE_DISKVOL.InitializeStatus = DVIS_WORKING;
				//开始将重定向内存的数据写入到磁盘中
				if (voldevext->DE_FILTER.DE_DISKVOL.IsProtect) {
					ClusterBuffer = ExAllocatePoolWithTag(PagedPool, voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit, MEM_TAG_RWBUFFER);
					RedirectMode = RM_IN_DISK;
				} else {
					RedirectMode = RM_DIRECT;
				}
				VWriteMemRedirectDataToDisk(voldevext, &RWBuffer, ClusterBuffer, RedirectMode);
				status = VReadWriteIrpDispose(voldevext, IrpSp->MajorFunction, buffer, length, offset, &RWBuffer, RedirectMode, ClusterBuffer);
				break;
			case DVIS_WORKING:
				status = VReadWriteIrpDispose(voldevext, IrpSp->MajorFunction, buffer, length, offset, &RWBuffer, RedirectMode, ClusterBuffer);
				break;
			case DVIS_SAVINGDATA:
				VSaveShadowData(voldevext);
				//写入数据后不受保护
				voldevext->DE_FILTER.DE_DISKVOL.IsProtect = FALSE;
				//设置为直接读写模式
				RedirectMode = RM_DIRECT;
				//重新设置为工作状态
				voldevext->DE_FILTER.DE_DISKVOL.InitializeStatus = DVIS_WORKING;
				//激活完成事件
				KeSetEvent(&voldevext->DE_FILTER.DE_DISKVOL.SaveShadowDataFinishEvent, 0, FALSE);

				status = VReadWriteIrpDispose(voldevext, IrpSp->MajorFunction, buffer, length, offset, &RWBuffer, RedirectMode, ClusterBuffer);
				break;
			default:
				status = STATUS_INVALID_DEVICE_REQUEST;
				ASSERT(0);
				break;
			}
			if (!NT_SUCCESS(status)) {
				length = 0;	
			}
			LLCompleteRequest(status, Irp, length);
		}
		//查看是否有用户请求
		while (ReqEntry = ExInterlockedRemoveHeadList(&voldevext->DE_FILTER.DE_DISKVOL.AppRequsetListHead.ListEntry, &voldevext->DE_FILTER.DE_DISKVOL.RequestListLock), ReqEntry != NULL) {
			PAPP_REQUEST_LIST_ENTRY  ARListEntrt;

			ARListEntrt = (PAPP_REQUEST_LIST_ENTRY)ReqEntry;
			switch (ARListEntrt->AppRequest.RequestType) {
			case AR_VOLUME_INFO://获取卷信息
				ARListEntrt->AppRequest.VolumeInfo.AvailableAllocationUnits = voldevext->DE_FILTER.DE_DISKVOL.FsSetting.AvailableAllocationUnits;
				ARListEntrt->AppRequest.VolumeInfo.BytesPerAllocationUnit = voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit;
				ARListEntrt->AppRequest.VolumeInfo.BytesPerSector = voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerSector;
				ARListEntrt->AppRequest.VolumeInfo.SectorsPerAllocationUnit = voldevext->DE_FILTER.DE_DISKVOL.FsSetting.SectorsPerAllocationUnit;
				ARListEntrt->AppRequest.VolumeInfo.TotalAllocationUnits = voldevext->DE_FILTER.DE_DISKVOL.FsSetting.TotalAllocationUnits;
				KdPrint(("SYS:VReadWriteThread:AR_VOLUME_INFO(%wZ) AvailableAllocationUnits=%I64u, BytesPerAllocationUnit=%u, BytesPerSector=%u, SectorsPerAllocationUnit=%u, TotalAllocationUnits=%I64u\n", 
					&voldevext->DE_FILTER.PhysicalDeviceName,
					ARListEntrt->AppRequest.VolumeInfo.AvailableAllocationUnits,
					ARListEntrt->AppRequest.VolumeInfo.BytesPerAllocationUnit,
					ARListEntrt->AppRequest.VolumeInfo.BytesPerSector,
					ARListEntrt->AppRequest.VolumeInfo.SectorsPerAllocationUnit,
					ARListEntrt->AppRequest.VolumeInfo.TotalAllocationUnits));
				ARListEntrt->status = STATUS_SUCCESS;
				break;
			default:
				//错误的驱动的请求
				ARListEntrt->status = STATUS_INVALID_DEVICE_REQUEST;
			}
			//激活事件
			KeSetEvent(&ARListEntrt->FiniEvent, 0, FALSE);
		}
	}
	KdPrint(("SYS:VReadWriteThread go to terminate(%wZ)\n", &voldevext->DE_FILTER.PhysicalDeviceName));
	if (ClusterBuffer!=NULL) {
		ExFreePoolWithTag(ClusterBuffer, MEM_TAG_RWBUFFER);
	}
	RW_FREE_MEM(&RWBuffer);
	PsTerminateSystemThread(STATUS_SUCCESS);
}

NTSTATUS VHandleReadWrite(
	PEP_DEVICE_EXTENSION VolDevExt,
	IN ULONG MajorFunction,//主功能号
	IN ULONGLONG ByteOffset,//偏移量
	IN OUT PVOID Buffer,//要读取/写入的缓冲区，其数据是簇对齐的
	IN ULONG Length//缓冲区长度
	)
{
	//最后两个参数是可以算出来的，因为前面的操作已经计算过了，所以直接过来用
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	//
	IO_STATUS_BLOCK iostatus;
	//当前偏移量
	ULONGLONG CurByteOffset;
	//当前的长度
	ULONG CurLength;
	//下面是为了连续处理，加快速度
	//连续的数据的偏移量
	ULONGLONG SeriateByteOffset;
	//连续的数据的长度
	ULONG SeriateLength;
	//真正要读写的偏移量
	ULONGLONG RealByteOffset;
	//Buffer偏移指针
	PBYTE Bufpos;

	PAGED_CODE();

	CurLength = Length;
	CurByteOffset = ByteOffset;

	SeriateLength = 0;
	SeriateByteOffset = 0;
	Bufpos = (PBYTE)Buffer;
	while (CurLength > 0) {
		//获得簇序号
		RealByteOffset = VGetRealCluster(VolDevExt, CurByteOffset/VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit, MajorFunction==IRP_MJ_READ);
		
		if (RealByteOffset == GET_CLUSTER_RESULT_NO_SPACE||RealByteOffset == GET_CLUSTER_RESULT_OUT_OF_BOUND) {
			//没有空间
			return STATUS_INVALID_PARAMETER;
		}
		//这个是真实的偏移量
		RealByteOffset *= VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit;
		//如果连续的簇数量为空，则写入当前的真实簇
		if (SeriateLength==0) {
			SeriateByteOffset = RealByteOffset;
			SeriateLength = VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit;
		} else {
			//判断当前的真实簇是否和连续的簇连续
			if (SeriateByteOffset+SeriateLength==RealByteOffset) {
				//连续
				SeriateLength+=VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit;
			} else {
				//不连续，先将之前的连续簇写入磁盘中
				//直接磁盘写入
				status = VDirectReadWriteDiskVolume(VolDevExt->DE_FILTER.PhysicalDeviceObject, 
					MajorFunction,
					SeriateByteOffset,
					Bufpos,
					SeriateLength,
					&iostatus,
					TRUE);
				if (!NT_SUCCESS(status)) {
					KdPrint(("SYS:VHandleReadWrite:VDirectReadWriteDiskVolume(%wZ)Error.(0x%08x)\n", &VolDevExt->DE_FILTER.PhysicalDeviceName, status));
					VolDevExt->DE_FILTER.DE_DISKVOL.IsRWError = TRUE;
					return status;
				}
				//指针向后移动
				Bufpos+=SeriateLength;
				//重新设置连续簇的偏移量和大小
				SeriateByteOffset = RealByteOffset;
				SeriateLength = VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit;
			}
		}
		
		CurLength-=VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit;
		CurByteOffset+=VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit;
	}
	//结尾操作，如果连续簇还有
	if (SeriateLength>0) {
		//直接磁盘写入
		status = VDirectReadWriteDiskVolume(VolDevExt->DE_FILTER.PhysicalDeviceObject, 
			MajorFunction,
			SeriateByteOffset,
			Bufpos,
			SeriateLength,
			&iostatus,
			TRUE);
		if (!NT_SUCCESS(status)) {
			KdPrint(("SYS:VHandleReadWrite:VDirectReadWriteDiskVolume(%wZ)Error.(0x%08x)\n", &VolDevExt->DE_FILTER.PhysicalDeviceName, status));
			VolDevExt->DE_FILTER.DE_DISKVOL.IsRWError = TRUE;
			return status;
		}
		//指针向后移动
		Bufpos+=SeriateLength;
		SeriateLength = 0;
		
	} else {
		ASSERT(0);
	}
	ASSERT((ULONG_PTR)Bufpos-(ULONG_PTR)Buffer == Length);
	//完成
	return status;
}

ULONGLONG VGetRealCluster(
	PEP_DEVICE_EXTENSION VolDevExt, 
	ULONGLONG ClusterIndex, 
	BOOLEAN IsReadOpt
	)
{
	//如果簇列表超出范围
	if (ClusterIndex >= VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.BitmapUsed.BitmapSize) {
		KdPrint(("SYS:VGetRealCluster:!!!%I64d>=BitmapSize(%I64d)\n", ClusterIndex, VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.BitmapUsed.BitmapSize));
		return GET_CLUSTER_RESULT_OUT_OF_BOUND;
	} 
	//如果在直接读写位图中找到了这些位图，则返回原始值
	if (LCXLBitmapGet(&VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.BitmapDirect, ClusterIndex)) {
		return ClusterIndex;
	}
	//如果重定向过
	if (LCXLBitmapGet(&VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.BitmapRedirect, ClusterIndex)) {
		PLCXL_TABLE_MAP TableMap;
		LCXL_TABLE_MAP LookupMap;

		LookupMap.orgIndex = ClusterIndex;
		//查找重定向哪里去了
		TableMap = (PLCXL_TABLE_MAP)RtlLookupElementGenericTable(&VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.RedirectTable, &LookupMap);
		if (TableMap!=NULL) {
			//返回重定向之后的序号
			return TableMap->mapIndex;
		} else {
			//不应该找不到的
			ASSERT(0);
			return ClusterIndex;
		}
	} else {
		//如果是读操作
		if (IsReadOpt) {
			return ClusterIndex;
		} else {
			ULONGLONG i;
			//不是读操作，则开始进行重定向工作
			//寻找第一个空闲扇区
			
			i = LCXLBitmapFindFreeBit(&VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.BitmapUsed, VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.LastScanIndex);
			
			//如果找到了
			if (i<(ULONGLONG)VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.BitmapUsed.BitmapSize) {
				LCXL_TABLE_MAP TableMap;
				PVOID p;
				//标记为已使用
				LCXLBitmapSet(&VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.BitmapUsed, i, 1, TRUE);
				//标记为已重定向
				LCXLBitmapSet(&VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.BitmapRedirect, ClusterIndex, 1, TRUE);
				//加入到重定向表中
				TableMap.orgIndex = ClusterIndex;
				TableMap.mapIndex = i;
				p = RtlInsertElementGenericTable(&VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.RedirectTable, &TableMap, sizeof(TableMap), NULL);
				ASSERT(p!=NULL);
				//KdPrint(("R %wZ:%016I64x->%016I64x\n", &VolDevExt->DE_FILTER.PhysicalDeviceName, ClusterIndex, i));
				VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.LastScanIndex = i+1;
				//可用簇减1
				VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.AvailableAllocationUnits--;
				//返回重定向之后的偏移量
				return TableMap.mapIndex;
			} else {
				KdPrint(("SYS:VGetRealCluster:%wZ is full.\n", &VolDevExt->DE_FILTER.PhysicalDeviceName));
				//有错误就无法保存影子磁盘里的文件，怕有问题
				VolDevExt->DE_FILTER.DE_DISKVOL.IsRWError = TRUE;
				VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.LastScanIndex = i;
				//如果没有找到空闲的，则表明已经没有空间了
				return GET_CLUSTER_RESULT_NO_SPACE;
			}
		}
		
	}
}


//////////////////////////////////////////////////////////////////////////
//LCXL
//读写扇区IRP完成例程
//////////////////////////////////////////////////////////////////////////
NTSTATUS VReadWriteSectorsCompletion( 
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp, 
	IN PVOID Context 
	) 
	/*++ 
	Routine Description: 
	A completion routine for use when calling the lower device objects to 
	which our filter deviceobject is attached. 

	Arguments: 

	DeviceObject - Pointer to deviceobject 
	Irp        - Pointer to a PnP Irp. 
	Context    - NULL or PKEVENT 
	Return Value: 

	NT Status is returned. 

	--*/ 
{ 
    PMDL    mdl; 
	
    UNREFERENCED_PARAMETER(DeviceObject); 
	
    // 
    // Free resources 
    // 
	//LCXL，如果SystemBuffer内存存在，则释放
    if (Irp->AssociatedIrp.SystemBuffer && (Irp->Flags & IRP_DEALLOCATE_BUFFER)) { 
        ExFreePool(Irp->AssociatedIrp.SystemBuffer); 
    } 
	//LCXL，如果返回值错误
	if (Irp->IoStatus.Status)
	{
		DbgPrint("!!!!!!!!!!Read Or Write HD Error Code====0x%x\n", Irp->IoStatus.Status);
	}
	/*
	因为这个 IRP 就是在我这层次建立的，上层本就不知道有这么一个 IRP。
	那么到这里我就要在 CompleteRoutine 中使用 IoFreeIrp()来释放掉这个 IRP，
	并返回STATUS_MORE_PROCESSING_REQUIRED不让它继续传递。这里一定要注意，
	在 CompleteRoutine函数返回后，这个 IRP 已经释放了，
	如果这个时候在有任何关于这个 IRP 的操作那么后果是灾难性的，必定导致 BSOD 错误。
	*/
    while (Irp->MdlAddress) { 
        mdl = Irp->MdlAddress; 
        Irp->MdlAddress = mdl->Next; 
        MmUnlockPages(mdl); 
        IoFreeMdl(mdl); 
    } 
	
    if (Irp->PendingReturned && (Context != NULL)) { 
        *Irp->UserIosb = Irp->IoStatus; 
		//LCXL，激活事件
        KeSetEvent((PKEVENT) Context, IO_DISK_INCREMENT, FALSE); 
    } 
	//LCXL，释放IRP
    IoFreeIrp(Irp); 
	
    // 
    // Don't touch irp any more 
    // 
	//LCXL，必须以STATUS_MORE_PROCESSING_REQUIRED作为返回值
    return STATUS_MORE_PROCESSING_REQUIRED; 
} 



NTSTATUS VDirectReadWriteDiskVolume(
	IN PDEVICE_OBJECT DiskVolumeDO,//磁盘卷设备对象 
	IN ULONG MajorFunction,//主功能号
	IN ULONGLONG ByteOffset,//偏移量
	IN OUT PVOID Buffer,//要读取/写入的缓冲区
	IN ULONG Length,//缓冲区长度
	OUT PIO_STATUS_BLOCK iostatus,
	IN BOOLEAN Wait//是否等待
	)
{
	PIRP Irp; 
	NTSTATUS status;

	PAGED_CODE();

	ASSERT(MajorFunction==IRP_MJ_WRITE||MajorFunction==IRP_MJ_READ);
	Irp = IoBuildAsynchronousFsdRequest(MajorFunction, DiskVolumeDO, 
		Buffer, Length, (PLARGE_INTEGER) &ByteOffset, iostatus); 
	if (!Irp) { 
		return STATUS_INSUFFICIENT_RESOURCES; 
	} 
	//LCXL， vista，Win7及以上的操作系统 对直接磁盘写入进行了保护, 驱动操作需要在IRP的FLAGS加上SL_FORCE_DIRECT_WRITE标志
	/*
	If the SL_FORCE_DIRECT_WRITE flag is set, kernel-mode drivers can write to volume areas that they 
	normally cannot write to because of direct write blocking. Direct write blocking was implemented for 
	security reasons in Windows Vista and later operating systems. This flag is checked both at the file 
	system layer and storage stack layer. For more 
	information about direct write blocking, see Blocking Direct Write Operations to Volumes and Disks. 
	The SL_FORCE_DIRECT_WRITE flag is available in Windows Vista and later versions of Windows. 
	http://msdn.microsoft.com/en-us/library/ms795960.aspx
	*/
	if (IRP_MJ_WRITE == MajorFunction) {
		IoGetNextIrpStackLocation(Irp)->Flags |= SL_FORCE_DIRECT_WRITE;
	}
	if (Wait) { 
		KEVENT event; 
		KeInitializeEvent(&event, NotificationEvent, FALSE); 
		IoSetCompletionRoutine(Irp, VReadWriteSectorsCompletion, 
			&event, TRUE, TRUE, TRUE); 

		status = IoCallDriver(DiskVolumeDO, Irp); 
		if (STATUS_PENDING == status) { 
			KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL); 
			status = iostatus->Status; 
		} 
	} else { 
		IoSetCompletionRoutine(Irp, VReadWriteSectorsCompletion, 
			NULL, TRUE, TRUE, TRUE); 
		Irp->UserIosb = NULL; 
		status = IoCallDriver(DiskVolumeDO, Irp); 
	}
	return status;
}

RTL_GENERIC_COMPARE_RESULTS NTAPI VCompareRoutine(
	PRTL_GENERIC_TABLE Table,
	PVOID FirstStruct,
	PVOID SecondStruct
	)
{
	PLCXL_TABLE_MAP first = (PLCXL_TABLE_MAP) FirstStruct;
	PLCXL_TABLE_MAP second = (PLCXL_TABLE_MAP) SecondStruct;

	UNREFERENCED_PARAMETER(Table);

	if (first->orgIndex < second->orgIndex) {
		return GenericLessThan;
	} else if (first->orgIndex > second->orgIndex) {
		return GenericGreaterThan;
	} else {
		return GenericEqual;
	}
}


PVOID NTAPI VAllocateRoutine (
	PRTL_GENERIC_TABLE Table,
	CLONG ByteSize
	)
{
	/*
	我去，终于找到原因了，原来还有additional memory。。。看来内存不够，重新调整大小
	For each new element, the AllocateRoutine is called to allocate memory for caller-supplied data 
	plus some additional memory for use by the Rtl...GenericTable routines. 
	
	Note that because of this "additional memory," 
	caller-supplied routines must not access the first (sizeof(RTL_SPLAY_LINKS ) + sizeof(LIST_ENTRY )) 
	bytes of any element in the generic table. 

	*/
	UNREFERENCED_PARAMETER(Table);
	UNREFERENCED_PARAMETER(ByteSize);

	return ExAllocateFromPagedLookasideList(&g_TableMapList);
}

VOID NTAPI VFreeRoutine (
	PRTL_GENERIC_TABLE Table,
	PVOID Buffer
	)
{
	UNREFERENCED_PARAMETER(Table);
	ExFreeToPagedLookasideList(&g_TableMapList, Buffer);
}

NTSTATUS VDeviceControl(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	)
{
	PEP_DEVICE_EXTENSION devext = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	PIO_STACK_LOCATION isp;

	PAGED_CODE();
	isp = IoGetCurrentIrpStackLocation( Irp );
	switch (isp->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_VOLUME_ONLINE:
		KdPrint(("SYS:VDeviceControl:IOCTL_VOLUME_ONLINE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IOCTL_VOLUME_OFFLINE:
		KdPrint(("SYS:VDeviceControl:IOCTL_VOLUME_OFFLINE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
#if (NTDDI_VERSION >= NTDDI_VISTA)
	case IOCTL_VOLUME_QUERY_MINIMUM_SHRINK_SIZE:
		//查询可以压缩的大小
		KdPrint(("SYS:VDeviceControl:IOCTL_VOLUME_QUERY_MINIMUM_SHRINK_SIZE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IOCTL_VOLUME_PREPARE_FOR_SHRINK:
		//开始准备压缩卷
		KdPrint(("SYS:VDeviceControl:IOCTL_VOLUME_PREPARE_FOR_SHRINK(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		if (devext->DE_FILTER.DE_DISKVOL.IsProtect) {
			//受保护？不能压缩卷
			KdPrint(("SYS:VDeviceControl:IOCTL_VOLUME_PREPARE_FOR_SHRINK(%wZ) is protected, deny to shrink\n", &devext->DE_FILTER.PhysicalDeviceName));
			return LLCompleteRequest(STATUS_ACCESS_DENIED, Irp, 0);
		}
		break;
#endif
	default:
		//KdPrint(("SYS:VDeviceControl:0x%08x(%wZ)\n", isp->Parameters.DeviceIoControl.IoControlCode, &devext->DE_FILTER.PhysicalDeviceName));
		break;
	}
	return LLSendToNextDriver(devext->DE_FILTER.LowerDeviceObject, Irp);
}
