#include "winkernel.h"
#include "driver.h"
#include "fatlbr.h"
#include "filesysfilter.h"
#include "diskvolfilter.h"



//获取第一数据扇区的偏移量
NTSTATUS FGetFirstDataSectorOffset(
	IN HANDLE FSDeviceHandle, 
	OUT PULONGLONG FirstDataSector
	)
{
	IO_STATUS_BLOCK ioBlock;
	NTSTATUS status;
	FAT_LBR		fatLBR = { 0 };
	LARGE_INTEGER	pos;

	PAGED_CODE();

	ASSERT(FirstDataSector != NULL);
	*FirstDataSector = 0;
	pos.QuadPart = 0;
	//LCXL，如果参数指针为0，则返回不存在(STATUS_NOT_FOUND)
	//LCXL，读取文件获取信息
	status = ZwReadFile(FSDeviceHandle, NULL, NULL, NULL, &ioBlock, &fatLBR, sizeof(fatLBR), &pos, NULL);
	//LCXL，读取文件获取信息
	if (NT_SUCCESS(status) && sizeof(FAT_LBR) == ioBlock.Information) {
		DWORD dwRootDirSectors	= 0;
		DWORD dwSectorNumPreCluster	= 0;

		// Validate jump instruction to boot code. This field has two
		// allowed forms: 
		// jmpBoot[0] = 0xEB, jmpBoot[1] = 0x??, jmpBoot[2] = 0x90 
		// and
		// jmpBoot[0] = 0xE9, jmpBoot[1] = 0x??, jmpBoot[2] = 0x??
		// 0x?? indicates that any 8-bit value is allowed in that byte.
		// JmpBoot[0] = 0xEB is the more frequently used format.
		if (fatLBR.wTrailSig == 0xAA55 && (fatLBR.Jump[0] == 0xEB && fatLBR.Jump[2] == 0x90 || fatLBR.Jump[0] == 0xE9|| fatLBR.Jump[0] == 0x49)) {
			// Compute first sector offset for the FAT volumes:		
			// First, we determine the count of sectors occupied by the
			// root directory. Note that on a FAT32 volume the BPB_RootEntCnt
			// value is always 0, so on a FAT32 volume dwRootDirSectors is
			// always 0. The 32 in the above is the size of one FAT directory
			// entry in bytes. Note also that this computation rounds up.
			//LCXL，下面是我对原文注释的翻译
			//计算FAT卷的第一个扇区
			//首先，我们通过根目录确定被占用的扇区数量。
			//注意：FAT32卷的BPB_RootEntCnt总是0
			dwRootDirSectors = ((( fatLBR.bpb.RootEntries * 32 ) + ( fatLBR.bpb.BytesPerSector - 1 ))/fatLBR.bpb.BytesPerSector );
			// The start of the data region, the first sector of cluster 2,
			// is computed as follows:
			//数据区的开始是
			//计算如下：
			dwSectorNumPreCluster = fatLBR.bpb.SectorsPerFat;		
			if( !dwSectorNumPreCluster ) {
				//是FAT32
				dwSectorNumPreCluster = fatLBR.ebpb32.LargeSectorsPerFat;
			}
			if( dwSectorNumPreCluster ) {
				// 得到数据区开始，就是第一簇的位置
				//数据区的起始扇区=FAT隐藏扇区+FAT数据块*一个簇的扇区数+根目录扇区数
				*FirstDataSector = ( fatLBR.bpb.ReservedSectors + ( fatLBR.bpb.Fats * dwSectorNumPreCluster ) + dwRootDirSectors );

			} else {
				//是NTFS
			}
		} else {
			//status = STATUS_NOT_FOUND;
		}
	}

	return status;
}

//文件过滤设备
NTSTATUS FDeviceCreate(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	)
{
	PEP_DEVICE_EXTENSION devext;
	//PIO_STACK_LOCATION isp;
	//PFILE_OBJECT FileObject;
	PAGED_CODE();

	devext = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	return LLSendToNextDriver(devext->DE_FILTER.LowerDeviceObject, Irp);
}

NTSTATUS FDeviceCleanup(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	)
{
	PEP_DEVICE_EXTENSION devext = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	PAGED_CODE();

	return LLSendToNextDriver(devext->DE_FILTER.LowerDeviceObject, Irp);
}

NTSTATUS FDeviceClose(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	)
{
	PEP_DEVICE_EXTENSION devext = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	PAGED_CODE();

	return LLSendToNextDriver(devext->DE_FILTER.LowerDeviceObject, Irp);
}

