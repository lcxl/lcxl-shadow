#include "winkernel.h"

//禁用警告：使用了非标准扩展 : 非常量聚合初始值设定项
#pragma warning(disable:4204)
#define INFO_MEM_TAG 'kdD'

NTSTATUS CreateDllSection(IN IN PUNICODE_STRING pDllName, OUT PHANDLE pSection, OUT PPVOID pBaseAddress)
{
	HANDLE hFile;
	OBJECT_ATTRIBUTES oa = {sizeof oa, 0, pDllName, OBJ_CASE_INSENSITIVE};
	IO_STATUS_BLOCK iosb;
	SIZE_T size=0;
	NTSTATUS status;

	PAGED_CODE();

	ASSERT(pSection&&pBaseAddress);
	*pBaseAddress = NULL;
	status = ZwOpenFile(&hFile, FILE_EXECUTE | SYNCHRONIZE, &oa, &iosb, FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT);
	if (NT_SUCCESS(status)) {
		oa.ObjectName = 0;
		status = ZwCreateSection(pSection, SECTION_ALL_ACCESS, &oa, 0,PAGE_EXECUTE, SEC_IMAGE, hFile);
		if (NT_SUCCESS(status)) {
			status = ZwMapViewOfSection(*pSection, ZwCurrentProcess(), pBaseAddress, 0, 1000, 0, &size, (SECTION_INHERIT)1, MEM_TOP_DOWN, PAGE_READWRITE);
			if (!NT_SUCCESS(status)) {
				ZwClose(*pSection);
			}
		}
		ZwClose(hFile);
	}
	return status;
}

PVOID GetSectionDllFuncAddr(IN PCHAR lpFunctionName, IN PVOID BaseAddress) 
{
	PVOID functionAddress = NULL;
	PIMAGE_DOS_HEADER dosheader;
	PIMAGE_OPTIONAL_HEADER opthdr;
	PIMAGE_EXPORT_DIRECTORY pExportTable;
	PDWORD arrayOfFunctionAddresses;
	PDWORD arrayOfFunctionNames;
	PWORD arrayOfFunctionOrdinals;
	DWORD Base;
	STRING ntFunctionName, ntFunctionNameSearch;
	PCHAR functionName;
	DWORD functionOrdinal;
	ULONG x;

	ASSERT(lpFunctionName&&BaseAddress);
	dosheader = (PIMAGE_DOS_HEADER)BaseAddress;
	opthdr =(PIMAGE_OPTIONAL_HEADER) ((PBYTE)BaseAddress+dosheader->e_lfanew+24);
	pExportTable =(PIMAGE_EXPORT_DIRECTORY)((PBYTE)BaseAddress + opthdr->DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXPORT]. VirtualAddress);
	// now we can get the exported functions, but note we convert from RVA to address
	arrayOfFunctionAddresses = (PDWORD)( (PBYTE)BaseAddress + pExportTable->AddressOfFunctions);
	arrayOfFunctionNames = (PDWORD)( (PBYTE)BaseAddress + pExportTable->AddressOfNames);
	arrayOfFunctionOrdinals = (PWORD)( (PBYTE)BaseAddress + pExportTable->AddressOfNameOrdinals);
	Base = pExportTable->Base;
	RtlInitString(&ntFunctionNameSearch, lpFunctionName);
	for (x = 0; x < pExportTable->NumberOfFunctions; x++) {
		functionName = (PCHAR)( (PBYTE)BaseAddress + arrayOfFunctionNames[x]);
		RtlInitString(&ntFunctionName, functionName);
		functionOrdinal = arrayOfFunctionOrdinals[x] + Base - 1; // always need to add base, -1 as array counts from 0
		// this is the funny bit.  you would expect the function pointer to simply be arrayOfFunctionAddresses[x]...
		// oh no... thats too simple.  it is actually arrayOfFunctionAddresses[functionOrdinal]!!
		if (RtlCompareString(&ntFunctionName, &ntFunctionNameSearch, TRUE) == 0) {
			functionAddress = (PBYTE)BaseAddress + arrayOfFunctionAddresses[functionOrdinal];
			break;
		}
	}
	return functionAddress;
}

