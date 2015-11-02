unit untSerCore;

interface

uses
  Windows, SysUtils, WinSvc,

  untSerInterface, untDllInterface, untCommFunc,
  untDllInterfaceEx, untCommDataType, untSockMgr, LCXLIOCPBase, LCXLIOCPCmd;

type
  /// <summary>
  /// 服务管理父类
  /// </summary>
  TSerMgrBase = class(TObject)
  private
    FServiceTableEntry: array [0 .. 1] of TServiceTableEntry;
    FServiceStatusHandle: SERVICE_STATUS_HANDLE; // 服务句柄
  protected
    FSerStatus: TServiceStatus;
     /// <summary>
    /// 报告服务状态给服务管理器
    /// </summary>
    /// <param name="SerStatus">
    /// 服务状态
    /// </param>
    /// <returns>
    /// 是否成功
    /// </returns>
    function ReportStatusToSCMgr(): Boolean;
    procedure SerMain(dwNumServicesArgs: DWORD; lpServiceArgVectors: PPChar);
    procedure SerHandler(dwControl: DWORD); virtual; abstract;
    // 运行服务
    procedure SerRun(); virtual; abstract;
  public
    constructor Create(); reintroduce; virtual;
    destructor Destroy(); override;



    /// <summary>
    /// 初始化服务
    /// </summary>
    /// <returns>
    /// 是否成功
    /// </returns>
    function Run(): Boolean;

  end;

  /// <summary>
  /// 服务类
  /// </summary>
  TSerCore = class(TSerMgrBase)
  private
    // 退出事件
    FExitEvent: THandle;
    FSerList: TIOCPSerList;
    FIOCPMgr: TIOCPManager;
    FSockLst: TSerSockLst;

    procedure IOCPEvent(EventType: TIocpEventEnum; SockObj: TCmdSockObj;
      Overlapped: PIOCPOverlapped);
    procedure RecvEvent(SockObj: TSerSockObj; Overlapped: PIOCPOverlapped);
    procedure SendEvent(SockObj: TSerSockObj; Overlapped: PIOCPOverlapped);
    // 验证密码
    function VerifyPassword(SockObj: TSerSockObj; hDevice: THandle): Boolean;
  protected
    procedure SerHandler(dwControl: DWORD); override;
    // 运行服务
    procedure SerRun(); override;
  public
  end;

var
  SerCore: TSerCore;

implementation

procedure ServiceMain(dwNumServicesArgs: DWORD; lpServiceArgVectors: PPChar); stdcall;
begin
  SerCore.SerMain(dwNumServicesArgs, lpServiceArgVectors);
end;

procedure ServiceHandler(dwControl: DWORD); stdcall;
begin
  SerCore.SerHandler(dwControl);
end;
{ TSerCore }

constructor TSerMgrBase.Create;
begin
  inherited;
  FServiceTableEntry[0].lpServiceName := LCXLSHADOW_SER_NAME;
  FServiceTableEntry[0].lpServiceProc := @ServiceMain;
  FServiceTableEntry[1].lpServiceName := nil;
  FServiceTableEntry[1].lpServiceProc := nil;
end;

destructor TSerMgrBase.Destroy;
begin

  inherited;
end;

function TSerMgrBase.Run: Boolean;
begin
{$IFDEF LCXL_SHADOW_SER_TEST}
  ServiceMain(0, nil);
  Result := True;
{$ELSE}
  Result := StartServiceCtrlDispatcher(FServiceTableEntry[0]);
{$ENDIF}
end;

function TSerMgrBase.ReportStatusToSCMgr(): Boolean;
begin
  with FSerStatus do
  begin
    if (dwCurrentState = SERVICE_START_PENDING) then
    begin
      dwControlsAccepted := 0;
    end
    else
    begin
      dwControlsAccepted := SERVICE_ACCEPT_STOP;
    end;
    if (dwCurrentState = SERVICE_RUNNING) or (dwCurrentState = SERVICE_STOPPED) then
      dwCheckPoint := 0
    else
      Inc(dwCheckPoint);
  end;
  Result := SetServiceStatus(FServiceStatusHandle, FSerStatus);
end;

