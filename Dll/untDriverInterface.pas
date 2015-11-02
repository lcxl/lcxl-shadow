unit untDriverInterface;

interface

uses
  SysUtils, Windows, untCommDataType;

const
  // 驱动符号
  DRIVER_SYMBOL_LINK = '\\.\LCXLShadow';

  // 与驱动保持一致----------------------------------------------------------------

const
  // 获取磁盘列表 $800
  IOCTL_GET_DISK_LIST = ((FILE_DEVICE_UNKNOWN shl 16) or
    (FILE_ANY_ACCESS shl 14) or ($0800 shl 2) or METHOD_BUFFERED);
  // 获取驱动设置 $801
  IOCTL_GET_DRIVER_SETTING = ((FILE_DEVICE_UNKNOWN shl 16) or
    (FILE_ANY_ACCESS shl 14) or ($0801 shl 2) or METHOD_BUFFERED);
  // 设置下次重启的影子模式 $802
  IOCTL_SET_NEXT_REBOOT_SHADOW_MODE = ((FILE_DEVICE_UNKNOWN shl 16) or
    (FILE_ANY_ACCESS shl 14) or ($0802 shl 2) or METHOD_BUFFERED);
  // 获取自定义保护磁盘列表
  IOCTL_GET_CUSTOM_DISK_LIST = ((FILE_DEVICE_UNKNOWN shl 16) or
    (FILE_ANY_ACCESS shl 14) or ($0803 shl 2) or METHOD_BUFFERED);
  // 设置自定义保护磁盘列表
  IOCTL_SET_CUSTOM_DISK_LIST = ((FILE_DEVICE_UNKNOWN shl 16) or
    (FILE_ANY_ACCESS shl 14) or ($0804 shl 2) or METHOD_BUFFERED);
  // 设置自定义保护磁盘列表
  IOCTL_APP_RQUESET = ((FILE_DEVICE_UNKNOWN shl 16) or
    (FILE_ANY_ACCESS shl 14) or ($0805 shl 2) or METHOD_BUFFERED);
  //启动影子模式
  IOCTL_START_SHADOW_MODE = ((FILE_DEVICE_UNKNOWN shl 16) or
    (FILE_ANY_ACCESS shl 14) or ($0806 shl 2) or METHOD_BUFFERED);
  //设置是否保存数据
  IOCTL_SET_SAVE_DATA_WHEN_SHUTDOWN = ((FILE_DEVICE_UNKNOWN shl 16) or
    (FILE_ANY_ACCESS shl 14) or ($0807 shl 2) or METHOD_BUFFERED);
  //设置是否处于影子模式
  IOCTL_IN_SHADOW_MODE = ((FILE_DEVICE_UNKNOWN shl 16) or
    (FILE_ANY_ACCESS shl 14) or ($0808 shl 2) or METHOD_BUFFERED);
  //获取随机ID号，此处为获取AES的key
  IOCTL_GET_RANDOM_ID = ((FILE_DEVICE_UNKNOWN shl 16) or
    (FILE_ANY_ACCESS shl 14) or ($0809 shl 2) or METHOD_BUFFERED);
  //验证，如果用IOCTL_GET_RANDOM_ID解密出来的是硬件序列号，则正确
  IOCTL_VALIDATE_CODE = ((FILE_DEVICE_UNKNOWN shl 16) or
    (FILE_ANY_ACCESS shl 14) or ($080A shl 2) or METHOD_BUFFERED);
  //验证密码
  IOCTL_VERIFY_PASSWORD = ((FILE_DEVICE_UNKNOWN shl 16) or
    (FILE_ANY_ACCESS shl 14) or ($080B shl 2) or METHOD_BUFFERED);
  //设置密码
  IOCTL_SET_PASSWORD = ((FILE_DEVICE_UNKNOWN shl 16) or
    (FILE_ANY_ACCESS shl 14) or ($080C shl 2) or METHOD_BUFFERED);

  // ------------------------------------------------------------------------------
//驱动是否正在运行
function DI_IsDriverRunning: BOOL; stdcall;
//设置是否处于影子模式
function DI_InShadowMode: BOOL; stdcall;
  // 打开驱动
function DI_Open: THandle; stdcall;

// 关闭驱动句柄
function DI_Close(hDevice: THandle): BOOL; stdcall;