PVOID GetInfoTable(IN SYSTEM_INFORMATION_CLASS ATableType)
{
	ULONG mSize = 0x2000;
	PVOID mPtr = NULL;
	NTSTATUS status;

	PAGED_CODE();

	do {
		mPtr = ExAllocatePoolWithTag( PagedPool, mSize, INFO_MEM_TAG);
		if (mPtr) {
			status = ZwQuerySystemInformation(ATableType, mPtr, mSize, &mSize); 
		} else {
			return NULL;
		}
		if (!NT_SUCCESS(status)) {
			ExFreePoolWithTag( mPtr, INFO_MEM_TAG);
			KdPrint(("SYS:GetInfoTable:The buf len is too small,need size %d\n", mSize));
		}
	} while (status == STATUS_INFO_LENGTH_MISMATCH);
	if (NT_SUCCESS(status)) {
		return mPtr;
	} else {
		KdPrint(("SYS:GetInfoTable Fail(Error:0x%08x)", status));
		return NULL;
	}
}

NTSTATUS IRPCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is invoked for the completion of an FsControl request.  It
    signals an event used to re-sync back to the dispatch routine.

Arguments:

    DeviceObject - Pointer to this driver's device object that was attached to
            the file system device object

    Irp - Pointer to the IRP that was just completed.

    Context - Pointer to the event to signal

--*/

{
    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );

    ASSERT(Context != NULL);

    //
    //  On Windows XP or later, the context passed in will be an event
    //  to signal.
    //

    KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

//等待IRP在下层设备的完成
//返回的是IRP的status值
NTSTATUS LLWaitIRPCompletion(IN PDEVICE_OBJECT TargetDeviceObject, IN OUT PIRP Irp)
{
	KEVENT waitEvent;
	NTSTATUS status;

	PAGED_CODE();
	
	KeInitializeEvent( &waitEvent, 
		NotificationEvent, 
		FALSE );

	IoCopyCurrentIrpStackLocationToNext ( Irp );

	IoSetCompletionRoutine( Irp,
		IRPCompletion,
		&waitEvent,     //context parameter
		TRUE,
		TRUE,
		TRUE );

	status = IoCallDriver(TargetDeviceObject, Irp );

	//
	//  Wait for the operation to complete
	//

	if (STATUS_PENDING == status) {

		status = KeWaitForSingleObject( &waitEvent,
			Executive,
			KernelMode,
			FALSE,
			NULL );
		ASSERT( STATUS_SUCCESS == status );
	}

	//
	//  Verify the IoCompleteRequest was called
	//

	ASSERT(KeReadStateEvent(&waitEvent) ||
		!NT_SUCCESS(Irp->IoStatus.Status));
	return Irp->IoStatus.Status;
}