procedure TSerMgrBase.SerMain(dwNumServicesArgs: DWORD; lpServiceArgVectors: PPChar);
begin
{$IFDEF LCXL_SHADOW_SER_TEST}
  SerRun();
{$ELSE}
  // 注册控制
  FServiceStatusHandle := RegisterServiceCtrlHandler(LCXLSHADOW_SER_NAME,
    @ServiceHandler);
  FSerStatus.dwServiceType := SERVICE_WIN32_OWN_PROCESS;
  FSerStatus.dwServiceSpecificExitCode := 0;
  FSerStatus.dwCheckPoint := 1;
  FSerStatus.dwWaitHint := 0;
  FSerStatus.dwWin32ExitCode := 0;
  // 报告正在启动
  FSerStatus.dwCurrentState := SERVICE_START_PENDING;
  ReportStatusToSCMgr();
  // 报告启动成功
  FSerStatus.dwCurrentState := SERVICE_RUNNING;
  ReportStatusToSCMgr();

  SerRun();
  // 报告服务当前的状态给服务控制管理器
  FSerStatus.dwCurrentState := SERVICE_STOP_PENDING;
  ReportStatusToSCMgr();
  FSerStatus.dwCurrentState := SERVICE_STOPPED;
  ReportStatusToSCMgr();
{$ENDIF}
end;

{ TSerCore }

procedure TSerCore.IOCPEvent(EventType: TIocpEventEnum; SockObj: TCmdSockObj;
  Overlapped: PIOCPOverlapped);
var
  _SockObj: TSerSockObj absolute SockObj;
begin
  case EventType of
    ieAddSocket:
      ;
    ieDelSocket:
      ;
    ieError:
      ;
    ieRecvPart:
      ;
    ieRecvAll:
      begin
        RecvEvent(_SockObj, Overlapped);
      end;
    ieRecvFailed:
      ;
    ieSendPart:
      ;
    ieSendAll:
      begin
        SendEvent(_SockObj, Overlapped);
      end;
    ieSendFailed:
      ;
  end;
end;

procedure TSerCore.RecvEvent(SockObj: TSerSockObj; Overlapped: PIOCPOverlapped);
var
  hDevice: THandle;
  RecvData: TCMDDataRec;
  //RecvCMD: Word;

  DriverSetting: TAppDriverSetting;
  NextShadowMode: LCXL_SHADOW_MODE; // 磁盘保护模式
  IsShadowMode: Boolean;
  Password: string;

  DiskListLen: DWORD;
  DriverDiskList: array of TVolumeDiskInfo;

  CustomDiskList: array of TDeviceNameType;
  CustomDiskListLen: DWORD;

  AppReq: PAppRequest;

  SaveData: TSaveDataWhenShutdown;
  //LL_SYS_VOLUME_INFO
  DosName: string;
  SysVolInfo: TLLSysVolumeInfo;
  VolumeSerialNumber: DWORD;
  MaximumComponentLength: DWORD;
  FileSystemFlags: DWORD;
  TmpInt641: Int64;

  //LL_SYS_SHUTDOWN
  IsSucc: Boolean;
  //校验信息
  AuthData: DWORD;
