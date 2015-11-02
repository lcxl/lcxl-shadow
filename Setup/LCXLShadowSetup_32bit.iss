; 脚本用 Inno Setup 脚本向导 生成。
; 查阅文档获取创建 INNO SETUP 脚本文件的详细资料！

#define MyAppName "LCXLShadow影子还原系统"
#define MyAppVersion "0.3"
#define MyAppPublisher "罗澄曦"
#define MyAppURL "http://lcxl.5166.info/"
#define MyAppExeName "LCXLShadowConsole.exe"
#define MyConfigExeName "LCXLShadowConfig.exe"
; #define MySerExeName "LCXLShadowSer.exe"
[Setup]
; 注意: AppId 的值是唯一识别这个程序的标志。
; 不要在其他程序中使用相同的 AppId 值。
; (在编译器中点击菜单“工具 -> 产生 GUID”可以产生一个新的 GUID)
AppId={{66986511-18B9-49B3-9AF9-F97E8EA8274A}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={pf}\LCXLShadow
DefaultGroupName={#MyAppName}
LicenseFile=.\许可文件.rtf
InfoBeforeFile=.\LCXLShadow介绍.rtf
InfoAfterFile=.\安装后.txt
OutputDir=.\Output
OutputBaseFilename=LCXLShadowSetup_32bit
SetupIconFile=.\SetupIcon.ico
Compression=lzma
SolidCompression=true
WizardImageFile=WizardImageFile.bmp
WizardSmallImageFile=.\WizardSmallImageFile.bmp
AppCopyright=Copyright (C) 2012-2013 LCXL
;ArchitecturesAllowed=x86 x64
ArchitecturesAllowed=x86
ArchitecturesInstallIn64BitMode=x64

[Languages]
;Name: "default"; MessagesFile: "compiler:Default.isl"
Name: "chinesesimp"; MessagesFile: "compiler:Languages\ChineseSimp.isl"
Name: "english"; MessagesFile: "compiler:Languages\English.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
;32位下的安装
Source: "..\bin\i386\DriverTest.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "..\bin\i386\LCXLShadow.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "..\bin\i386\LCXLShadowConsole.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "..\bin\i386\LCXLMsg.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "..\bin\i386\LCXLShadowConfig.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "..\bin\i386\LCXLShadowSer.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "..\bin\i386\LCXLShadowDriver.sys"; DestDir: "{app}"; Flags: ignoreversion; Attribs: readonly hidden system; Check: not Is64BitInstallMode
;64位下的安装
Source: "..\bin\amd64\DriverTest.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode
Source: "..\bin\amd64\LCXLShadow.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode
Source: "..\bin\amd64\LCXLShadowConsole.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode
Source: "..\bin\amd64\LCXLMsg.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode
Source: "..\bin\amd64\LCXLShadowConfig.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode
Source: "..\bin\amd64\LCXLShadowSer.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode
Source: "..\bin\amd64\LCXLShadowDriver.sys"; DestDir: "{app}"; Flags: ignoreversion; Attribs: readonly hidden system; Check: Is64BitInstallMode
; 注意: 不要在任何共享的系统文件使用 "Flags: ignoreversion"

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
[Messages]
BeveledLabel=版权所有 (C) 2012-2013 罗澄曦
[Run]
Filename: "{app}\{#MyConfigExeName}"; Parameters: "/Install";StatusMsg: "正在初始化驱动程序..."
Filename: "{app}\{#MyAppExeName}"; Parameters: "/Install";StatusMsg: "正在初始化控制台程序..."
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, "&", "&&")}}"; Flags: nowait postinstall skipifsilent
[UninstallRun]
Filename: "{app}\{#MyAppExeName}"; Parameters: "/Uninstall";StatusMsg: "正在卸载控制台程序..."
Filename: "{app}\{#MyConfigExeName}"; Parameters: "/Uninstall";StatusMsg: "正在卸载驱动程序..."
[Code]
//设置是否处于影子模式
//function DI_InShadowMode: BOOL; external 'DI_InShadowMode@{app}\LCXLShadow.dll stdcall uninstallonly';

function CreateFile(
    lpFileName             : String;
    dwDesiredAccess        : Cardinal;
    dwShareMode            : Cardinal;
    lpSecurityAttributes   : Cardinal;
    dwCreationDisposition  : Cardinal;
    dwFlagsAndAttributes   : Cardinal;
    hTemplateFile          : Integer
): THandle; external 'CreateFileA@kernel32.dll stdcall';