NTSTATUS
	IoCompletionRoutine(
	IN PDEVICE_OBJECT  DeviceObject,
	IN PIRP  Irp,
	IN PVOID  Context
	)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Context);

	PAGED_CODE();

	KdPrint(("SYS(%d:%d):IoCompletionRoutine!\n", PsGetCurrentProcessId(), PsGetCurrentThreadId()));
	*Irp->UserIosb = Irp->IoStatus;

	if (Irp->UserEvent)
		KeSetEvent(Irp->UserEvent, IO_NO_INCREMENT, 0);

	if (Irp->MdlAddress)
	{
		IoFreeMdl(Irp->MdlAddress);
		Irp->MdlAddress = NULL;
	}

	IoFreeIrp(Irp);

	return STATUS_MORE_PROCESSING_REQUIRED;
}

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
	)
{
	NTSTATUS status;
	KEVENT event;
	PIRP irp;
	PIO_STACK_LOCATION irpSp;
	IO_SECURITY_CONTEXT securityContext;
	ACCESS_STATE accessState;
	OBJECT_ATTRIBUTES objectAttributes;
	PFILE_OBJECT fileObject;
	AUX_ACCESS_DATA auxData;

	PAGED_CODE();

	RtlZeroMemory(&auxData, sizeof(AUX_ACCESS_DATA));
	
	InitializeObjectAttributes(&objectAttributes, NULL, OBJ_CASE_INSENSITIVE| OBJ_KERNEL_HANDLE, 0, NULL);

	status = ObCreateObject(KernelMode,
		*IoFileObjectType,
		&objectAttributes,
		KernelMode,
		NULL,
		sizeof(FILE_OBJECT),
		0,
		0,
		(PVOID *)&fileObject);

	if (!NT_SUCCESS(status)) {
		return status;
	}

	RtlZeroMemory(fileObject, sizeof(FILE_OBJECT));
	fileObject->Type = IO_TYPE_FILE;
	fileObject->Size = sizeof(FILE_OBJECT);
	fileObject->DeviceObject = RealDevice;
	//	fileObject->RelatedFileObject = NULL;
	fileObject->Flags = FO_SYNCHRONOUS_IO;
	fileObject->FileName.MaximumLength = FileName->MaximumLength;
	fileObject->FileName.Buffer = (PWCH)ExAllocatePoolWithTag(NonPagedPool, FileName->MaximumLength, 'File');
	//fileObject->FileObjectExtension
	if (fileObject->FileName.Buffer == NULL) {
		ObDereferenceObject(fileObject);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RtlCopyUnicodeString(&fileObject->FileName, FileName);
	KeInitializeEvent(&fileObject->Lock, SynchronizationEvent, FALSE);
	KeInitializeEvent(&fileObject->Event, NotificationEvent, FALSE);

	irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
	
	if (irp == NULL) {
		ObDereferenceObject(fileObject);
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	KeInitializeEvent(&event, SynchronizationEvent, FALSE);

	irp->MdlAddress = NULL;
	irp->Flags |= IRP_CREATE_OPERATION | IRP_SYNCHRONOUS_API;
	irp->RequestorMode = KernelMode;
	irp->UserIosb = IoStatusBlock;
	////LCXL:CHANGE
	irp->UserEvent = &event;
	//irp->UserEvent = NULL;
	irp->PendingReturned = FALSE;
	irp->Cancel = FALSE;
	irp->CancelRoutine = NULL;
	irp->Tail.Overlay.Thread = (PETHREAD)KeGetCurrentThread();
	irp->Tail.Overlay.AuxiliaryBuffer = NULL;
	irp->Tail.Overlay.OriginalFileObject = fileObject;

	status = SeCreateAccessState(&accessState,
		&auxData,
		DesiredAccess,
		IoGetFileObjectGenericMapping());

	if (!NT_SUCCESS(status)) {
		IoFreeIrp(irp);
		ExFreePool(fileObject->FileName.Buffer);
		ObDereferenceObject(fileObject);
		return status;
	}

	securityContext.SecurityQos = NULL;
	securityContext.AccessState = &accessState;
	securityContext.DesiredAccess = DesiredAccess;
	securityContext.FullCreateOptions = 0;

	irpSp = IoGetNextIrpStackLocation(irp);
	irpSp->MajorFunction = IRP_MJ_CREATE;
	irpSp->DeviceObject = DeviceObject;
	irpSp->FileObject = fileObject;
	irpSp->Parameters.Create.SecurityContext = &securityContext;
	irpSp->Parameters.Create.Options = (CreateDisposition << 24) | CreateOptions;
	irpSp->Parameters.Create.FileAttributes = (USHORT)FileAttributes;
	irpSp->Parameters.Create.ShareAccess = (USHORT)ShareAccess;
	irpSp->Parameters.Create.EaLength = 0;

	IoSetCompletionRoutine(irp, IoCompletionRoutine, NULL, TRUE, TRUE, TRUE);
	status = IoCallDriver(DeviceObject, irp);

	if (status == STATUS_PENDING) {
		KeWaitForSingleObject(&event, Executive, KernelMode, TRUE, NULL);
	}

	status = IoStatusBlock->Status;

	if (!NT_SUCCESS(status)) {
		ExFreePool(fileObject->FileName.Buffer);
		fileObject->FileName.Length = 0;
		fileObject->DeviceObject = NULL;
		ObDereferenceObject(fileObject);
	} else {
		InterlockedIncrement(&fileObject->DeviceObject->ReferenceCount);
		if (fileObject->Vpb) {
			InterlockedIncrement((PLONG)&fileObject->Vpb->ReferenceCount);
		}
		*Object = fileObject;
		KdPrint(("IrpCreateFile:Open file success! object = %x\n", fileObject));
	}

	return status;
}

NTSTATUS
	CompleteAndSetEvent(
	IN PDEVICE_OBJECT  DeviceObject,
	IN PIRP  Irp,
	IN PVOID  Context
	)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Context);
	DbgPrint(("CompleteAndSetEvent!\n"));
	//*Irp->UserIosb = Irp->IoStatus;

	KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, 0);

	if (Irp->MdlAddress)
	{
		IoFreeMdl(Irp->MdlAddress);
		Irp->MdlAddress = NULL;
	}

	return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
	IrpCloseFile(
	IN PDEVICE_OBJECT DeviceObject,
	IN PFILE_OBJECT FileObject
	)
{
	NTSTATUS status;
	KEVENT event;
	PIRP irp;
	PVPB vpb;
	IO_STATUS_BLOCK ioStatusBlock;
	PIO_STACK_LOCATION irpSp;

	PAGED_CODE();

	irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);

	if (irp == NULL) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	KeInitializeEvent(&event, SynchronizationEvent, FALSE);

	irp->Tail.Overlay.OriginalFileObject = FileObject;
	irp->Tail.Overlay.Thread = PsGetCurrentThread();
	irp->RequestorMode = KernelMode;
	irp->UserEvent = &event;
	irp->UserIosb = &irp->IoStatus;
	irp->Overlay.AsynchronousParameters.UserApcRoutine = (PIO_APC_ROUTINE)NULL;
	irp->Flags = IRP_SYNCHRONOUS_API | IRP_CLOSE_OPERATION;

	irpSp = IoGetNextIrpStackLocation(irp);
	irpSp->MajorFunction = IRP_MJ_CLEANUP;
	irpSp->FileObject = FileObject;

	status = IoCallDriver(DeviceObject, irp);

	if (status == STATUS_PENDING) {
		KeWaitForSingleObject(&event,
			UserRequest,
			KernelMode,
			FALSE,
			NULL);
	}

	IoReuseIrp(irp , STATUS_SUCCESS);
	KeClearEvent(&event);

	irpSp = IoGetNextIrpStackLocation(irp);
	irpSp->MajorFunction = IRP_MJ_CLOSE;
	irpSp->FileObject = FileObject;

	irp->UserIosb = &ioStatusBlock;
	irp->UserEvent = &event;
	irp->Tail.Overlay.OriginalFileObject = FileObject;
	irp->Tail.Overlay.Thread = PsGetCurrentThread();
	irp->AssociatedIrp.SystemBuffer = (PVOID)NULL;
	irp->Flags = IRP_CLOSE_OPERATION | IRP_SYNCHRONOUS_API;

	vpb = FileObject->Vpb;

	if (vpb && !(FileObject->Flags & FO_DIRECT_DEVICE_OPEN)) {
		InterlockedDecrement((PLONG)&vpb->ReferenceCount);
		FileObject->Flags |= FO_FILE_OPEN_CANCELLED;
	}

	status = IoCallDriver(DeviceObject, irp);

	if (status == STATUS_PENDING) {
		KeWaitForSingleObject(	&event,
			Executive,
			KernelMode,
			FALSE,
			NULL);
	}
	//LCXL:ADD-
	InterlockedIncrement(&FileObject->DeviceObject->ReferenceCount);
	//ObDereferenceObject(FileObject);
	//LCXL:Del
	IoFreeIrp(irp);

	return status;
}