begin
  RecvData.Assgin(SockObj.RecvData, SockObj.RecvDataLen);
  if not SockObj.IsAuthed then
  begin
    if RecvData.CMD <> LL_AUTH then
    begin
      // 认证失败
      SockObj.SendFail(LL_FAIL, LL_AUTH, 0);
    end
    else
    begin
      //if SockObj.GetRemoteIP = '127.0.0.1' then
      //begin
        // 本机连接，验证认证信息是否匹配
        AuthData := MakeLong(SockObj.GetLocalPort(), SockObj.GetRemotePort());
        OutputDebugStr(Format('SER:LL_AUTH:AuthData=%08x', [AuthData]));
        if (RecvData.DataLen = Sizeof(AuthData)) and (CompareMem(RecvData.Data, @AuthData, SizeOf(AuthData))) then
        begin
          if DI_IsDriverRunning then
          begin
            SockObj.IsAuthed := True;
            // 认证成功
            OutputDebugStr('认证成功！');
            SockObj.SendSucc(LL_SUCC, RecvData.CMD);
          end
          else
          begin
            OutputDebugStr('驱动未运行！');
            SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError);
          end;
        end
        else
        begin
          // 认证失败
          OutputDebugStr('认证失败！');
          SockObj.SendFail(LL_FAIL, RecvData.CMD, 0);
        end;
      //end
      //else
      //begin
        // 远程连接，验证用户名和密码
        // 目前不支持
      //  SockObj.SendFail(LL_FAIL, RecvData.CMD, 0);
      //end;
    end;
    Exit;
  end;
  case RecvData.CMD of
    LL_IS_DRIVER_RUNNING: // 获取驱动是否启动
      begin
        if DI_IsDriverRunning then
        begin
          OutputDebugStr('驱动已启动！');
          SockObj.SendSucc(LL_SUCC, RecvData.CMD);
        end
        else
        begin
          OutputDebugStr('驱动未启动！');
          SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError);
        end;
      end;
    LL_GET_DRIVER_SETTING: // 获取驱动设置
      begin
        hDevice := DI_OpenEx;
        if hDevice = INVALID_HANDLE_VALUE then
        begin
          OutputDebugStr('LL_GET_DRIVER_SETTING 打开设备失败！');
          SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError);
          Exit;
        end;

        if DI_GetDriverSetting(hDevice, DriverSetting) then
        begin
          OutputDebugStr('获取驱动设置成功！');
          SockObj.SendData(RecvData.CMD, @DriverSetting, SizeOf(DriverSetting));
        end
        else
        begin
          OutputDebugStr('获取驱动设置失败！');
          SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError);
        end;
        DI_Close(hDevice);
      end;
    LL_SET_NEXT_REBOOT_SHADOW_MODE: // 设置下次重启的影子模式
      begin
        hDevice := DI_OpenEx;
        if hDevice = INVALID_HANDLE_VALUE then
        begin
          SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError);
          Exit;
        end;
        if RecvData.DataLen = SizeOf(NextShadowMode) then
        begin
          if VerifyPassword(SockObj, hDevice) then
          begin
            NextShadowMode := PLCXL_SHADOW_MODE(RecvData.Data)^;
            if DI_SetNextRebootShadowMode(hDevice, NextShadowMode) then
            begin
              OutputDebugStr('设置下次重启的影子模式成功！');
              SockObj.SendSucc(LL_SUCC, RecvData.CMD, RecvData.Data, RecvData.DataLen);
            end
            else
            begin
              OutputDebugStr('设置下次重启的影子模式失败！');
              SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError);
            end;
          end
          else
          begin
            SockObj.SendFail(LL_FAIL, RecvData.CMD, 1);
          end;
        end
        else
        begin
          OutputDebugStr('LL_SET_NEXT_REBOOT_SHADOW_MODE 长度不符合');
              SockObj.SendFail(LL_FAIL, RecvData.CMD, 2);
        end;
        DI_Close(hDevice);
      end;

    LL_IN_SHADOW_MODE: // 是否处于影子模式下
      begin
        IsShadowMode := DI_InShadowMode;
        OutputDebugStr(Format('处于影子模式:%d', [Integer(IsShadowMode)]));
        SockObj.SendData(RecvData.CMD, @IsShadowMode, SizeOf(IsShadowMode));
      end;

    LL_SET_PASSWORD: // 设置密码
      begin
        SetString(Password, PChar(RecvData.Data),
          RecvData.DataLen div SizeOf(Char));

        hDevice := DI_OpenEx;
        if hDevice = INVALID_HANDLE_VALUE then
        begin
          SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError);
          Exit;
        end;
        // 验证密码
        if VerifyPassword(SockObj, hDevice) then
        begin
          if DI_SetPassword(hDevice, PChar(Password), Length(Password) * SizeOf(Char))
          then
          begin
            SockObj.PassMD5 := Password;
            SockObj.SendSucc(LL_SUCC, RecvData.CMD);
          end
          else
          begin
            OutputDebugStr('设置密码失败！');
            SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError);
          end;
        end;

        DI_Close(hDevice);
      end;

    LL_VERIFY_PASSWORD: // 验证密码
      begin
        SetString(Password, PChar(RecvData.Data),
          RecvData.DataLen div SizeOf(Char));
        hDevice := DI_OpenEx;
        if hDevice = INVALID_HANDLE_VALUE then
        begin
          SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError);
          Exit;
        end;
        if DI_VerifyPassword(hDevice, PChar(SockObj.PassMD5),
          SizeOf(Char) * Length(SockObj.PassMD5)) then
        begin
          SockObj.PassMD5 := Password;
          SockObj.SendSucc(LL_SUCC, RecvData.CMD);
        end
        else
        begin
          OutputDebugStr('验证密码失败，密码不正确');
          SockObj.SendFail(LL_FAIL, RecvData.CMD, 0);
        end;
        DI_Close(hDevice);
      end;
    LL_CLEAR_PASSWORD:
      begin
        OutputDebugStr('清除密码');
        SockObj.PassMD5 := '';
        SockObj.SendSucc(LL_SUCC, RecvData.CMD);
      end;
    LL_GET_DISK_LIST: // 获取磁盘列表
      begin
        // 设置长度
        DiskListLen := 64;
        SetLength(DriverDiskList, DiskListLen);

        hDevice := DI_OpenEx;
        if hDevice = INVALID_HANDLE_VALUE then
        begin
          SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError);
          Exit;
        end;
        if DI_GetDiskList(hDevice, @DriverDiskList[0], DiskListLen) then
        begin
          SockObj.SendData(RecvData.CMD, @DriverDiskList[0],
            DiskListLen * SizeOf(TVolumeDiskInfo));
        end
        else
        begin
          OutputDebugStr('获取磁盘列表失败！');
          SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError);
        end;
        DI_Close(hDevice);
      end;

    LL_GET_CUSTOM_DISK_LIST: // 获取自定义磁盘列表
      begin
        hDevice := DI_OpenEx;
        if hDevice = INVALID_HANDLE_VALUE then
        begin
          SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError);
          Exit;
        end;
        CustomDiskListLen := 64;
        SetLength(CustomDiskList, CustomDiskListLen);
        if not DI_GetCustomDiskList(hDevice, PChar(@CustomDiskList[0]), CustomDiskListLen)
        then
        begin
          OutputDebugStr('获取自定义磁盘列表失败！');
          SockObj.SendData(RecvData.CMD, @CustomDiskList[0],
            CustomDiskListLen * SizeOf(TDeviceNameType));
        end
        else
        begin
          SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError);
        end;
        DI_Close(hDevice);
      end;

    LL_SET_CUSTOM_DISK_LIST: // 设置自定义磁盘列表
      begin
        hDevice := DI_OpenEx;
        if hDevice = INVALID_HANDLE_VALUE then
        begin
          SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError);
          Exit;
        end;
        // 验证密码
        if VerifyPassword(SockObj, hDevice) then
        begin
          if DI_SetCustomDiskList(hDevice, PChar(RecvData.Data),
            RecvData.DataLen div (SizeOf(Char) * MAX_DEVNAME_LENGTH)) then
          begin
            OutputDebugStr('设置自定义磁盘列表成功！');
            SockObj.SendSucc(LL_SUCC, RecvData.CMD);
          end
          else
          begin
            OutputDebugStr('设置自定义磁盘列表失败！');
            SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError());
          end;
        end;

        DI_Close(hDevice);
      end;

    LL_APP_REQUEST: // 程序命令
      begin
        if SizeOf(TAppRequest) <> RecvData.DataLen then
        begin
          SockObj.SendFail(LL_FAIL, RecvData.CMD, 24);
          Exit;
        end;
        AppReq := PAppRequest(RecvData.Data);

        hDevice := DI_OpenEx;
        if hDevice = INVALID_HANDLE_VALUE then
        begin

          SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError());
          Exit;
        end;

        if DI_AppRequest(hDevice, AppReq^) then
        begin
          OutputDebugStr('SER:LL_APP_REQUEST成功！');
          SockObj.SendData(RecvData.CMD, AppReq, SizeOf(TAppRequest));
        end
        else
        begin
          OutputDebugStr('SER:LL_APP_REQUEST失败！');
          SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError());
        end;

        DI_Close(hDevice);
      end;

    LL_START_SHADOW_MODE: // 启动影子模式
      begin
        hDevice := DI_OpenEx;
        if hDevice = INVALID_HANDLE_VALUE then
        begin
          SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError());
          Exit;
        end;

        if VerifyPassword(SockObj, hDevice) then
        begin
          if RecvData.DataLen = SizeOf(NextShadowMode) then
          begin
            NextShadowMode := PLCXL_SHADOW_MODE(RecvData.Data)^;
            if DI_StartShadowMode(hDevice, NextShadowMode) then
            begin
              OutputDebugStr('启动影子模式成功！');
              SockObj.SendSucc(LL_SUCC, RecvData.CMD);
            end
            else
            begin

              SockObj.SendFail(LL_FAIL, RecvData.CMD, 24);
              OutputDebugStr('启动影子模式失败！');
            end;
          end;
        end;
        DI_Close(hDevice);
      end;

    LL_SET_SAVE_DATA_WHEN_SHUTDOWN: // 设置当关机时保存数据
      begin
        hDevice := DI_OpenEx;
        if hDevice = INVALID_HANDLE_VALUE then
        begin
          SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError());
          Exit;
        end;
        if RecvData.DataLen = SizeOf(SaveData) then
        begin
          SaveData := PSaveDataWhenShutdown(RecvData.Data)^;
          if VerifyPassword(SockObj, hDevice) then
          begin
            if DI_SetSaveDataWhenShutdown(hDevice, SaveData) then
            begin
              SockObj.SendSucc(LL_SUCC, RecvData.CMD);
            end
            else
            begin
              SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError());
            end;
          end;
        end
        else
        begin
          SockObj.SendFail(LL_FAIL, RecvData.CMD, 24);
        end;
        DI_Close(hDevice);
      end;

    LL_SYS_VOLUME_INFO://获取系统的磁盘信息
      begin
        ZeroMemory(@SysVolInfo, SizeOf(SysVolInfo));

        SetString(DosName, PChar(RecvData.Data), RecvData.DataLen div SizeOf(Char));

        StrPLCopy(SysVolInfo.DosName, DosName, SizeOf(SysVolInfo.DosName) div SizeOf(Char));
        //获取卷宗信息
        GetVolumeInformation(PChar(DosName + '\'),
          SysVolInfo.SysVolInfo.VolumeName, SizeOf(SysVolInfo.SysVolInfo.VolumeName) div SizeOf(Char),
          @VolumeSerialNumber, MaximumComponentLength, FileSystemFlags, nil, 0);
        // 获取磁盘空间
        GetDiskFreeSpaceEx(PChar(DosName + '\'), TmpInt641, SysVolInfo.SysVolInfo.TotalSize,
          PLargeInteger(@SysVolInfo.SysVolInfo.AvaliableSize));
        SockObj.SendData(RecvData.CMD, @SysVolInfo, SizeOf(SysVolInfo));
      end;
    LL_SYS_SHUTDOWN:
      begin
        if RecvData.DataLen = SizeOf(Byte) then
        begin
          if EnableShutdownPrivilege then
          begin
            case PByte(RecvData.Data)^ of
              SS_SHUTDOWN:
              begin
                IsSucc := ExitWindowsEx(EWX_SHUTDOWN, 0);
              end;
              SS_REBOOT:
              begin
                IsSucc := ExitWindowsEx(EWX_REBOOT, 0);
              end;
            else
              //87:参数错误
              SetLastError(87);
              IsSucc := False;
            end;
            if not IsSucc then
            begin
              SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError());
            end;
          end;
        end
        else
        begin
           SockObj.SendFail(LL_FAIL, RecvData.CMD, 24);
        end;
      end
  else

  end;
end;

procedure TSerCore.SendEvent(SockObj: TSerSockObj; Overlapped: PIOCPOverlapped);
begin

end;

procedure TSerCore.SerHandler(dwControl: DWORD);
begin
  // Handle the requested control code.
  case dwControl of
    SERVICE_CONTROL_STOP, SERVICE_CONTROL_SHUTDOWN:
      begin
        // 关闭服务
        OutputDebugStr('服务端接收到关闭命令');
        SetEvent(FExitEvent);

      end;
    SERVICE_CONTROL_INTERROGATE:
      ;
    SERVICE_CONTROL_PAUSE:
      ;
    SERVICE_CONTROL_CONTINUE:
      ;
    // invalid control code
  else
    // update the service status.
    ReportStatusToSCMgr();
  end;

end;

procedure TSerCore.SerRun;
begin
  // 创建IOCP对象类
  FIOCPMgr := TIOCPManager.Create();
  FSerList := TIOCPSerList.Create(FIOCPMgr);
  FSerList.IOCPEvent := IOCPEvent;
  FExitEvent := CreateEvent(nil, True, False, nil);

  FSockLst := TSerSockLst.Create;
  // 启动监听
  if FSockLst.StartListen(FSerList, 9999) then
  begin
    // 等待退出
    WaitForSingleObject(FExitEvent, INFINITE);
    FSockLst.Close;
  end
  else
  begin
    OutputDebugStr('启动监听失败！');
    FSockLst.Free;
  end;

  CloseHandle(FExitEvent);
  FSerList.Free;
  FIOCPMgr.Free;
end;

function TSerCore.VerifyPassword(SockObj: TSerSockObj; hDevice: THandle): Boolean;
begin
  Result := DI_VerifyPassword(hDevice, PChar(SockObj.PassMD5),
    SizeOf(Char) * Length(SockObj.PassMD5));
  if not Result then
  begin
    OutputDebugStr('密码验证失败');
    SockObj.SendFail(LL_FAIL, LL_VERIFY_PASSWORD, 0);
  end;
end;

end.