//是否需要挂载这个卷设备
//如果返回False，则不挂载此设备
//注意：有可能会出现已挂载的设备又有请求挂载，需要自行辨别
BOOL FPreAttachVolumeDevice(
	IN PDEVICE_OBJECT VolumeDeviceObject,//卷设备对象
	IN PDEVICE_OBJECT DiskDeviceObject,//磁盘设备对象
	IN PUNICODE_STRING DiskDeviceName///磁盘设备名称
	)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(DiskDeviceName);
	UNREFERENCED_PARAMETER(DiskDeviceObject);
	UNREFERENCED_PARAMETER(VolumeDeviceObject);

	KdPrint(("SYS:FPreAttachVolumeDevice:DiskDeviceName=%wZ\n", DiskDeviceName));	
	return TRUE;
}

//挂载成功时，调用此函数
VOID FPostAttachVolumeDevice(
	IN PDEVICE_OBJECT FilterDeviceObject//文件系统过滤设备
	)
{
	NTSTATUS status;
	PEP_DEVICE_EXTENSION fsdevext;
	PDEVICE_OBJECT devobj;
	PEP_DEVICE_EXTENSION voldevext;

	PAGED_CODE();

	fsdevext = (PEP_DEVICE_EXTENSION)FilterDeviceObject->DeviceExtension;
	KdPrint(("SYS(%d):FPostAttachVolumeDevice:DiskDeviceName=%wZ\n", PsGetCurrentProcessId(), &fsdevext->DE_FILTER.DE_FSVOL.DiskVolDeviceName));

	//寻找所对应的下层磁盘卷过滤设备

	//获取驱动中第一个设备对象，开始遍历
	devobj = FilterDeviceObject->DriverObject->DeviceObject;
	while (devobj != NULL) {
		voldevext = (PEP_DEVICE_EXTENSION)devobj->DeviceExtension;
		//如果找到的设备是磁盘卷过滤设备
		if (voldevext->DeviceType==DO_DISKVOL) {
			//如果找到了下层的磁盘卷过滤设备
			if (voldevext->DE_FILTER.PhysicalDeviceObject == fsdevext->DE_FILTER.DE_FSVOL.DiskVolDeviceObject) {
				break;
			}
		}
		//没有找到，找下一个设备
		devobj = devobj->NextDevice;
	}
	//找到下层设备了
	if (devobj != NULL) {
		HANDLE hThread;

		voldevext = (PEP_DEVICE_EXTENSION)devobj->DeviceExtension;
		//建立文件系统过滤设备和磁盘卷过滤设备的关系
		fsdevext->DE_FILTER.DE_FSVOL.DiskVolFilter = devobj;
		voldevext->DE_FILTER.DE_DISKVOL.FSVolFilter = FilterDeviceObject;

		if (g_DriverSetting.InitState == DS_NONE) {
			//开始初始化
			g_DriverSetting.InitState = DS_INITIALIZING;
			KdPrint(("SYS:FPostAttachVolumeDevice:The volume %wZ is(maybe) system volume\n", &fsdevext->DE_FILTER.DE_FSVOL.DiskVolDeviceName));
			status = PsCreateSystemThread(&hThread, 
				(ACCESS_MASK)0L,
				NULL,
				NULL,
				NULL,
				VGetDriverSettingThread, 
				FilterDeviceObject);
			if (NT_SUCCESS(status)) {
				ZwClose(hThread);
			}
		}

		if (voldevext->DE_FILTER.DE_DISKVOL.InitializeStatus==DVIS_NONE||!voldevext->DE_FILTER.DE_DISKVOL.IsProtect) {
			//重置初始化状态
			voldevext->DE_FILTER.DE_DISKVOL.InitializeStatus = DVIS_NONE;
			status = PsCreateSystemThread(&hThread, 
				(ACCESS_MASK)0L,
				NULL,
				NULL,
				NULL,
				VGetFSInfoThread, 
				FilterDeviceObject);
			if (NT_SUCCESS(status)) {
				ZwClose(hThread);
			}
		}
		status = STATUS_SUCCESS;
	} else {
		//如果找不到，说明是其他乱七八糟的设备，
		KdPrint(("SYS(%d):FPostAttachVolumeDevice (%wZ) is not find is below disk volume device.\n", PsGetCurrentProcessId(), &fsdevext->DE_FILTER.DE_FSVOL.DiskVolDeviceName));
		status = STATUS_NOT_FOUND;
	}
	if (!NT_SUCCESS(status)){
		KdPrint(("SYS(%d):FPostAttachVolumeDevice (%wZ) is Fail!0x%08x,Delete filter deivce\n", PsGetCurrentProcessId(), &fsdevext->DE_FILTER.DE_FSVOL.DiskVolDeviceName, status));
		IoDetachDevice(fsdevext->DE_FILTER.LowerDeviceObject);
		FPostDetachVolumeDevice(FilterDeviceObject, fsdevext->DE_FILTER.LowerDeviceObject);
		IoDeleteDevice(FilterDeviceObject);
	}
}

