unit untCommFunc;

interface

uses
  Windows, SysUtils, Registry, JwaWinIoctl, WinSvc;

const
  RUN_VALUE_NAME = 'LCXLShadow';
  IDE_ATA_IDENTIFY = $EC; // ATA的ID指令(IDENTIFY DEVICE)
  // #define DFP_RECEIVE_DRIVE_DATA  0x0007c088
  DFP_RECEIVE_DRIVE_DATA = ((IOCTL_DISK_BASE shl 16) or
    ((FILE_READ_ACCESS or FILE_WRITE_ACCESS) shl 14) or ($0022 shl 2) or
    METHOD_BUFFERED);

type
  _IDINFO = record
    wGenConfig: USHORT; // WORD 0: 基本信息字
    wNumCyls: USHORT; // WORD 1: 柱面数
    wReserved2: USHORT; // WORD 2: 保留
    wNumHeads: USHORT; // WORD 3: 磁头数
    wReserved4: USHORT; // WORD 4: 保留
    wReserved5: USHORT; // WORD 5: 保留
    wNumSectorsPerTrack: USHORT; // WORD 6: 每磁道扇区数
    wVendorUnique: array [0 .. 2] of USHORT; // WORD 7-9: 厂家设定值
    sSerialNumber: array [0 .. 19] of AnsiChar; // WORD 10-19:序列号
    wBufferType: USHORT; // WORD 20: 缓冲类型
    wBufferSize: USHORT; // WORD 21: 缓冲大小
    wECCSize: USHORT; // WORD 22: ECC校验大小
    sFirmwareRev: array [0 .. 7] of AnsiChar; // WORD 23-26: 固件版本
    sModelNumber: array [0 .. 39] of AnsiChar; // WORD 27-46: 内部型号
    wMoreVendorUnique: USHORT; // WORD 47: 厂家设定值
    wReserved48: USHORT; // WORD 48: 保留
    (*
      struct {
      USHORT reserved1:8;
      USHORT DMA:1;   // 1=支持DMA
      USHORT LBA:1;   // 1=支持LBA
      USHORT DisIORDY:1;  // 1=可不使用IORDY
      USHORT IORDY:1;  // 1=支持IORDY
      USHORT SoftReset:1;  // 1=需要ATA软启动
      USHORT Overlap:1;  // 1=支持重叠操作
      USHORT Queue:1;  // 1=支持命令队列
      USHORT InlDMA:1;  // 1=支持交叉存取DMA
      } wCapabilities;   // WORD 49: 一般能力
    *)
    wCapabilities: USHORT; // WORD 49: 一般能力
    wReserved1: USHORT; // WORD 50: 保留
    wPIOTiming: USHORT; // WORD 51: PIO时序
    wDMATiming: USHORT; // WORD 52: DMA时序
    (*
      struct {
      USHORT CHSNumber:1;  // 1=WORD 54-58有效
      USHORT CycleNumber:1;  // 1=WORD 64-70有效
      USHORT UnltraDMA:1;  // 1=WORD 88有效
      USHORT reserved:13;
      } wFieldValidity;   // WORD 53: 后续字段有效性标志
    *)
    wFieldValidity: USHORT; // WORD 53: 后续字段有效性标志
    wNumCurCyls: USHORT; // WORD 54: CHS可寻址的柱面数
    wNumCurHeads: USHORT; // WORD 55: CHS可寻址的磁头数
    wNumCurSectorsPerTrack: USHORT; // WORD 56: CHS可寻址每磁道扇区数
    wCurSectorsLow: USHORT; // WORD 57: CHS可寻址的扇区数低位字
    wCurSectorsHigh: USHORT; // WORD 58: CHS可寻址的扇区数高位字
    (*
      struct {
      USHORT CurNumber:8;  // 当前一次性可读写扇区数
      USHORT Multi:1;  // 1=已选择多扇区读写
      USHORT reserved1:7;
      } wMultSectorStuff;   // WORD 59: 多扇区读写设定
    *)
    wMultSectorStuff: USHORT; // WORD 59: 多扇区读写设定
    dwTotalSectors: ULONG; // WORD 60-61: LBA可寻址的扇区数
    wSingleWordDMA: USHORT; // WORD 62: 单字节DMA支持能力
    (*
      struct {
      USHORT Mode0:1;  // 1=支持模式0 (4.17Mb/s)
      USHORT Mode1:1;  // 1=支持模式1 (13.3Mb/s)
      USHORT Mode2:1;  // 1=支持模式2 (16.7Mb/s)
      USHORT Reserved1:5;
      USHORT Mode0Sel:1;  // 1=已选择模式0
      USHORT Mode1Sel:1;  // 1=已选择模式1
      USHORT Mode2Sel:1;  // 1=已选择模式2
      USHORT Reserved2:5;
      } wMultiWordDMA;   // WORD 63: 多字节DMA支持能力
    *)
    wMultiWordDMA: USHORT; // WORD 63: 多字节DMA支持能力
    (*
      struct {
      USHORT AdvPOIModes:8;  // 支持高级POI模式数
      USHORT reserved:8;
      } wPIOCapacity;   // WORD 64: 高级PIO支持能力
    *)
    wPIOCapacity: USHORT; // WORD 64: 高级PIO支持能力
    wMinMultiWordDMACycle: USHORT; // WORD 65: 多字节DMA传输周期的最小值
    wRecMultiWordDMACycle: USHORT; // WORD 66: 多字节DMA传输周期的建议值
    wMinPIONoFlowCycle: USHORT; // WORD 67: 无流控制时PIO传输周期的最小值
    wMinPOIFlowCycle: USHORT; // WORD 68: 有流控制时PIO传输周期的最小值
    wReserved69: array [0 .. 10] of USHORT; // WORD 69-79: 保留
    (*
      struct {
      USHORT Reserved1:1;
      USHORT ATA1:1;   // 1=支持ATA-1
      USHORT ATA2:1;   // 1=支持ATA-2
      USHORT ATA3:1;   // 1=支持ATA-3
      USHORT ATA4:1;   // 1=支持ATA/ATAPI-4
      USHORT ATA5:1;   // 1=支持ATA/ATAPI-5
      USHORT ATA6:1;   // 1=支持ATA/ATAPI-6
      USHORT ATA7:1;   // 1=支持ATA/ATAPI-7
      USHORT ATA8:1;   // 1=支持ATA/ATAPI-8
      USHORT ATA9:1;   // 1=支持ATA/ATAPI-9
      USHORT ATA10:1;  // 1=支持ATA/ATAPI-10
      USHORT ATA11:1;  // 1=支持ATA/ATAPI-11
      USHORT ATA12:1;  // 1=支持ATA/ATAPI-12
      USHORT ATA13:1;  // 1=支持ATA/ATAPI-13
      USHORT ATA14:1;  // 1=支持ATA/ATAPI-14
      USHORT Reserved2:1;
      } wMajorVersion;   // WORD 80: 主版本
    *)
    wMajorVersion: USHORT; // WORD 80: 主版本
    wMinorVersion: USHORT; // WORD 81: 副版本
    wReserved82: array [0 .. 5] of USHORT; // WORD 82-87: 保留
    (*
      struct {
      USHORT Mode0:1;  // 1=支持模式0 (16.7Mb/s)
      USHORT Mode1:1;  // 1=支持模式1 (25Mb/s)
      USHORT Mode2:1;  // 1=支持模式2 (33Mb/s)
      USHORT Mode3:1;  // 1=支持模式3 (44Mb/s)
      USHORT Mode4:1;  // 1=支持模式4 (66Mb/s)
      USHORT Mode5:1;  // 1=支持模式5 (100Mb/s)
      USHORT Mode6:1;  // 1=支持模式6 (133Mb/s)
      USHORT Mode7:1;  // 1=支持模式7 (166Mb/s) ???
      USHORT Mode0Sel:1;  // 1=已选择模式0
      USHORT Mode1Sel:1;  // 1=已选择模式1
      USHORT Mode2Sel:1;  // 1=已选择模式2
      USHORT Mode3Sel:1;  // 1=已选择模式3
      USHORT Mode4Sel:1;  // 1=已选择模式4
      USHORT Mode5Sel:1;  // 1=已选择模式5
      USHORT Mode6Sel:1;  // 1=已选择模式6
      USHORT Mode7Sel:1;  // 1=已选择模式7
      } wUltraDMA;   // WORD 88: Ultra DMA支持能力
    *)
    wUltraDMA: USHORT; // WORD 88: Ultra DMA支持能力
    wReserved89: array [0 .. 166] of USHORT; // WORD 89-255
  end;

  IDINFO = _IDINFO;
  PIDINFO = ^IDINFO;

