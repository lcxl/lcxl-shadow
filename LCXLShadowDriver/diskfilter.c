#include "winkernel.h"
#include "driver.h"
#include "diskfilter.h"

//磁盘过滤设备源文件，放着磁盘过滤设备有关的函数

//磁盘过滤设备

//创建磁盘线程
NTSTATUS DCreateDiskThread(
	IN PDEVICE_OBJECT DiskFilterDO
	)
{
	PEP_DEVICE_EXTENSION devext;
	NTSTATUS status;
	PAGED_CODE();

	devext = (PEP_DEVICE_EXTENSION)DiskFilterDO->DeviceExtension;
	//关闭线程标示为FALSE
	devext->DE_FILTER.DE_DISK.ToTerminateThread = FALSE;
	
	//初始化这个卷的请求处理队列
	InitializeListHead(&devext->DE_FILTER.DE_DISK.RWListHead);
	//初始化内存重定向列表
	InitializeListHead(&devext->DE_FILTER.DE_DISK.RedirectListHead.ListEntry);
	//初始化磁盘自旋锁
	KeInitializeSpinLock(&devext->DE_FILTER.DE_DISK.RequestListLock);
	//初始化读写请求事件（同步事件）
	KeInitializeEvent(&devext->DE_FILTER.DE_DISK.RequestEvent, SynchronizationEvent, FALSE);
	//创建系统线程，上下文传入磁盘卷过滤设备的设备对象
	devext->DE_FILTER.DE_DISK.ThreadHandle = NULL;
	status = PsCreateSystemThread(&devext->DE_FILTER.DE_DISK.ThreadHandle, 
		(ACCESS_MASK)0L,
		NULL,
		NULL,
		NULL,
		DReadWriteThread, 
		DiskFilterDO);
	if (NT_SUCCESS(status)) {
		//获取线程句柄
		status = ObReferenceObjectByHandle(devext->DE_FILTER.DE_DISK.ThreadHandle,
			0,
			*PsThreadType,
			KernelMode,
			(PVOID *)&devext->DE_FILTER.DE_DISK.ThreadObject,
			NULL
			);
		if (!NT_SUCCESS(status)) {
			//不成功？
			devext->DE_FILTER.DE_DISK.ToTerminateThread = TRUE;
			//激活线程
			KeSetEvent(&devext->DE_FILTER.DE_DISK.RequestEvent, 0, TRUE);
			ASSERT(0);
		}
		KdPrint(("SYS:DCreateDiskThread:PsCreateSystemThread succeed.(handle = 0x%p)\n", devext->DE_FILTER.DE_DISK.ThreadHandle));
	} else {
		KdPrint(("SYS:DCreateDiskThread:PsCreateSystemThread failed.(0x%08x)\n", status));
	}
	return status;
}

//关闭磁盘线程
NTSTATUS DCloseDiskThread(
	IN PDEVICE_OBJECT DiskFilterDO
	)
{
	NTSTATUS status;
	PEP_DEVICE_EXTENSION devext;

	devext = (PEP_DEVICE_EXTENSION)DiskFilterDO->DeviceExtension;
	if (devext->DE_FILTER.DE_DISK.ThreadHandle!=NULL) {
		//结束标志设为TRUE
		devext->DE_FILTER.DE_DISK.ToTerminateThread = TRUE;
		//激活线程
		KeSetEvent(&devext->DE_FILTER.DE_DISK.RequestEvent, 0, TRUE);

		//等待线程退出
		status = KeWaitForSingleObject(devext->DE_FILTER.DE_DISK.ThreadObject, UserRequest, KernelMode, FALSE, NULL);
		if (!NT_SUCCESS(status)) {
			KdPrint(("SYS:VPostDetachDiskVolDevice:KeWaitForSingleObject failed(0x%08x).\n", status));
		}
		//减少引用
		ObDereferenceObject(devext->DE_FILTER.DE_DISK.ThreadObject);
		//关闭句柄
		ZwClose(devext->DE_FILTER.DE_DISK.ThreadHandle);
		devext->DE_FILTER.DE_DISK.ThreadHandle = NULL;
	}
	return STATUS_SUCCESS;
}