//卸载卷设备
VOID FPostDetachVolumeDevice(
	IN PDEVICE_OBJECT DeviceObject,//我的设备
	IN PDEVICE_OBJECT VolumeDeviceObject//设备卷，确切的是下层设备
	)
{
	PEP_DEVICE_EXTENSION fsdevExt;
	PEP_DEVICE_EXTENSION voldevExt;
	PAGED_CODE();

	UNREFERENCED_PARAMETER( VolumeDeviceObject );
	fsdevExt = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	ASSERT(fsdevExt->DeviceType == DO_FSVOL);

	//这里的Phy
	KdPrint(("SYS:FPostDetachVolumeDevice:DiskDeviceName=%wZ\n", &fsdevExt->DE_FILTER.DE_FSVOL.DiskVolDeviceName));
	//解除文件系统过滤设备和磁盘卷过滤设备的关系
	if (fsdevExt->DE_FILTER.DE_FSVOL.DiskVolFilter != NULL) {
		voldevExt = (PEP_DEVICE_EXTENSION)fsdevExt->DE_FILTER.DE_FSVOL.DiskVolFilter->DeviceExtension;
		ASSERT(voldevExt->DeviceType == DO_DISKVOL);
		voldevExt->DE_FILTER.DE_DISKVOL.FSVolFilter = NULL;
		fsdevExt->DE_FILTER.DE_FSVOL.DiskVolFilter = NULL;
	}
}

NTSTATUS FSetDirectRWFile(
	IN PEP_DEVICE_EXTENSION fsdevext, 
	IN PUNICODE_STRING FilePath,
	IN ULONGLONG DataClusterOffset,//数据区簇偏移量
	OUT PLCXL_BITMAP BitmapDirect
	)
{
	NTSTATUS status;
	IO_STATUS_BLOCK IoStatusBlock;
	UNICODE_STRING FullFilePath;
	WCHAR FullFilePathBuf[MAX_PATH+1];
	HANDLE hFile;
	OBJECT_ATTRIBUTES oa;

	PAGED_CODE();
	ASSERT(fsdevext != NULL);
	ASSERT(FilePath != NULL);
	ASSERT(BitmapDirect!=NULL);

	//合并路径	
	RtlInitEmptyUnicodeString(&FullFilePath, FullFilePathBuf, sizeof(FullFilePathBuf));
	RtlCopyUnicodeString(&FullFilePath, &((PEP_DEVICE_EXTENSION)fsdevext->DE_FILTER.DE_FSVOL.DiskVolFilter->DeviceExtension)->DE_FILTER.PhysicalDeviceName);
	RtlAppendUnicodeStringToString(&FullFilePath, FilePath);
	InitializeObjectAttributes(&oa, &FullFilePath, OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, NULL);
	//打开要读取的文件
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
		fsdevext->DE_FILTER.PhysicalDeviceObject
		);
	if (NT_SUCCESS(status)) {
		PRETRIEVAL_POINTERS_BUFFER FileClusters;
		//获得簇列表
		status = GetFileClusterList(hFile, &FileClusters);
		if (NT_SUCCESS(status)) {
			LARGE_INTEGER PrevVCN;
			//自己定义的数据结构，用来快速读写
			PLCXL_VCN_EXTENT pExtent;
			DWORD i;

			//加入到直接读写位图中
			PrevVCN = FileClusters->StartingVcn;
			for (i = 0, pExtent = (PLCXL_VCN_EXTENT)&FileClusters->Extents[i]; i < FileClusters->ExtentCount; i++, pExtent++) {
				KdPrint(("LCXL Setting:GetFileClusterList(%016I64x->%016I64x)\n", (ULONGLONG)pExtent->Lcn.QuadPart, (ULONGLONG)(pExtent->Lcn.QuadPart+pExtent->NextVcn.QuadPart-PrevVCN.QuadPart-1)));
				//写入到保护位图中
				LCXLBitmapSet(BitmapDirect, (ULONGLONG)pExtent->Lcn.QuadPart+DataClusterOffset, (ULONGLONG)(pExtent->NextVcn.QuadPart-PrevVCN.QuadPart), TRUE);
				//获取下一个族列表的开始
				PrevVCN = pExtent->NextVcn;
			}
			//释放内存
			ExFreePool(FileClusters);
		} else {
			KdPrint(("SYS:FSetDirectRWFile:GetFileClusterList(%wZ) failed(0x%08x)\n", FilePath, status));
		}
		ZwClose(hFile);
	} else {
		KdPrint(("SYS:FSetDirectRWFile:ObOpenObjectByPointer(0x%08x) failed\n", status));
	}
	return status;
}
