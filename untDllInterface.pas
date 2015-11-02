unit untDllInterface;

interface
uses
  Windows, untCommDataType;

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

const
  LCXL_SHADOW_DLL = 'LCXLShadow.dll';

function DI_IsDriverRunning; external LCXL_SHADOW_DLL name 'DI_IsDriverRunning';
function DI_InShadowMode; external LCXL_SHADOW_DLL name 'DI_InShadowMode';
function DI_Open; external LCXL_SHADOW_DLL name 'DI_Open';
function DI_Close; external LCXL_SHADOW_DLL name 'DI_Close';
function DI_GetRandomId; external LCXL_SHADOW_DLL name 'DI_GetRandomId';
function DI_ValidateCode; external LCXL_SHADOW_DLL name 'DI_ValidateCode';
function DI_GetDiskList; external LCXL_SHADOW_DLL name 'DI_GetDiskList';
function DI_GetDriverSetting; external LCXL_SHADOW_DLL name 'DI_GetDriverSetting';
function DI_SetNextRebootShadowMode; external LCXL_SHADOW_DLL name 'DI_SetNextRebootShadowMode';
function DI_GetCustomDiskList; external LCXL_SHADOW_DLL name 'DI_GetCustomDiskList';
function DI_SetCustomDiskList; external LCXL_SHADOW_DLL name 'DI_SetCustomDiskList';
function DI_AppRequest; external LCXL_SHADOW_DLL name 'DI_AppRequest';
function DI_StartShadowMode; external LCXL_SHADOW_DLL name 'DI_StartShadowMode';
function DI_SetSaveDataWhenShutdown; external LCXL_SHADOW_DLL name 'DI_SetSaveDataWhenShutdown';
function DI_VerifyPassword; external LCXL_SHADOW_DLL name 'DI_VerifyPassword';
function DI_SetPassword; external LCXL_SHADOW_DLL name 'DI_SetPassword';

end.