VOID DReadWriteThread(
	IN PVOID Context//线程上下文，这里是磁盘卷过滤驱动对象
	)
{
	//设备对象指针
	PDEVICE_OBJECT DeviceObject;
	//设备扩展
	PEP_DEVICE_EXTENSION diskdevext;

	PAGED_CODE();

	//设置这个线程的优先级为低实时性
	KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);
	DeviceObject = (PDEVICE_OBJECT)Context;
	diskdevext = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	ASSERT(diskdevext->DeviceType == DO_DISK);
	//如果线程结束标志为TRUE，则退出
	while (!diskdevext->DE_FILTER.DE_DISK.ToTerminateThread) {
		//请求队列的入口
		PLIST_ENTRY	ReqEntry;

		//先等待请求队列同步事件，如果队列中没有irp需要处理，我们的线程就等待在这里，让出cpu时间给其它线程
		KeWaitForSingleObject(
			&diskdevext->DE_FILTER.DE_DISK.RequestEvent,
			Executive,
			KernelMode,
			FALSE,
			NULL
			);
		//从请求队列的首部拿出一个请求来准备处理，这里使用了自旋锁机制，所以不会有冲突
		while (ReqEntry = ExInterlockedRemoveHeadList(&diskdevext->DE_FILTER.DE_DISK.RWListHead, &diskdevext->DE_FILTER.DE_DISK.RequestListLock), ReqEntry != NULL) {
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
			if (IrpSp->MajorFunction == IRP_MJ_READ) {
				//重定向读操作
				KdPrint((
					"SYS:DReadWriteThread:Read disk %wZ outside, offset=0x%016I64x, length=%d\n", 
					&diskdevext->DE_FILTER.PhysicalDeviceName, 
					IrpSp->Parameters.Read.ByteOffset.QuadPart, 
					IrpSp->Parameters.Read.Length
					));
				status = LLWaitIRPCompletion(diskdevext->DE_FILTER.LowerDeviceObject, Irp);
				if (NT_SUCCESS(status)) {
					LCXLReadFromRedirectRWMemList(
						&diskdevext->DE_FILTER.DE_DISK.RedirectListHead, 
						buffer,
						(ULONGLONG)IrpSp->Parameters.Read.ByteOffset.QuadPart, 
						IrpSp->Parameters.Read.Length
						);
				} else {
					KdPrint(("SYS:DReadWriteThread:LLWaitIRPCompletion Error:0x%08x\n", status));
				}
				LLCompleteRequest(status, Irp, Irp->IoStatus.Information);
			} else {
				//重定向读操作
				KdPrint((
					"SYS:DReadWriteThread:Write disk %wZ outside, offset=0x%016I64x, length=%d\n", 
					&diskdevext->DE_FILTER.PhysicalDeviceName, 
					IrpSp->Parameters.Write.ByteOffset.QuadPart, 
					IrpSp->Parameters.Write.Length
					));
				//重定向写操作
				LCXLInsertToRedirectRWMemList(
					&diskdevext->DE_FILTER.DE_DISK.RedirectListHead, 
					buffer, 
					(ULONGLONG)IrpSp->Parameters.Write.ByteOffset.QuadPart, 
					IrpSp->Parameters.Write.Length
					);
#ifdef DBG
				{
					PREDIRECT_RW_MEM RedirectListEntry;
					ULONG TotalSize = 0;

					RedirectListEntry = &diskdevext->DE_FILTER.DE_DISK.RedirectListHead;
					//将写入重定向，并尽可能的合并列表
					for (RedirectListEntry = (PREDIRECT_RW_MEM)RedirectListEntry->ListEntry.Flink; RedirectListEntry != &diskdevext->DE_FILTER.DE_DISK.RedirectListHead;RedirectListEntry = (PREDIRECT_RW_MEM)RedirectListEntry->ListEntry.Flink) {
						TotalSize += RedirectListEntry->length;
					}
					DbgPrint("SYS:DReadWriteThread:TotalSize=%x\n", TotalSize);
				}
#endif
				//在这里直接成功完成，来欺骗系统
				LLCompleteRequest(STATUS_SUCCESS, Irp, IrpSp->Parameters.Write.Length);
			}
			//-----------
		}
	}
	PsTerminateSystemThread(STATUS_SUCCESS);
}