function IsAdminUser: Boolean;
function EnableShutdownPrivilege: Boolean;

function IsAutoRun: Boolean; // 是否开机启动
function SetAutoRun(AutoRun: Boolean): Boolean; // 设置是否开机启动

/// <summary>
/// 获取磁盘序列号，PASCAL版
/// </summary>
function DiskIdentifyDevice(DiskNum: Integer; var ID_INFO: IDINFO): Boolean;

/// <summary>
/// 调试输出
/// </summary>
/// <param name="DebugInfo">
/// 输出字符串
/// </param>
/// <param name="AddLinkBreak">
/// 是否添加换行符
/// </param>
procedure OutputDebugStr(const DebugInfo: string;
  AddLinkBreak: Boolean = True); inline;

/// <summary>
/// 获取命令参数的位置，如果未找到，则返回0
/// </summary>
/// <param name="Switch">
/// 命令参数名
/// </param>
/// <param name="Chars">
/// 命令开关
/// </param>
/// <param name="IgnoreCase">
/// 是否忽略大小写
/// </param>
/// <returns>
/// 如果未找到，则返回0
/// </returns>
function GetCmdLineSwitchIndex(const Switch: string;
  const Chars: TSysCharSet = SwitchChars; IgnoreCase: Boolean = True): Integer;