NTSTATUS
	IrpReadFile(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER ByteOffset OPTIONAL,
	IN ULONG Length,
	OUT PVOID Buffer,
	OUT PIO_STATUS_BLOCK IoStatusBlock
	)
{
	NTSTATUS status;
	KEVENT event;
	PIRP irp;
	PIO_STACK_LOCATION irpSp;
	PDEVICE_OBJECT deviceObject;

	if (ByteOffset == NULL)
	{
		if (!(FileObject->Flags & FO_SYNCHRONOUS_IO))
			return STATUS_INVALID_PARAMETER;

		ByteOffset = &FileObject->CurrentByteOffset;
	}

	if (FileObject->Vpb == 0 || FileObject->Vpb->RealDevice == NULL)
		return STATUS_UNSUCCESSFUL;

	deviceObject = FileObject->Vpb->DeviceObject;
	irp = IoAllocateIrp(deviceObject->StackSize, FALSE);

	if (irp == NULL)
		return STATUS_INSUFFICIENT_RESOURCES;

	irp->MdlAddress = IoAllocateMdl(Buffer, Length, FALSE, TRUE, NULL);

	if (irp->MdlAddress == NULL)
	{
		IoFreeIrp(irp);
		return STATUS_INSUFFICIENT_RESOURCES;;
	}

	MmBuildMdlForNonPagedPool(irp->MdlAddress);

	irp->Flags = IRP_READ_OPERATION;
	irp->RequestorMode = KernelMode;
	irp->UserIosb = IoStatusBlock;
	irp->UserEvent = &event;
	irp->Tail.Overlay.Thread = (PETHREAD)KeGetCurrentThread();
	irp->Tail.Overlay.OriginalFileObject = FileObject;

	irpSp = IoGetNextIrpStackLocation(irp);
	irpSp->MajorFunction = IRP_MJ_READ;
	irpSp->MinorFunction = IRP_MN_NORMAL;
	irpSp->DeviceObject = deviceObject;
	irpSp->FileObject = FileObject;
	irpSp->Parameters.Read.Length = Length;
	irpSp->Parameters.Read.ByteOffset = *ByteOffset;

	KeInitializeEvent(&event, SynchronizationEvent, FALSE);
	IoSetCompletionRoutine(irp, IoCompletionRoutine, NULL, TRUE, TRUE, TRUE);
	status = IoCallDriver(deviceObject, irp);

	if (status == STATUS_PENDING)
		status = KeWaitForSingleObject(&event, Executive, KernelMode, TRUE, NULL);

	return status;
}

