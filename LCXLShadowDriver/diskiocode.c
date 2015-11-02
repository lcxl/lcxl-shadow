#include "winkernel.h"
#include "diskiocode.h"

// 向驱动发“IDENTIFY DEVICE”命令，获得设备信息
// hDevice: 设备句柄
// pIdInfo:  设备信息结构指针
NTSTATUS DiskIdentifyDevice(
	IN PDEVICE_OBJECT DiskPDO, 
	IN PIDINFO pIdInfo
	)
{
	PSENDCMDINPARAMS pSCIP;		// 输入数据结构指针
	PSENDCMDOUTPARAMS pSCOP;	// 输出数据结构指针
	KEVENT event;
	IO_STATUS_BLOCK iostatus;
	PIRP irp;
	NTSTATUS status;				// IOCTL返回值

	PAGED_CODE();

	// 申请输入/输出数据结构空间
	pSCIP = (PSENDCMDINPARAMS)ExAllocatePoolWithTag(PagedPool, sizeof(SENDCMDINPARAMS), 'DPDO');
	if (pSCIP==NULL) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	pSCOP = (PSENDCMDOUTPARAMS)ExAllocatePoolWithTag(PagedPool, sizeof(SENDCMDOUTPARAMS)+sizeof(IDINFO), 'DPDO');
	if (pSCOP==NULL) {
		ExFreePool(pSCOP);
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	RtlZeroMemory(pSCIP, sizeof(SENDCMDINPARAMS));
	RtlZeroMemory(pSCOP, sizeof(SENDCMDOUTPARAMS)+sizeof(IDINFO));
	
	// 指定ATA/ATAPI命令的寄存器值
	pSCIP->irDriveRegs.bCommandReg = IDE_ATA_IDENTIFY;

	// 指定输入/输出数据缓冲区大小
	pSCOP->cBufferSize = sizeof(IDINFO);

	KeInitializeEvent(&event, NotificationEvent, FALSE);
	// IDENTIFY DEVICE
	irp = IoBuildDeviceIoControlRequest(
		DFP_RECEIVE_DRIVE_DATA,
		DiskPDO,
		pSCIP,
		sizeof(SENDCMDINPARAMS),
		pSCOP,
		sizeof(SENDCMDOUTPARAMS) + sizeof(IDINFO),
		FALSE,
		&event,
		&iostatus);
	if (irp != NULL) {
		status = IoCallDriver(DiskPDO, irp);
		if (status == STATUS_PENDING) {
			KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
			status = iostatus.Status;
		}
		if (NT_SUCCESS(status)) {
			RtlCopyMemory(pIdInfo, pSCOP->bBuffer, sizeof(IDINFO));
		} else {
			KdPrint(("SYS:DDevicePnp DFP_RECEIVE_DRIVE_DATA failed(0x%08x)\n", status));
		}
	} else {
		//资源不足
		status = STATUS_INSUFFICIENT_RESOURCES;
	}
	// 释放输入/输出数据空间
	ExFreePool(pSCOP);
	ExFreePool(pSCIP);

	return status;
}