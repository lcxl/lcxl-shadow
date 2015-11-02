#include <ntifs.h>
#include <ntddk.h>
#include "notify.h"
//禁用警告：使用了非标准扩展 : 非常量聚合初始值设定项
#pragma warning(disable:4204)


NTSTATUS 
NTAPI 
ExRaiseHardError( 
				 IN NTSTATUS ErrorStatus, 
				 IN ULONG NumberOfParameters, 
				 IN ULONG UnicodeStringParameterMask, 
				 IN PVOID Parameters, 
				 IN ULONG ResponseOption, 
				 OUT PULONG Response );


ULONG 
kMessageBox (
	PUNICODE_STRING Message,
	PUNICODE_STRING Caption,
	ULONG ResponseOption,
	ULONG Type
	) 
{
	NTSTATUS		Status; 
	PVOID Parameters[] = {
		Message,
		Caption,
		(PVOID)(ResponseOption|Type), 
		0 
	}; 

	ULONG Response = 0; 

	Status = ExRaiseHardError ( 
		STATUS_SERVICE_NOTIFICATION|0x10000000, 
		3, // Number of parameters
		3, // Parameter mask -- first two are pointers
		&Parameters, 
		ResponseOption, 
		&Response 
		); 
	if (!NT_SUCCESS(Status)) {
		KdPrint(("SYS:kMessageBox:ExRaiseHardError Failed!0x08x\n", Status));
	}
	return Response; 
}