function StartServicebyName(const ServiceName: string): Boolean;

implementation

function IsAdminUser: Boolean;
const
  SECURITY_BUILTIN_DOMAIN_RID = $20;
  DOMAIN_ALIAS_RID_ADMINS = $220;
  SECURITY_NT_AUTHORITY: TSIDIDENTIFIERAUTHORITY = (Value: (0, 0, 0, 0, 0, 5));
var
  hAccessToken: THandle;
  ptgGroups: PTokenGroups;
  dwInfoBufferSize: DWORD;
  psidAdministrators: PSID;
  x: Integer;
  bSuccess: BOOL;
begin
  Result := False;
  bSuccess := OpenThreadToken(GetCurrentThread, TOKEN_QUERY, True,
    hAccessToken);
  if not bSuccess then
  begin
    if GetLastError = ERROR_NO_TOKEN then
      bSuccess := OpenProcessToken(GetCurrentProcess, TOKEN_QUERY,
        hAccessToken);
  end;
  if bSuccess then
  begin
    GetMem(ptgGroups, 1024);
    bSuccess := GetTokenInformation(hAccessToken, TokenGroups, ptgGroups, 1024,
      dwInfoBufferSize);
    CloseHandle(hAccessToken);
    if bSuccess then
    begin
      AllocateAndInitializeSid(SECURITY_NT_AUTHORITY, 2,
        SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
        psidAdministrators);
{$R-}
      for x := 0 to ptgGroups.GroupCount - 1 do
        if EqualSid(psidAdministrators, ptgGroups.Groups[x].Sid) then
        begin
          Result := True;
          Break;
        end;
{$R+}
      FreeSid(psidAdministrators);
    end;
    FreeMem(ptgGroups);
  end;
end;

function EnableShutdownPrivilege: Boolean;
  function EnablePrivilege(hToken: Cardinal; PrivName: string;
    bEnable: Boolean): Boolean;
  var
    TP: TOKEN_PRIVILEGES;
    Dummy: Cardinal;
  begin
    Result := False;
    TP.PrivilegeCount := 1;
    if LookupPrivilegeValue(nil, PChar(PrivName), TP.Privileges[0].Luid) then
    begin
      if bEnable then
      begin
        TP.Privileges[0].Attributes := SE_PRIVILEGE_ENABLED;
      end
      else
      begin
        TP.Privileges[0].Attributes := 0;
      end;
      Result := AdjustTokenPrivileges(hToken, False, TP, SizeOf(TP),
        nil, Dummy);
    end;
  end;

