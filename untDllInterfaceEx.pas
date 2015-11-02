unit untDllInterfaceEx;

interface
uses
  Windows, SysUtils, untDllInterface, untCommFunc, Classes, ElAES, untCommDataType;



//打开驱动，此函数包含了驱动验证等一系列步骤
function DI_OpenEx: THandle;

//获取驱动器名称列表
function DI_GetDiskDosNameList(hDevice: THandle): string;

var
  ID_INFO: IDINFO;

implementation

function DI_OpenEx: THandle;
var
  Sour, Dest: TMemoryStream;
  AESKey: TAESKey128;
  AESKeyLen: DWORD;
  IsSuc: Boolean;
begin
  Result := DI_Open;
  if Result = INVALID_HANDLE_VALUE then
  begin
    Exit;
  end;
  IsSuc := True;
  AESKeyLen := SizeOf(AESKey);
  if DI_GetRandomId(Result, @AESKey, AESKeyLen) then
  begin
    Sour := TMemoryStream.Create;
    Dest := TMemoryStream.Create;
    //写入磁盘序列号
    Sour.Write(ID_INFO, SizeOf(ID_INFO));
    Sour.Position := 0;
    try
      //开始AES加密
      EncryptAESStreamECB(Sour, SizeOf(ID_INFO), AESKey, Dest);
    except
      IsSuc := False;
    end;
    if IsSuc then
    begin
      //验证硬件KEY
      IsSuc := DI_ValidateCode(Result, Dest.Memory, Dest.Position);
    end;
    Sour.Free;
    Dest.Free;
  end;
  //失败了？
  if not IsSuc then
  begin
    DI_Close(Result);
    Result := INVALID_HANDLE_VALUE;
  end;
end;

//获取驱动器名称列表
function DI_GetDiskDosNameList(hDevice: THandle): string;
var
  DiskListLen: DWORD;
  DriverDiskList: array of TVolumeDiskInfo;
  I: Integer;
begin
  DiskListLen := 100;
  SetLength(DriverDiskList, DiskListLen);
  if DI_GetDiskList(hDevice, @DriverDiskList[0], DiskListLen) then
  begin
    for I := 0 to DiskListLen-1 do
    begin
      Result := Result+DriverDiskList[i].DosName;
    end;
  end;
end;

initialization

//获取磁盘序列号
DiskIdentifyDevice(0, ID_INFO);

end.
