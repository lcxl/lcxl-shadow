#include "lcxl_shadow.h"

BOOL WINAPI DI_IsDriverRunning()
{
	HANDLE hDevice;

	hDevice = DI_Open();
	if (hDevice != INVALID_HANDLE_VALUE) {
		CloseHandle(hDevice);
		return TRUE;
	} 
	return FALSE;
}

BOOL WINAPI DI_InShadowMode()
{
	HANDLE hDevice;

	hDevice = DI_Open();
	if (hDevice != INVALID_HANDLE_VALUE) {
		DWORD cbout;
		BOOL isShadowMode = DeviceIoControl(hDevice, IOCTL_IN_SHADOW_MODE, NULL, 0, NULL, 0,
			&cbout, NULL);
		CloseHandle(hDevice);
		return isShadowMode;
	}
	return FALSE;
}

HANDLE WINAPI DI_Open()
{
	return CreateFile(DRIVER_SYMBOL_LINK, GENERIC_READ | GENERIC_WRITE, 0,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

BOOL WINAPI DI_Close(HANDLE hDevice)
{
	return CloseHandle(hDevice);
}

BOOL WINAPI DI_GetRandomId(HANDLE hDevice, OUT LPVOID DriverId, IN OUT LPDWORD DriverIdLen)
{
	return DeviceIoControl(hDevice, IOCTL_GET_RANDOM_ID, NULL, 0, DriverId,
		*DriverIdLen, DriverIdLen, NULL);
}

BOOL WINAPI DI_ValidateCode(HANDLE hDevice, IN LPVOID MachineCode, DWORD MachineCodeLen)
{
	DWORD cbout;

	return DeviceIoControl(hDevice, IOCTL_VALIDATE_CODE, MachineCode,
		MachineCodeLen, NULL, 0, &cbout, NULL);
}

BOOL WINAPI DI_GetDiskList(HANDLE hDevice, OUT PVOLUME_DISK_INFO DiskList, IN OUT LPDWORD DiskListLen)
{
	DWORD cbout;

	if (DeviceIoControl(hDevice, IOCTL_GET_DISK_LIST, NULL, 0, DiskList, *DiskListLen * sizeof(VOLUME_DISK_INFO), &cbout, NULL)) {
		*DiskListLen = cbout / sizeof(VOLUME_DISK_INFO);
		return TRUE;
	}
	return FALSE;
}

BOOL WINAPI DI_GetDriverSetting(HANDLE hDevice, OUT PAPP_DRIVER_SETTING DriverSetting)
{
	DWORD outlen;

	outlen = sizeof(APP_DRIVER_SETTING);
	return DeviceIoControl(hDevice, IOCTL_GET_DRIVER_SETTING, NULL, 0,
		DriverSetting, outlen, &outlen, NULL);
}

BOOL WINAPI DI_SetNextRebootShadowMode(HANDLE hDevice, LCXL_SHADOW_MODE NextShadowMode)
{
	DWORD outlen;

	return DeviceIoControl(hDevice, IOCTL_SET_NEXT_REBOOT_SHADOW_MODE,
		&NextShadowMode, sizeof(NextShadowMode), NULL, 0, &outlen, NULL);
}

BOOL WINAPI DI_GetCustomDiskList(HANDLE hDevice, OUT PWCHAR CustomDiskList, IN OUT LPDWORD CustomDiskListLen)
{
	DWORD outlen;
	if (DeviceIoControl(hDevice, IOCTL_GET_CUSTOM_DISK_LIST,
			NULL, 0, CustomDiskList, *CustomDiskListLen*MAX_DEVNAME_LENGTH *sizeof(WCHAR),
			&outlen, NULL)) {
		*CustomDiskListLen = outlen / (MAX_DEVNAME_LENGTH * sizeof(WCHAR));
		return TRUE;
	}
	return FALSE;
}

BOOL WINAPI DI_SetCustomDiskList(HANDLE hDevice, IN PWCHAR CustomDiskList, DWORD CustomDiskListLen)
{
	DWORD outlen;

	return DeviceIoControl(hDevice, IOCTL_SET_CUSTOM_DISK_LIST,
		CustomDiskList, CustomDiskListLen*MAX_DEVNAME_LENGTH *sizeof(WCHAR),
		NULL, 0, &outlen, NULL);
}

BOOL WINAPI DI_AppRequest(HANDLE hDevice, IN OUT PAPP_REQUEST AppReq)
{
	DWORD outlen;

	return DeviceIoControl(hDevice, IOCTL_APP_RQUESET,
			AppReq, sizeof(APP_REQUEST), AppReq, sizeof(APP_REQUEST), &outlen, NULL);
}

BOOL WINAPI DI_StartShadowMode(HANDLE hDevice, LCXL_SHADOW_MODE CurShadowMode)
{
	DWORD outlen;

	return DeviceIoControl(hDevice, IOCTL_START_SHADOW_MODE,
			&CurShadowMode, sizeof(CurShadowMode), NULL, 0, &outlen, NULL);
}

BOOL WINAPI DI_SetSaveDataWhenShutdown(HANDLE hDevice, IN PSAVE_DATA_WHEN_SHUTDOWN SaveData)
{
	DWORD outlen;

	return DeviceIoControl(hDevice, IOCTL_SET_SAVE_DATA_WHEN_SHUTDOWN,
			&SaveData, sizeof(SAVE_DATA_WHEN_SHUTDOWN), NULL, 0, &outlen, NULL);
}

BOOL WINAPI DI_VerifyPassword(HANDLE hDevice, IN PWCHAR PasswordMD5, DWORD PasswordMD5Len)
{
	DWORD outlen;

	return DeviceIoControl(hDevice, IOCTL_VERIFY_PASSWORD,
			PasswordMD5, PasswordMD5Len, NULL, 0, &outlen, NULL);

}

BOOL WINAPI DI_SetPassword(HANDLE hDevice, IN PWCHAR PasswordMD5, DWORD PasswordMD5Len)
{
	DWORD outlen;

	return DeviceIoControl(hDevice, IOCTL_SET_PASSWORD,
			PasswordMD5, PasswordMD5Len, NULL, 0, &outlen, NULL);
}

BOOL APIENTRY DllMain(HANDLE hModule,  DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}