//磁盘写入hook函数
NTSTATUS DHookDeviceWrite(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	)
{
	PDEVICE_OBJECT diskFDO;
	//查看挂载的设备
	diskFDO = DeviceObject->AttachedDevice;
	//开始循环查找
	while (diskFDO!=NULL) {
		//判断我们的驱动是否在堆栈上
		if (diskFDO->DriverObject == g_CDO->DriverObject) {
			break;
		}
		diskFDO = diskFDO->AttachedDevice;
	}
	//找到了DISK设备
	if (diskFDO!=NULL) {
		//判断IRP的模式是不是内核模式，这个是防止ring3应用程序的破坏
		//IoGetRequestorProcessId是为了防止部分驱动程序对磁盘的破坏
		if (Irp->RequestorMode == KernelMode&&IoGetRequestorProcessId(Irp)<=4||g_DriverSetting.CurShadowMode == SM_NONE) {
			return g_DriverSetting.DiskInfo.OrgMFWrite(DeviceObject, Irp);
		} else {
			//失败吧，亲，你没有权限哦
			KdPrint(("SYS:VDeviceWrite(%d):Irp->RequestorMode != KernelMode\n", IoGetRequestorProcessId(Irp)));
		}
	} else {
		//机器狗来了？失败
		KdPrint(("SYS:DHookDeviceWrite:LCXLShadow Filter Device not Found!!!!\n"));
	}
	return LLCompleteRequest(STATUS_ACCESS_DENIED, Irp, 0);
}

//Hook DISK 驱动的WRITE进程
__inline VOID DHookDiskMJWriteFunc()
{
	PDRIVER_DISPATCH WriteFunc;

	WriteFunc = g_DriverSetting.DiskInfo.DiskDriverObject->MajorFunction[IRP_MJ_WRITE];
	if (WriteFunc != DHookDeviceWrite) {
		//获取原始的DISK的写入例程
		g_DriverSetting.DiskInfo.OrgMFWrite = WriteFunc;
		//替换DISK的写入例程
		g_DriverSetting.DiskInfo.DiskDriverObject->MajorFunction[IRP_MJ_WRITE] = DHookDeviceWrite;
	}
}

VOID DPostAttachDiskDevice(
	IN PDEVICE_OBJECT FilterDeviceObject//文件系统过滤设备 
	)
{
	PEP_DEVICE_EXTENSION devext;
	NTSTATUS status;

	PAGED_CODE();
	devext = (PEP_DEVICE_EXTENSION)FilterDeviceObject->DeviceExtension;
	if (!g_DriverSetting.IsFirstGetDiskInfo) {
		UNICODE_STRING DiskDriverName;
		PDEVICE_OBJECT diskFDO;

		g_DriverSetting.IsFirstGetDiskInfo = TRUE;
		RtlInitUnicodeString(&DiskDriverName, L"\\Driver\\Disk");
		diskFDO = devext->DE_FILTER.PhysicalDeviceObject;
		
		while (diskFDO!=NULL) {
			//判断是不是Disk的设备
			if (RtlCompareUnicodeString(&diskFDO->DriverObject->DriverName, &DiskDriverName, TRUE)==0) {
				break;
			}
			diskFDO = diskFDO->AttachedDevice;
		}
		
		//找到了DISK最底层设备
		if (diskFDO!=NULL) {
			//获取DISK驱动对象
			g_DriverSetting.DiskInfo.DiskDriverObject = diskFDO->DriverObject;
			DHookDiskMJWriteFunc();
		}
		//获取磁盘ID
		status = DiskIdentifyDevice(devext->DE_FILTER.LowerDeviceObject, &g_DriverSetting.DiskInfo.DiskIDInfo);
		if (NT_SUCCESS(status)) {

		} else {
			//fix-it:目前不能获取SCSI硬盘的序列号
			//目前先置零
			RtlZeroMemory(&g_DriverSetting.DiskInfo.DiskIDInfo, sizeof(g_DriverSetting.DiskInfo.DiskIDInfo));
			KdPrint(("!!!SYS:DPostAttachDiskDevice:DiskIdentifyDevice failed: 0x%08x\n", status));
		}
	} else {
		return;
	}
}