var
  hToken: THandle;
begin
  Result := False;
  if OpenProcessToken(GetCurrentProcess, TOKEN_ADJUST_PRIVILEGES, hToken) then
  begin
    Result := EnablePrivilege(hToken, 'SeShutdownPrivilege', True);
    CloseHandle(hToken);
  end;
end;

function IsAutoRun: Boolean; // 是否开机启动
var
  Reg: TRegistry;
  FilePath: string;
  CurFilePath: string;
begin
  Result := False;
  Reg := TRegistry.Create;
  Reg.RootKey := HKEY_LOCAL_MACHINE;
  if Reg.OpenKey('SOFTWARE\Microsoft\Windows\CurrentVersion\Run', True) then
  begin
    try
      FilePath := Reg.ReadString(RUN_VALUE_NAME);
      CurFilePath := Format('"%s" /autorun', [ParamStr(0)]);
      if CompareText(CurFilePath, FilePath) = 0 then
      begin
        Result := True;
      end;
    except

    end;
    Reg.CloseKey;
  end;
  Reg.Free;
end;

function SetAutoRun(AutoRun: Boolean): Boolean; // 设置是否开机启动
var
  Reg: TRegistry;
  CurFilePath: string;
  LastErrorCode: DWORD;
begin
  Result := False;
  LastErrorCode := 0;
  Reg := TRegistry.Create;
  Reg.RootKey := HKEY_LOCAL_MACHINE;
  if Reg.OpenKey('SOFTWARE\Microsoft\Windows\CurrentVersion\Run', True) then
  begin
    if AutoRun then
    begin
      CurFilePath := Format('"%s" /autorun', [ParamStr(0)]);
      try
        Reg.WriteString(RUN_VALUE_NAME, CurFilePath);
        Result := True;
      except
        LastErrorCode := GetLastError;
        OutputDebugStr(Format('SetAutoRun Failed:%d', [LastErrorCode]));
      end;
    end
    else
    begin
      Reg.DeleteValue(RUN_VALUE_NAME);
      Result := True;
    end;
    Reg.CloseKey;
  end;
  Reg.Free;
  SetLastError(LastErrorCode);
end;

// 获取磁盘序列号，PASCAL版
function DiskIdentifyDevice(DiskNum: Integer; var ID_INFO: IDINFO): Boolean;
var
  pSCIP: PSendCmdInParams;
  pSCOP: PSENDCMDOUTPARAMS;
  hDevice: THandle;
  retSize: DWORD;
  LastError: DWORD;