NTSTATUS
	IrpWriteFile(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER ByteOffset OPTIONAL,
	IN ULONG Length,
	IN PVOID Buffer,
	OUT PIO_STATUS_BLOCK IoStatusBlock
	)
{
	NTSTATUS status;
	KEVENT event;
	PIRP irp;
	PIO_STACK_LOCATION irpSp;
	PDEVICE_OBJECT deviceObject;

	if (ByteOffset == NULL)
	{
		if (!(FileObject->Flags & FO_SYNCHRONOUS_IO))
			return STATUS_INVALID_PARAMETER;

		ByteOffset = &FileObject->CurrentByteOffset;
	}

	if (FileObject->Vpb == 0 || FileObject->Vpb->RealDevice == NULL)
		return STATUS_UNSUCCESSFUL;

	deviceObject = FileObject->Vpb->DeviceObject;
	irp = IoAllocateIrp(deviceObject->StackSize, FALSE);

	if (irp == NULL)
		return STATUS_INSUFFICIENT_RESOURCES;

	irp->MdlAddress = IoAllocateMdl(Buffer, Length, FALSE, TRUE, NULL);

	if (irp->MdlAddress == NULL)
	{
		IoFreeIrp(irp);
		return STATUS_INSUFFICIENT_RESOURCES;;
	}

	MmBuildMdlForNonPagedPool(irp->MdlAddress);

	irp->Flags = IRP_WRITE_OPERATION;
	irp->RequestorMode = KernelMode;
	irp->UserIosb = IoStatusBlock;
	irp->UserEvent = &event;
	irp->Tail.Overlay.Thread = (PETHREAD)KeGetCurrentThread();
	irp->Tail.Overlay.OriginalFileObject = FileObject;

	irpSp = IoGetNextIrpStackLocation(irp);
	irpSp->MajorFunction = IRP_MJ_WRITE;
	irpSp->MinorFunction = IRP_MN_NORMAL;
	irpSp->DeviceObject = deviceObject;
	irpSp->FileObject = FileObject;
	irpSp->Parameters.Write.Length = Length;
	irpSp->Parameters.Write.ByteOffset = *ByteOffset;

	KeInitializeEvent(&event, SynchronizationEvent, FALSE);
	IoSetCompletionRoutine(irp, IoCompletionRoutine, NULL, TRUE, TRUE, TRUE);
	status = IoCallDriver(deviceObject, irp);

	if (status == STATUS_PENDING)
		status = KeWaitForSingleObject(&event, Executive, KernelMode, TRUE, NULL);

	return status;
}