NTSTATUS DDeviceRead(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	)
{
	PEP_DEVICE_EXTENSION diskdevext;
	//Windows 8 下,此函数的IRQ会高于APC_LEVEL,因此这里要删除PAGED_CODE();
	//PAGED_CODE();

	diskdevext = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	if (diskdevext->DE_FILTER.DE_DISK.InitializeStatus == DIS_MEMORY) {
		PIO_STACK_LOCATION irpsp;
		KIRQL OldIrql;
		BOOLEAN IsSafeOpt;//是否是安全
		ULONG i;

		irpsp = IoGetCurrentIrpStackLocation(Irp);
		IsSafeOpt = FALSE;
		KeAcquireSpinLock(&diskdevext->DE_FILTER.DE_DISK.RequestListLock, &OldIrql);
		for (i = 0; i < diskdevext->DE_FILTER.DE_DISK.DiskVolFilterNum; i++) {
			//如果IRP读写都是在磁盘卷的区域内，则可信任
			if (irpsp->Parameters.Read.ByteOffset.QuadPart>=diskdevext->DE_FILTER.DE_DISK.DiskVolFilterDO[i].StartingOffset.QuadPart&&
				irpsp->Parameters.Read.ByteOffset.QuadPart+irpsp->Parameters.Read.Length<=diskdevext->DE_FILTER.DE_DISK.DiskVolFilterDO[i].StartingOffset.QuadPart+diskdevext->DE_FILTER.DE_DISK.DiskVolFilterDO[i].ExtentLength.QuadPart) {
					IsSafeOpt = TRUE;
					break;
			}
		}
		KeReleaseSpinLock(&diskdevext->DE_FILTER.DE_DISK.RequestListLock, OldIrql);
		if (!IsSafeOpt) {
			//将IRP设置为等待状态
			IoMarkIrpPending(Irp);
			//然后将这个irp放进相应的请求队列里
			ExInterlockedInsertTailList(
				&diskdevext->DE_FILTER.DE_DISK.RWListHead,
				&Irp->Tail.Overlay.ListEntry,
				&diskdevext->DE_FILTER.DE_DISK.RequestListLock
				);
			//设置队列的等待事件，通知队列对这个irp进行处理
			KeSetEvent(
				&diskdevext->DE_FILTER.DE_DISK.RequestEvent, 
				(KPRIORITY)0, 
				FALSE);
			//返回pending状态，这个irp就算处理完了
			return STATUS_PENDING;
		}
	}
	return LLSendToNextDriver(diskdevext->DE_FILTER.LowerDeviceObject, Irp);
}