begin
  Result := False;
  LastError := 0;
  hDevice := CreateFile(PChar(Format('\\.\PHYSICALDRIVE%d', [DiskNum])),
    GENERIC_READ or GENERIC_WRITE, FILE_SHARE_READ or FILE_SHARE_WRITE, nil,
    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if hDevice = INVALID_HANDLE_VALUE then
  begin
    LastError := GetLastError();
    OutputDebugStr(Format('DiskIdentifyDevice:CreateFile失败:%d-%s',
      [LastError, SysErrorMessage(LastError)]));
    SetLastError(LastError);
    Exit;
  end;

  GetMem(pSCIP, SizeOf(SENDCMDINPARAMS));
  GetMem(pSCOP, SizeOf(SENDCMDOUTPARAMS) + SizeOf(ID_INFO));

  ZeroMemory(pSCIP, SizeOf(SENDCMDINPARAMS));
  ZeroMemory(pSCOP, SizeOf(SENDCMDOUTPARAMS) + SizeOf(ID_INFO));

  // 指定ATA/ATAPI命令的寄存器值
  pSCIP.irDriveRegs.bCommandReg := IDE_ATA_IDENTIFY;
  // 指定输入/输出数据缓冲区大小
  pSCOP.cBufferSize := SizeOf(ID_INFO);
  if DeviceIoControl(hDevice, DFP_RECEIVE_DRIVE_DATA, pSCIP,
    SizeOf(SENDCMDINPARAMS), pSCOP, SizeOf(SENDCMDOUTPARAMS) + SizeOf(ID_INFO),
    retSize, nil) then
  begin
    CopyMemory(@ID_INFO, @pSCOP.bBuffer[0], SizeOf(ID_INFO));
    Result := True;
  end
  else
  begin
    LastError := GetLastError();
    // fix-it:目前不支持SCSI磁盘的序列号获取，所以暂时以0填充
    ZeroMemory(@ID_INFO, SizeOf(ID_INFO));
    OutputDebugStr(Format('!!!DLL:DiskIdentifyDevice:DeviceIoControl失败:%d-%s',
      [LastError, SysErrorMessage(LastError)]));
    Result := True;
  end;
  CloseHandle(hDevice);
  SetLastError(LastError);
end;

// 调试输出
procedure OutputDebugStr(const DebugInfo: string; AddLinkBreak: Boolean);
begin
{$IFDEF DEBUG}
  if AddLinkBreak then
  begin
    Windows.OutputDebugString(PChar(Format('%s'#10, [DebugInfo])));
  end
  else
  begin
    Windows.OutputDebugString(PChar(DebugInfo));
  end;
{$ENDIF}
end;

function GetCmdLineSwitchIndex(const Switch: string; const Chars: TSysCharSet;
  IgnoreCase: Boolean): Integer;
var
  I: Integer;
  S: string;
begin
  for I := 1 to ParamCount do
  begin
    S := ParamStr(I);
    if (Chars = []) or (S[1] in Chars) then
      if IgnoreCase then
      begin
        if (AnsiCompareText(Copy(S, 2, Maxint), Switch) = 0) then
        begin
          Result := I;
          Exit;
        end;
      end
      else
      begin
        if (AnsiCompareStr(Copy(S, 2, Maxint), Switch) = 0) then
        begin
          Result := I;
          Exit;
        end;
      end;
  end;
  Result := 0;
end;

function StartServicebyName(const ServiceName: string): Boolean;
var
  hSCMgr: THandle;
  hSer: THandle;
  ssStatus: SERVICE_STATUS_PROCESS;
  dwBytesNeeded: DWORD;
  SerCmd: PChar;
  LastError: DWORD;
begin
  Result := False;
  LastError := 0;
  hSCMgr := OpenSCManager(nil, nil, SC_MANAGER_ALL_ACCESS);
  if (hSCMgr = 0) then
  begin
    Exit;
  end;
  hSer := OpenService(hSCMgr, PChar(ServiceName), SERVICE_ALL_ACCESS);
  if hSer <> 0 then
  begin
    if QueryServiceStatusEx(hSer, // handle to service
      SC_STATUS_PROCESS_INFO, // information level
      LPBYTE(@ssStatus), // address of structure
      SizeOf(SERVICE_STATUS_PROCESS), // size of structure
      &dwBytesNeeded) then // size needed if buffer is too small
    begin
      if ssStatus.dwCurrentState = SERVICE_STOPPED then
      begin
        SerCmd := nil;
        WinSvc.StartService(hSer, 0, SerCmd);
        QueryServiceStatusEx(hSer, // handle to service
          SC_STATUS_PROCESS_INFO, // information level
          LPBYTE(@ssStatus), // address of structure
          SizeOf(SERVICE_STATUS_PROCESS), // size of structure
          &dwBytesNeeded);
      end;
      while ssStatus.dwCurrentState = SERVICE_START_PENDING do
      begin
        OutputDebugStr(Format('APP:StartService:等待%s服务启动', [ServiceName]));
        Sleep(1000);
        if not QueryServiceStatusEx(hSer, // handle to service
          SC_STATUS_PROCESS_INFO, // information level
          LPBYTE(@ssStatus), // address of structure
          SizeOf(SERVICE_STATUS_PROCESS), // size of structure
          &dwBytesNeeded) then
        begin
          LastError := GetLastError();
          OutputDebugStr
            (Format('APP:StartService:QueryServiceStatusEx(%s)函数失败:%d',
            [ServiceName, LastError]));
          Break;
        end;
      end;
      Result := ssStatus.dwCurrentState = SERVICE_RUNNING;
    end
    else
    begin
      LastError := GetLastError();
    end;
    CloseServiceHandle(hSer);
  end
  else
  begin
    LastError := GetLastError();
  end;
  CloseServiceHandle(hSCMgr);
  SetLastError(LastError);
end;

end.