NTSTATUS
	IrpQueryFile(
	IN PFILE_OBJECT FileObject,
	OUT PVOID FileInformation,
	IN ULONG Length,
	IN FILE_INFORMATION_CLASS FileInformationClass
	)
{
	NTSTATUS status;
	KEVENT event;
	PIRP irp;
	IO_STATUS_BLOCK ioStatus;
	PIO_STACK_LOCATION irpSp;
	PDEVICE_OBJECT deviceObject;

	if (FileObject->Vpb == 0 || FileObject->Vpb->RealDevice == NULL)
		return STATUS_UNSUCCESSFUL;

	deviceObject = FileObject->Vpb->DeviceObject;
	KeInitializeEvent(&event, SynchronizationEvent, FALSE);
	irp = IoAllocateIrp(deviceObject->StackSize, FALSE);

	if (irp == NULL)
		return STATUS_INSUFFICIENT_RESOURCES;

	irp->Flags = IRP_BUFFERED_IO;
	irp->AssociatedIrp.SystemBuffer = FileInformation;
	irp->RequestorMode = KernelMode;
	irp->Overlay.AsynchronousParameters.UserApcRoutine = (PIO_APC_ROUTINE)NULL;
	irp->UserEvent = &event;
	irp->UserIosb = &ioStatus;
	irp->Tail.Overlay.Thread = (PETHREAD)KeGetCurrentThread();
	irp->Tail.Overlay.OriginalFileObject = FileObject;

	irpSp = IoGetNextIrpStackLocation(irp);
	irpSp->MajorFunction = IRP_MJ_QUERY_INFORMATION;
	irpSp->DeviceObject = deviceObject;
	irpSp->FileObject = FileObject;
	irpSp->Parameters.QueryFile.Length = Length;
	irpSp->Parameters.QueryFile.FileInformationClass = FileInformationClass;

	IoSetCompletionRoutine(irp, IoCompletionRoutine, NULL, TRUE, TRUE, TRUE);
	status = IoCallDriver(deviceObject, irp);

	if (status == STATUS_PENDING)
		KeWaitForSingleObject(&event, Executive, KernelMode, TRUE, NULL);

	return ioStatus.Status;
}
/*
NTSTATUS
	IrpQueryDirectory(
	IN PFILE_OBJECT FileObject,
	IN FILE_INFORMATION_CLASS FileInformationClass,
	OUT PVOID Buffer,
	IN ULONG Length
	)
{
	NTSTATUS status;
	KEVENT event;
	PIRP irp;
	IO_STATUS_BLOCK ioStatus;
	PIO_STACK_LOCATION irpSp;
	PDEVICE_OBJECT deviceObject;
	PQUERY_DIRECTORY queryDirectory;

	if (FileObject->Vpb == 0 || FileObject->Vpb->RealDevice == NULL)
		return STATUS_UNSUCCESSFUL;

	deviceObject = FileObject->Vpb->DeviceObject;
	KeInitializeEvent(&event, SynchronizationEvent, FALSE);
	irp = IoAllocateIrp(deviceObject->StackSize, FALSE);
		if (irp == NULL)
		return STATUS_INSUFFICIENT_RESOURCES;

	irp->Flags = IRP_INPUT_OPERATION | IRP_BUFFERED_IO;
	irp->RequestorMode = KernelMode;
	irp->UserEvent = &event;
	irp->UserIosb = &ioStatus;
	irp->UserBuffer = Buffer;
	irp->Tail.Overlay.Thread = (PETHREAD)KeGetCurrentThread();
	irp->Tail.Overlay.OriginalFileObject = FileObject;
	irp->Overlay.AsynchronousParameters.UserApcRoutine = (PIO_APC_ROUTINE)NULL;
	//irp->Pointer = FileObject;

	irpSp = IoGetNextIrpStackLocation(irp);
	irpSp->MajorFunction = IRP_MJ_DIRECTORY_CONTROL;
	irpSp->MinorFunction = IRP_MN_QUERY_DIRECTORY;
	irpSp->DeviceObject = deviceObject;
	irpSp->FileObject = FileObject;

	queryDirectory = (PQUERY_DIRECTORY)&irpSp->Parameters;
	queryDirectory->Length = Length;
	queryDirectory->FileName = NULL;
	queryDirectory->FileInformationClass = FileInformationClass;
	queryDirectory->FileIndex = 0;

	IoSetCompletionRoutine(irp, IoCompletionRoutine, NULL, TRUE, TRUE, TRUE);
	status = IoCallDriver(deviceObject, irp);

	if (status == STATUS_PENDING)
	{
		KeWaitForSingleObject(&event, Executive, KernelMode, TRUE, NULL);
		status = ioStatus.Status;
	}

	return status;
}
*/