NTSTATUS DDeviceWrite(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	)
{
	PEP_DEVICE_EXTENSION diskdevext;

	//Windows 8 下,此函数的IRQ会高于APC_LEVEL,因此这里要删除PAGED_CODE();
	//PAGED_CODE();

	diskdevext = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	if (diskdevext->DE_FILTER.DE_DISK.InitializeStatus == DIS_MEMORY) {
		KIRQL OldIrql;
		PIO_STACK_LOCATION irpsp;
		BOOLEAN IsSafeOpt;//是否是安全
		ULONG i;

		irpsp = IoGetCurrentIrpStackLocation(Irp);
		IsSafeOpt = FALSE;
		//PAGED_CODE();
		KeAcquireSpinLock(&diskdevext->DE_FILTER.DE_DISK.RequestListLock, &OldIrql);
		for (i = 0; i < diskdevext->DE_FILTER.DE_DISK.DiskVolFilterNum; i++) {
			//如果IRP读写都是在磁盘卷的区域内，则可信任
			if (irpsp->Parameters.Write.ByteOffset.QuadPart>=diskdevext->DE_FILTER.DE_DISK.DiskVolFilterDO[i].StartingOffset.QuadPart&&
				irpsp->Parameters.Write.ByteOffset.QuadPart+irpsp->Parameters.Write.Length<=diskdevext->DE_FILTER.DE_DISK.DiskVolFilterDO[i].StartingOffset.QuadPart+diskdevext->DE_FILTER.DE_DISK.DiskVolFilterDO[i].ExtentLength.QuadPart) {
					IsSafeOpt = TRUE;
					break;
			}
		}
		KeReleaseSpinLock(&diskdevext->DE_FILTER.DE_DISK.RequestListLock, OldIrql);
		if (!IsSafeOpt) {
			diskdevext->DE_FILTER.DE_DISK.IsVolumeChanged = TRUE;
			//将IRP设置为等待状态
			IoMarkIrpPending(Irp);
			//然后将这个irp放进相应的请求队列里
			ExInterlockedInsertTailList(
				&diskdevext->DE_FILTER.DE_DISK.RWListHead,
				&Irp->Tail.Overlay.ListEntry,
				&diskdevext->DE_FILTER.DE_DISK.RequestListLock
				);
			//设置队列的等待事件，通知队列对这个irp进行处理
			KeSetEvent(
				&diskdevext->DE_FILTER.DE_DISK.RequestEvent, 
				(KPRIORITY)0, 
				FALSE);
			//返回pending状态，这个irp就算处理完了
			return STATUS_PENDING;
		} else {
			IsSafeOpt = TRUE;
		}
	}
	//检测HOOK是否被恢复，如果被恢复，则重新HOOK
	DHookDiskMJWriteFunc();
	return LLSendToNextDriver(diskdevext->DE_FILTER.LowerDeviceObject, Irp);
}