function CloseHandle(hHandle: THandle): Boolean; external 'CloseHandle@kernel32.dll stdcall';

function IsAppRunning(): Boolean;
begin
  Result := False;
  if CheckForMutexes('LCXLShadow_App') then
  begin
    Result := True;
  end;
end;

const
  OPEN_EXISTING = 3;
  INVALID_HANDLE_VALUE = $FFFFFFFF;
  

function InitializeSetup(): Boolean;
var
  HasRun: Boolean;
  MykeynotExist:boolean;
  ResultCode: Integer;
  uicmd: String;
  hDevice: THandle;
begin
  Result := False;
  while not Result do
  begin
    Result := not IsAppRunning();
    if not Result then
    begin
      if MsgBox('安装程序检测到LCXLShadow控制台程序正在运行。'#13#10#13#10'您必须先关闭它然后单击“是”继续安装，或按“否”退出。', mbConfirmation, MB_YESNO) = idNO then
      begin
        break;
      end;
    end;
  end;
  if not Result then
  begin
    Exit;
  end;
  Result := False;
  if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{66986511-18B9-49B3-9AF9-F97E8EA8274A}_is1', 'UninstallString', uicmd) then
  begin
    if MsgBox('安装程序检测到您已经安装了本软件。'#13#10#13#10'是否先卸载当前软件再安装？', mbConfirmation, MB_YESNO) = idYES then
    begin
      Exec(RemoveQuotes(uicmd), '', '', SW_SHOW, ewWaitUntilTerminated, ResultCode);
      if ResultCode = 0 then
      begin
        //卸载成功后必须重启电脑
        Result := False;
      end
      else
      begin
        if MsgBox('未能成功卸载，是否强制覆盖？', mbError, MB_YESNO) = IdYES then
        begin
          Result := True;
        end;
      end;
    end;
  end
  else
  begin
    //已经卸载了，再看看驱动是否正在运行，如果正在运行，说明还没有重新启动
    hDevice := CreateFile('\\.\LCXLShadow', 0, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hDevice <>  INVALID_HANDLE_VALUE) then
    begin
      MsgBox('安装程序检测到您卸载后没有重启，请先重启电脑。', mbConfirmation, MB_OK);
      CloseHandle(hDevice);
      Result := False;
    end
    else
    begin
      Result := True;
    end;
  end;
end;

const
  MF_BYPOSITION=$400;
  function DeleteMenu(HMENU: HWND; uPosition: UINT; uFlags: UINT): BOOL; external 'DeleteMenu@user32.dll stdcall';
  function GetSystemMenu(HWND: hWnd; bRevert: BOOL): HWND; external 'GetSystemMenu@user32.dll stdcall';

procedure InitializeWizard();
begin
 //删除构建信息
  DeleteMenu(GetSystemMenu(wizardform.handle,false),8,MF_BYPOSITION);
  DeleteMenu(GetSystemMenu(wizardform.handle,false),7,MF_BYPOSITION);
end;

function InitializeUninstall(): Boolean;
var
  HasRun: Boolean;
  ResultCode: Integer;
begin
  Result := False;
  //运行配置程序，查看是否处于影子模式
  if not Exec(RemoveQuotes(ExpandConstant('{app}\{#MyConfigExeName}')), '-InShadowMode', '', SW_SHOW, ewWaitUntilTerminated, ResultCode) then
  begin
    MsgBox('运行配置程序失败，请重新安装本程序！', mbError, MB_OK);
    Exit;
  end;
  if (ResultCode<>0) then
  begin
    MsgBox('安装程序检测到您正处于影子模式，请先从影子模式退出后再运行本软件。', mbError, MB_OK);
    Exit;
  end;
  while not Result do
  begin
    Result := not IsAppRunning();
    if not Result then
    begin
      if MsgBox('安装程序检测到LCXLShadow控制台程序正在运行。'#13#10#13#10'您必须先关闭它然后单击“是”继续安装，或按“否”退出。', mbConfirmation, MB_YESNO) = idNO then
      begin
        break;
      end;
    end;
  end;
end;

function NeedRestart(): Boolean;
begin
  Result := True;
end;

function UninstallNeedRestart(): Boolean;
begin
  Result := True;
end;

