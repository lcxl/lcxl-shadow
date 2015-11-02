#include "winkernel.h"

//Çý¶¯Ð¶ÔØÀý³Ì
VOID DriverUnload(
	IN PDRIVER_OBJECT DriverObject
	)
{
	UNREFERENCED_PARAMETER(DriverObject);
}

NTSTATUS DriverEntry (
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
	)
{
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);
	
	DriverObject->DriverUnload = DriverUnload;
	return STATUS_SUCCESS;
}