NTSTATUS DDevicePnp(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	)
{
	PEP_DEVICE_EXTENSION devext = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	PIO_STACK_LOCATION isp;
	NTSTATUS status;

	PAGED_CODE();
	isp = IoGetCurrentIrpStackLocation( Irp );
	switch(isp->MinorFunction) 
	{
	case IRP_MN_START_DEVICE:
		//启动设备
		KdPrint(("SYS:DDevicePnp:IRP_MN_START_DEVICE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		status = LLWaitIRPCompletion(devext->DE_FILTER.LowerDeviceObject, Irp);
		if (NT_SUCCESS(status)) {
			KEVENT event;
			PIRP QueryIrp;
			NTSTATUS localstatus;
			IO_STATUS_BLOCK iostatus;
			PSTORAGE_DEVICE_DESCRIPTOR DeviceDescriptor;//磁盘描述信息
			ULONG DeviceDescriptorSize;
			STORAGE_DEVICE_NUMBER DeviceNumber;

			//复制目标设备有用的标记
			DeviceObject->Characteristics |= devext->DE_FILTER.LowerDeviceObject->Characteristics & (FILE_REMOVABLE_MEDIA | FILE_READ_ONLY_DEVICE | FILE_FLOPPY_DISKETTE); 
			//初始化
			devext->DE_FILTER.DE_DISK.DiskNumber = (ULONG)-1;
			KeInitializeEvent(&event, NotificationEvent, FALSE);
			
			//
			// 查询磁盘序号
			//
			QueryIrp = IoBuildDeviceIoControlRequest(
				IOCTL_STORAGE_GET_DEVICE_NUMBER,
				devext->DE_FILTER.LowerDeviceObject,
				NULL,
				0,
				&DeviceNumber,
				sizeof(DeviceNumber),
				FALSE,
				&event,
				&iostatus);

			if (QueryIrp != NULL) {
				localstatus = IoCallDriver(devext->DE_FILTER.LowerDeviceObject, QueryIrp);
				if (localstatus == STATUS_PENDING) {
					KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
					localstatus = iostatus.Status;
				}
				if (NT_SUCCESS(localstatus)) {
					devext->DE_FILTER.DE_DISK.DiskNumber = DeviceNumber.DeviceNumber;
				} else {
					KdPrint(("SYS:DDevicePnp(%wZ) IOCTL_STORAGE_GET_DEVICE_NUMBER failed(0x%08x)\n", &devext->DE_FILTER.PhysicalDeviceName, localstatus));
				}
			} else {
				//资源不足
				localstatus = STATUS_INSUFFICIENT_RESOURCES;
			}
			//如果不成功，使用其他方法
			if (!NT_SUCCESS(localstatus)) {
				VOLUME_NUMBER VolumeNumber;

				//清除事件
				KeClearEvent(&event);
				RtlZeroMemory(&VolumeNumber, sizeof(VOLUME_NUMBER));
				//查询卷
				QueryIrp = IoBuildDeviceIoControlRequest(
					IOCTL_VOLUME_QUERY_VOLUME_NUMBER,
					devext->DE_FILTER.LowerDeviceObject, 
					NULL, 
					0,
					&VolumeNumber, 
					sizeof(VolumeNumber), 
					FALSE, 
					&event, 
					&iostatus);
				if (QueryIrp!=NULL) {
					localstatus = IoCallDriver(devext->DE_FILTER.LowerDeviceObject, QueryIrp);
					if (localstatus == STATUS_PENDING) {
						KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
						localstatus = iostatus.Status;
					}
					if (NT_SUCCESS(localstatus)) {
						devext->DE_FILTER.DE_DISK.DiskNumber = VolumeNumber.VolumeNumber;
					}
				} else {
					//资源不足
					localstatus = STATUS_INSUFFICIENT_RESOURCES;
				}
			}
			
			//清除事件
			KeClearEvent(&event);
			DeviceDescriptorSize =  sizeof(STORAGE_DEVICE_DESCRIPTOR)+1024;
			DeviceDescriptor = (PSTORAGE_DEVICE_DESCRIPTOR)ExAllocatePoolWithTag(NonPagedPool, DeviceDescriptorSize, 'DESL');
			if (DeviceDescriptor != NULL) {
				PSTORAGE_PROPERTY_QUERY Query; //   查询输入参数 
				ULONG QuerySize;

				RtlZeroMemory(DeviceDescriptor, DeviceDescriptorSize);
				DeviceDescriptor->Size = DeviceDescriptorSize;

				QuerySize = sizeof(STORAGE_PROPERTY_QUERY)+1024;
				Query = (PSTORAGE_PROPERTY_QUERY)ExAllocatePoolWithTag(NonPagedPool, QuerySize, 'QUEY');

				RtlZeroMemory(Query, QuerySize);
				//设置查询类型
				Query->PropertyId = StorageDeviceProperty;
				Query->QueryType = PropertyStandardQuery;
				//查询属性，这里主要查总线信息
				QueryIrp = IoBuildDeviceIoControlRequest(
					IOCTL_STORAGE_QUERY_PROPERTY,
					devext->DE_FILTER.PhysicalDeviceObject,
					Query,
					QuerySize,
					DeviceDescriptor,
					DeviceDescriptorSize,
					FALSE,
					&event,
					&iostatus);

				if (QueryIrp != NULL) {
					localstatus = IoCallDriver(devext->DE_FILTER.PhysicalDeviceObject, QueryIrp);
					if (localstatus == STATUS_PENDING) {
						KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
						localstatus = iostatus.Status;
					}
					if (NT_SUCCESS(localstatus)) {
						devext->DE_FILTER.DE_DISK.BusType = DeviceDescriptor->BusType;
					} else {
						KdPrint(("SYS:DDevicePnp(%wZ) IOCTL_STORAGE_QUERY_PROPERTY failed(0x%08x)\n", &devext->DE_FILTER.PhysicalDeviceName, localstatus));
					}
				}
				ExFreePool(Query);
				ExFreePool(DeviceDescriptor);
			} else {
				KdPrint(("SYS:DDevicePnp(%wZ) ExAllocatePoolWithTag failed\n", &devext->DE_FILTER.PhysicalDeviceName));
			}
			//清除事件
			KeClearEvent(&event);
			//查询磁盘扇区大小
			QueryIrp = IoBuildDeviceIoControlRequest(
				IOCTL_DISK_GET_DRIVE_GEOMETRY,
				devext->DE_FILTER.LowerDeviceObject,
				NULL,
				0,
				&devext->DE_FILTER.DE_DISK.DiskGeometry,
				sizeof(devext->DE_FILTER.DE_DISK.DiskGeometry),
				FALSE,
				&event,
				&iostatus);
			if (QueryIrp != NULL) {
				localstatus = IoCallDriver(devext->DE_FILTER.LowerDeviceObject, QueryIrp);
				if (localstatus == STATUS_PENDING) {
					KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
					localstatus = iostatus.Status;
				}
				if (NT_SUCCESS(localstatus)) {
					KdPrint(("SYS:DDevicePnp(%wZ) IOCTL_DISK_GET_DRIVE_GEOMETRY, BytesPerSector=%d\n", &devext->DE_FILTER.PhysicalDeviceName, devext->DE_FILTER.DE_DISK.DiskGeometry.BytesPerSector));
				} else {
					KdPrint(("SYS:DDevicePnp(%wZ) IOCTL_DISK_GET_DRIVE_GEOMETRY failed!(0x%08x)\n", &devext->DE_FILTER.PhysicalDeviceName, localstatus));
				}
			}
			if (DIS_LOCAL_DISK(devext->DE_FILTER.DE_DISK.BusType)) {
				DCreateDiskThread(DeviceObject);
			}
			/*else {
				KdPrint(("SYS:DDevicePnp:IRP_MN_START_DEVICE(%wZ) is not a local disk, detach it\n", &devext->DE_FILTER.PhysicalDeviceName));
				//不是本地卷，则删除过滤设备
				IoDetachDevice(devext->DE_FILTER.LowerDeviceObject);
				//如果存在过滤设备，就要删除它，这并不是真正的删除，只是减少其引用，所以访问devext是安全的。
				IoDeleteDevice(DeviceObject);
			}*/
		}
		return LLCompleteRequest(status, Irp, Irp->IoStatus.Information);
		break;
		/*
	case IRP_MN_QUERY_REMOVE_DEVICE:
		KdPrint(("SYS:DDevicePnp:IRP_MN_QUERY_REMOVE_DEVICE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
		*/
	case IRP_MN_REMOVE_DEVICE:
		KdPrint(("SYS:DDevicePnp:IRP_MN_REMOVE_DEVICE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		IoDetachDevice(devext->DE_FILTER.LowerDeviceObject);
		if (DIS_LOCAL_DISK(devext->DE_FILTER.DE_DISK.BusType)) {
			//关闭设备
			DCloseDiskThread(DeviceObject);
		}
		//如果存在过滤设备，就要删除它，这并不是真正的删除，只是减少其引用，所以访问devext是安全的。
		IoDeleteDevice(DeviceObject);
		break;
		/*
	case IRP_MN_CANCEL_REMOVE_DEVICE:
		KdPrint(("SYS:DDevicePnp:IRP_MN_CANCEL_REMOVE_DEVICE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
		*/
	case IRP_MN_STOP_DEVICE:
		KdPrint(("SYS:DDevicePnp:IRP_MN_STOP_DEVICE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		status = LLWaitIRPCompletion(devext->DE_FILTER.LowerDeviceObject, Irp);
		if (NT_SUCCESS(status)) {
		}
		return LLCompleteRequest(status, Irp, Irp->IoStatus.Information);
		break;
		/*
	case IRP_MN_QUERY_STOP_DEVICE:
		KdPrint(("SYS:DDevicePnp:IRP_MN_QUERY_STOP_DEVICE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_CANCEL_STOP_DEVICE:
		KdPrint(("SYS:DDevicePnp:IRP_MN_CANCEL_STOP_DEVICE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;

	case IRP_MN_QUERY_DEVICE_RELATIONS:
		KdPrint(("SYS:DDevicePnp:IRP_MN_QUERY_DEVICE_RELATIONS-%s(%wZ)\n", qrydevtypestrlst[isp->Parameters.QueryDeviceRelations.Type], &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_INTERFACE:
		KdPrint(("SYS:DDevicePnp:IRP_MN_QUERY_INTERFACE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_CAPABILITIES:
		KdPrint(("SYS:DDevicePnp:IRP_MN_QUERY_CAPABILITIES(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_RESOURCES:
		KdPrint(("SYS:DDevicePnp:IRP_MN_QUERY_RESOURCES(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
		KdPrint(("SYS:DDevicePnp:IRP_MN_QUERY_RESOURCE_REQUIREMENTS(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_DEVICE_TEXT:
		KdPrint(("SYS:DDevicePnp:IRP_MN_QUERY_DEVICE_TEXT(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
		KdPrint(("SYS:DDevicePnp:IRP_MN_FILTER_RESOURCE_REQUIREMENTS(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;

	case IRP_MN_READ_CONFIG:
		KdPrint(("SYS:DDevicePnp:IRP_MN_READ_CONFIG(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_WRITE_CONFIG:
		KdPrint(("SYS:DDevicePnp:IRP_MN_WRITE_CONFIG(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_EJECT:
		KdPrint(("SYS:DDevicePnp:IRP_MN_EJECT(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_SET_LOCK:
		KdPrint(("SYS:DDevicePnp:IRP_MN_SET_LOCK(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_ID:
		KdPrint(("SYS:DDevicePnp:IRP_MN_QUERY_ID(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_PNP_DEVICE_STATE:
		KdPrint(("SYS:DDevicePnp:IRP_MN_QUERY_PNP_DEVICE_STATE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_BUS_INFORMATION:
		KdPrint(("SYS:DDevicePnp:IRP_MN_QUERY_BUS_INFORMATION(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_DEVICE_USAGE_NOTIFICATION:
		KdPrint(("SYS:DDevicePnp:IRP_MN_DEVICE_USAGE_NOTIFICATION(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_SURPRISE_REMOVAL:
		KdPrint(("SYS:DDevicePnp:IRP_MN_SURPRISE_REMOVAL(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
		*/
	}

	return LLSendToNextDriver(devext->DE_FILTER.LowerDeviceObject, Irp);
}

NTSTATUS DDeviceControl(
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
	case IOCTL_DISK_DELETE_DRIVE_LAYOUT://删除分区表
	case IOCTL_DISK_SET_DRIVE_LAYOUT_EX://设置分区信息
	case IOCTL_DISK_SET_DRIVE_LAYOUT://设置分区信息
	case IOCTL_DISK_GROW_PARTITION://扩展分区
	case IOCTL_DISK_SET_PARTITION_INFO_EX://设置分区信息
	case IOCTL_DISK_SET_PARTITION_INFO://设置分区信息
	case IOCTL_DISK_VERIFY://磁盘验证，执行写0操作
		KdPrint(("SYS:DDeviceControl:IOCTL_0x%08x(%wZ)\n", isp->Parameters.DeviceIoControl.IoControlCode, &devext->DE_FILTER.PhysicalDeviceName));
		if (DIS_LOCAL_DISK(devext->DE_FILTER.DE_DISK.BusType)) {
			BOOLEAN HasShadowVolume;//是否有保护卷
			ULONG i;
			PDEVICE_OBJECT DiskVolFilterDO;
	
			HasShadowVolume = FALSE;
			for (i = 0; i < devext->DE_FILTER.DE_DISK.DiskVolFilterNum; i++) {
				PEP_DEVICE_EXTENSION voldevext;

				DiskVolFilterDO = devext->DE_FILTER.DE_DISK.DiskVolFilterDO[i].FilterDO;
				voldevext = (PEP_DEVICE_EXTENSION)DiskVolFilterDO->DeviceExtension;
				//是否有保护卷在磁盘中
				HasShadowVolume = HasShadowVolume||voldevext->DE_FILTER.DE_DISKVOL.IsProtect;
			}
			//如果有保护卷，则禁止此操作
			if (HasShadowVolume) {
				return LLCompleteRequest(STATUS_ACCESS_DENIED, Irp, 0);
			}
		}
		break;
	}
	return LLSendToNextDriver(devext->DE_FILTER.LowerDeviceObject, Irp);
}