// 获取驱动中的随机化ID
// 此ID用来验证程序的合法性
// 此ID将机器码（由硬件ID生成）进行加密以后发给驱动进行验证
function DI_GetRandomId(hDevice: THandle; DriverId: Pointer;
  var DriverIdLen: DWORD)
  : BOOL; stdcall;

// 验证加密后的机器码
function DI_ValidateCode(hDevice: THandle; MachineCode: Pointer;
  MachineCodeLen: DWORD): BOOL; stdcall;

// 获取磁盘列表
// DiskList: 磁盘列表，里面的存储状态为
function DI_GetDiskList(hDevice: THandle; DiskList: PVolumeDiskInfo;
  var DiskListLen: DWORD): BOOL; stdcall;

// 获取驱动设置
function DI_GetDriverSetting(hDevice: THandle;
  var DriverSetting: TAppDriverSetting): BOOL; stdcall;

// 设置下次重启模式
function DI_SetNextRebootShadowMode(hDevice: THandle;
  NextShadowMode: LCXL_SHADOW_MODE): BOOL; stdcall;

//获取自定义沙箱模式的磁盘列表
function DI_GetCustomDiskList(hDevice: THandle; CustomDiskList: PChar;
  var CustomDiskListLen: DWORD): BOOL; stdcall;

//设置自定义沙箱模式的磁盘列表
function DI_SetCustomDiskList(hDevice: THandle; CustomDiskList: PChar;
  CustomDiskListLen: DWORD): BOOL; stdcall;

//设置/获取应用程序请求
function DI_AppRequest(hDevice: THandle;
  var AppReq: TAppRequest): BOOL; stdcall;

//启动影子模式
function DI_StartShadowMode(hDevice: THandle;
  CurShadowMode: LCXL_SHADOW_MODE): BOOL; stdcall;

//设置是否保存数据
function DI_SetSaveDataWhenShutdown(hDevice: THandle;
  const SaveData: TSaveDataWhenShutdown): BOOL; stdcall;

//验证密码
function DI_VerifyPassword(hDevice: THandle;
  PasswordMD5: PChar; PasswordMD5Len: DWORD): BOOL; stdcall;

//设置密码
function DI_SetPassword(hDevice: THandle;
  PasswordMD5: PChar; PasswordMD5Len: DWORD): BOOL; stdcall;

implementation

//驱动是否正在运行
function DI_IsDriverRunning: BOOL;
var
  hDevice: THandle;
begin
  hDevice := DI_Open;
  Result := hDevice <> INVALID_HANDLE_VALUE;
  if Result then
  begin
    CloseHandle(hDevice);
  end;
end;
//设置是否处于影子模式
function DI_InShadowMode: BOOL;
var
  hDevice: THandle;
  cbout: DWORD;
begin
  hDevice := DI_Open;
  Result := hDevice <> INVALID_HANDLE_VALUE;
  if Result then
  begin
    Result := DeviceIoControl(hDevice, IOCTL_IN_SHADOW_MODE, nil, 0, nil, 0,
      cbout, nil);
    CloseHandle(hDevice);
  end;
end;