NTSTATUS GetFileClusterList(
	IN HANDLE hFile, 
	OUT PRETRIEVAL_POINTERS_BUFFER *FileClusters)
{

	NTSTATUS status;
	IO_STATUS_BLOCK iosb;
	LARGE_INTEGER StartVcn;
	PRETRIEVAL_POINTERS_BUFFER pVcnPairs;
	ULONG ulOutPutSize;
	ULONG uCounts;

	PAGED_CODE();
	ASSERT(FileClusters != NULL);
	//FSCTL_GET_NTFS_FILE_RECORD
	uCounts = 0;
	StartVcn.QuadPart=0;
	do {
		uCounts+=200;
		pVcnPairs = NULL;
		ulOutPutSize = sizeof(RETRIEVAL_POINTERS_BUFFER) + uCounts* sizeof(pVcnPairs->Extents);
		pVcnPairs = (PRETRIEVAL_POINTERS_BUFFER)ExAllocatePoolWithTag(PagedPool, ulOutPutSize, 'FVCN');
		if(pVcnPairs == NULL) {
			return STATUS_INSUFFICIENT_RESOURCES;
		}
		status = ZwFsControlFile( hFile,NULL, NULL, 0, &iosb,
			FSCTL_GET_RETRIEVAL_POINTERS,
			&StartVcn, sizeof(StartVcn),
			pVcnPairs, ulOutPutSize );
		if (!NT_SUCCESS(status)) {
			ExFreePool(pVcnPairs);
			KdPrint(("SYS:GetFileClusterList:ZwFsControlFile(size=%d) Failed(0x%08x)\n", ulOutPutSize, status));
		} else {
			*FileClusters = pVcnPairs;
		}
	} while (status == STATUS_BUFFER_OVERFLOW);
	return status;
}

