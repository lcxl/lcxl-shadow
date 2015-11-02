#ifndef _DLL_INTERFACE_H_
#define _DLL_INTERFACE_H_

#include "../LCXLShadowDriver/driverinterface.h"

BOOL WINAPI DI_IsDriverRunning();
BOOL WINAPI DI_InShadowMode();
HANDLE WINAPI DI_Open();
BOOL WINAPI DI_Close(HANDLE hDevice);
BOOL WINAPI DI_GetRandomId(HANDLE hDevice, OUT LPVOID DriverId, IN OUT LPDWORD DriverIdLen);
BOOL WINAPI DI_ValidateCode(HANDLE hDevice, IN LPVOID MachineCode, DWORD MachineCodeLen);
BOOL WINAPI DI_GetDiskList(HANDLE hDevice, OUT PVOLUME_DISK_INFO DiskList, IN OUT LPDWORD DiskListLen);
BOOL WINAPI DI_GetDriverSetting(HANDLE hDevice, OUT PAPP_DRIVER_SETTING DriverSetting);
BOOL WINAPI DI_SetNextRebootShadowMode(HANDLE hDevice, LCXL_SHADOW_MODE NextShadowMode);
BOOL WINAPI DI_GetCustomDiskList(HANDLE hDevice, OUT PWCHAR CustomDiskList, IN OUT LPDWORD CustomDiskListLen);
BOOL WINAPI DI_SetCustomDiskList(HANDLE hDevice, IN PWCHAR CustomDiskList, DWORD CustomDiskListLen);
BOOL WINAPI DI_AppRequest(HANDLE hDevice, IN OUT PAPP_REQUEST AppReq);
BOOL WINAPI DI_StartShadowMode(HANDLE hDevice, LCXL_SHADOW_MODE CurShadowMode);
BOOL WINAPI DI_SetSaveDataWhenShutdown(HANDLE hDevice, IN PSAVE_DATA_WHEN_SHUTDOWN SaveData);
BOOL WINAPI DI_VerifyPassword(HANDLE hDevice, IN PWCHAR PasswordMD5, DWORD PasswordMD5Len);
BOOL WINAPI DI_SetPassword(HANDLE hDevice, IN PWCHAR PasswordMD5, DWORD PasswordMD5Len);

#endif