function DI_Open: THandle;
begin
  Result := CreateFile(DRIVER_SYMBOL_LINK, GENERIC_READ or GENERIC_WRITE, 0,
    nil, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
end;

function DI_Close(hDevice: THandle): BOOL;
begin
  Result := CloseHandle(hDevice);
end;

function DI_GetRandomId(hDevice: THandle; DriverId: Pointer;
  var DriverIdLen: DWORD): BOOL;
var
  cbout: DWORD;
begin
  Result := DeviceIoControl(hDevice, IOCTL_GET_RANDOM_ID, nil, 0, DriverId,
    DriverIdLen, cbout, nil);
  if Result then
  begin
    DriverIdLen := cbout;
  end;
end;

function DI_ValidateCode(hDevice: THandle; MachineCode: Pointer;
  MachineCodeLen: DWORD): BOOL;
var
  cbout: DWORD;
begin
  Result := DeviceIoControl(hDevice, IOCTL_VALIDATE_CODE, MachineCode,
    MachineCodeLen, nil, 0, cbout, nil);
end;

function DI_GetDiskList(hDevice: THandle; DiskList: PVolumeDiskInfo;
  var DiskListLen: DWORD): BOOL;
var
  cbout: DWORD;
begin
  Result := DeviceIoControl(hDevice, IOCTL_GET_DISK_LIST, nil, 0, DiskList,
    DiskListLen * sizeof(TVolumeDiskInfo), cbout, nil);
  if Result then
  begin
    cbout := cbout div sizeof(TVolumeDiskInfo);
  end;
  DiskListLen := cbout;
end;

function DI_GetDriverSetting(hDevice: THandle;
  var DriverSetting: TAppDriverSetting): BOOL;
var
  outlen: DWORD;
begin
  outlen := sizeof(DriverSetting);
  Result := DeviceIoControl(hDevice, IOCTL_GET_DRIVER_SETTING, nil, 0,
    @DriverSetting, outlen, outlen, nil);
end;

function DI_SetNextRebootShadowMode(hDevice: THandle;
  NextShadowMode: LCXL_SHADOW_MODE): BOOL;
var
  outlen: DWORD;
begin
  Result := DeviceIoControl(hDevice, IOCTL_SET_NEXT_REBOOT_SHADOW_MODE,
    @NextShadowMode, sizeof(NextShadowMode), nil, 0, outlen, nil);
end;

//获取自定义沙箱模式的磁盘列表
function DI_GetCustomDiskList(hDevice: THandle; CustomDiskList: PChar;
  var CustomDiskListLen: DWORD): BOOL;
var
  outlen: DWORD;
begin
  Result := DeviceIoControl(hDevice, IOCTL_GET_CUSTOM_DISK_LIST,
    nil, 0, CustomDiskList, CustomDiskListLen*MAX_DEVNAME_LENGTH *sizeof(Char),
    outlen, nil);
  if Result then
  begin
    CustomDiskListLen := outlen div (MAX_DEVNAME_LENGTH * sizeof(Char));
  end;
end;

//设置自定义沙箱模式的磁盘列表
function DI_SetCustomDiskList(hDevice: THandle; CustomDiskList: PChar;
  CustomDiskListLen: DWORD): BOOL;
var
  outlen: DWORD;
begin
  Result := DeviceIoControl(hDevice, IOCTL_SET_CUSTOM_DISK_LIST,
    CustomDiskList, CustomDiskListLen*MAX_DEVNAME_LENGTH *sizeof(Char),
    nil, 0, outlen, nil);
end;

//设置/获取应用程序请求
function DI_AppRequest(hDevice: THandle; var AppReq: TAppRequest): BOOL;
var
  outlen: DWORD;
begin
  Result := DeviceIoControl(hDevice, IOCTL_APP_RQUESET,
    @AppReq, sizeof(AppReq), @AppReq, sizeof(AppReq), outlen, nil);
end;

//启动影子模式
function DI_StartShadowMode(hDevice: THandle;
  CurShadowMode: LCXL_SHADOW_MODE): BOOL;
var
  outlen: DWORD;
begin
  Result := DeviceIoControl(hDevice, IOCTL_START_SHADOW_MODE,
    @CurShadowMode, sizeof(CurShadowMode), nil, 0, outlen, nil);
end;

function DI_SetSaveDataWhenShutdown(hDevice: THandle;
  const SaveData: TSaveDataWhenShutdown): BOOL;
var
  outlen: DWORD;
begin
  Result := DeviceIoControl(hDevice, IOCTL_SET_SAVE_DATA_WHEN_SHUTDOWN,
    @SaveData, sizeof(SaveData), nil, 0, outlen, nil);
end;

//验证密码
function DI_VerifyPassword(hDevice: THandle;
  PasswordMD5: PChar; PasswordMD5Len: DWORD): BOOL;
var
  outlen: DWORD;
begin
  Result := DeviceIoControl(hDevice, IOCTL_VERIFY_PASSWORD,
    PasswordMD5, PasswordMD5Len, nil, 0, outlen, nil);
end;

//设置密码
function DI_SetPassword(hDevice: THandle;
  PasswordMD5: PChar; PasswordMD5Len: DWORD): BOOL;
var
  outlen: DWORD;
begin
  Result := DeviceIoControl(hDevice, IOCTL_SET_PASSWORD,
    PasswordMD5, PasswordMD5Len, nil, 0, outlen, nil);
end;

end.