//获取csrss.exe进程
NTSTATUS GetCsrssPid(HANDLE *CsrssPid)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	HANDLE Process, hObject;
	ULONG CsrId = 0;
	OBJECT_ATTRIBUTES obj;
	CLIENT_ID cid;
	POBJECT_NAME_INFORMATION ObjName;
	UNICODE_STRING ApiPortName;
	PSYSTEM_HANDLE_INFORMATION_EX Handles;
	ULONG i;

	PAGED_CODE();

	RtlInitUnicodeString(&ApiPortName, L"\\Windows\\ApiPort");

	Handles = (PSYSTEM_HANDLE_INFORMATION_EX)GetInfoTable( SystemHandleInformation );
	if( Handles == NULL ) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	ObjName = (POBJECT_NAME_INFORMATION)ExAllocatePoolWithTag( PagedPool, 0x2000,  INFO_MEM_TAG);
	KdPrint(("SYS: Number of handles %d\n", Handles->NumberOfHandles));
	for(i = 0; i < Handles->NumberOfHandles; i++) {  
		//打开的对象的类型是否为21 Port object
		if (Handles->Information[i].ObjectTypeNumber == 21) {
			InitializeObjectAttributes(&obj, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
			cid.UniqueProcess = (HANDLE)Handles->Information[i].ProcessId;
			cid.UniqueThread  = 0;

			ntStatus = ZwOpenProcess(&Process, PROCESS_DUP_HANDLE, &obj, &cid);
			if(NT_SUCCESS(ntStatus)) {
				ntStatus = ZwDuplicateObject(
					Process, 
					(HANDLE)Handles->Information[i].Handle,
					NtCurrentProcess(), 
					&hObject, 
					0,
					0, 
					DUPLICATE_SAME_ACCESS);
				if(NT_SUCCESS(ntStatus)){
					ntStatus = ZwQueryObject(hObject, (OBJECT_INFORMATION_CLASS)ObjectNameInformation, ObjName, 0x2000, NULL);
					if(NT_SUCCESS(ntStatus)) {
						if (ObjName->Name.Buffer != NULL) {
							if (RtlCompareUnicodeString(&ApiPortName, &ObjName->Name, TRUE) == 0) {
								KdPrint(("SYS: Csrss PID:%d\n", Handles->Information[i].ProcessId));
								KdPrint(("SYS: Csrss Port - %wZ\n", &ObjName->Name));
								CsrId = Handles->Information[i].ProcessId;
							}
						}
						
					} else {
						KdPrint(("SYS: Error in Query Object\n"));
					}
					ZwClose(hObject);
				} else {
					KdPrint(("SYS: Error on duplicating object\n"));
				}
				ZwClose(Process);
			} else {
				KdPrint(("SYS: Could not open process\n"));
			}
		}
	}
	ExFreePoolWithTag( Handles, INFO_MEM_TAG);
	ExFreePoolWithTag(ObjName, INFO_MEM_TAG);
	*CsrssPid = (HANDLE)CsrId;
	return ntStatus;
}

//多读互斥体
typedef struct _MultiReadMutex {
	int iReadCount;//读线程
	FAST_MUTEX z;//Z队列
	FAST_MUTEX kReadMutex;//
	KMUTEX kRWMutex;//读写互斥体
} MultiReadMutex, *PMultiReadMutex;

VOID LockLcxlMutexRead(PMultiReadMutex pMutex)//加锁读互斥
{
	ExAcquireFastMutex(&pMutex->z);
	ExAcquireFastMutex(&pMutex->kReadMutex);
	if (pMutex->iReadCount == 0) {
		KeWaitForSingleObject(&pMutex->kRWMutex, Executive, KernelMode, FALSE, NULL);
	}
	pMutex->iReadCount++;
	ExReleaseFastMutex(&pMutex->kReadMutex);
	ExReleaseFastMutex(&pMutex->z);
}

VOID UnlockLcxlMutexRead(PMultiReadMutex pMutex)//解锁读互斥
{
	ExAcquireFastMutex(&pMutex->kReadMutex);
	pMutex->iReadCount--;
	if (pMutex->iReadCount == 0) {
		KeReleaseMutex(&pMutex->kRWMutex, FALSE);
	}
	ExReleaseFastMutex(&pMutex->kReadMutex);
}

VOID LockLcxlMutexWrite(PMultiReadMutex pMutex)//加锁写互斥
{
	ExAcquireFastMutex(&pMutex->z);
	KeWaitForSingleObject(&pMutex->kRWMutex, Executive, KernelMode, FALSE, NULL);
}

VOID UnlockLcxlMutexWrite(PMultiReadMutex pMutex)//解锁写互斥
{
	KeReleaseMutex(&pMutex->kRWMutex, FALSE);
	ExReleaseFastMutex(&pMutex->z);
}