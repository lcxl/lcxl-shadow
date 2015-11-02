unit untCommDataType;

interface
uses
  Windows;

const
  //定义驱动错误类型
  DRIVER_ERROR_SUCCESS = $00000000;//成功
  DRIVER_ERROR_SETTING_FILE_NOT_FOUND = $00000001;//驱动配置文件不存在
  MAX_DEVNAME_LENGTH = 64;
type
  TDeviceNameType = array [0 .. MAX_DEVNAME_LENGTH-1] of Char;
  {$POINTERMATH ON}
  PDeviceNameType = ^TDeviceNameType;
  {$POINTERMATH OFF}
type
{$Z+}// 4字节
  LCXL_SHADOW_MODE = (SM_NONE, SM_ALL, SM_SYSTEM, SM_CUSTOM);
  PLCXL_SHADOW_MODE = ^LCXL_SHADOW_MODE;
  //应用程序请求类型
  //AR_VOLUME_INFO:卷信息
  APP_REQUEST_TYPE = (AR_VOLUME_INFO);
  PAPP_REQUEST_TYPE = APP_REQUEST_TYPE;
{$Z-}
type
  _VOLUME_DISK_INFO = packed record
    ///	<summary>
    ///	  盘符名
    ///	</summary>
    DosName: array [0 .. MAX_DEVNAME_LENGTH-1] of Char;
    ///	<summary>
    ///	  此磁盘是否被保护
    ///	</summary>
    IsProtect: Boolean;
    ///	<summary>
    ///	  是否在关机的时候保存数据
    ///	</summary>
    IsSaveShadowData: Boolean;
    ///	<summary>
    ///	  磁盘卷名称
    ///	</summary>
    VolumeDiskName: array [0 .. MAX_DEVNAME_LENGTH-1] of Char;
  end;

  VOLUME_DISK_INFO = _VOLUME_DISK_INFO;
  PVOLUME_DISK_INFO = ^VOLUME_DISK_INFO;
  TVolumeDiskInfo = VOLUME_DISK_INFO;
  {$POINTERMATH ON}
  PVolumeDiskInfo = ^TVolumeDiskInfo;
  {$POINTERMATH OFF}
  // 获取驱动设置
  _APP_DRIVER_SETTING = packed record
    DriverErrorType: ULONG;//驱动在运行过程中是否有错误
    CurShadowMode: LCXL_SHADOW_MODE; // 当前磁盘保护模式
    NextShadowMode: LCXL_SHADOW_MODE; // 磁盘保护模式
  end;

  APP_DRIVER_SETTING = _APP_DRIVER_SETTING;
  PAPP_DRIVER_SETTING = ^APP_DRIVER_SETTING;
  TAppDriverSetting = APP_DRIVER_SETTING;
  PAppDriverSetting = ^TAppDriverSetting;

  _APP_REQUEST_VOLUME_INFO = packed record
	  BytesPerSector: ULONG;//每个扇区包含的字节数
	  SectorsPerAllocationUnit: ULONG;//分配单元中包含的扇区数
  	BytesPerAllocationUnit: ULONG;//分配单元中包含的字节数，为BytesPerSector*SectorsPerAllocationUnit
	  TotalAllocationUnits: ULONGLONG;//总共有多少分配单元
	  AvailableAllocationUnits: ULONGLONG;//还有多少可用的单元，
  end;
  APP_REQUEST_VOLUME_INFO = _APP_REQUEST_VOLUME_INFO;
  PAPP_REQUEST_VOLUME_INFO = ^APP_REQUEST_VOLUME_INFO;

  //应用程序请求
  _APP_REQUEST = packed record
	  //IN 盘符名
	  VolumeName: array [0..MAX_DEVNAME_LENGTH-1] of Char;
	  //IN 请求类型
	  RequestType: APP_REQUEST_TYPE ;
	  //请求具体内存
    case Integer of
    0:
    (
      VolumeInfo: APP_REQUEST_VOLUME_INFO ;//AR_VOLUME_INFO
    );
 end;
 APP_REQUEST = _APP_REQUEST;
 PAPP_REQUEST = ^APP_REQUEST;
 TAppRequest = APP_REQUEST;
 PAppRequest = ^TAppRequest;

 //是否在关机的时候保存影子数据
 _SAVE_DATA_WHEN_SHUTDOWN = packed record
   //盘符名
	VolumeName: array [0..MAX_DEVNAME_LENGTH-1] of Char;
	//是否保存数据
	IsSaveData: Boolean;
 end;
 SAVE_DATA_WHEN_SHUTDOWN = _SAVE_DATA_WHEN_SHUTDOWN;
 PSAVE_DATA_WHEN_SHUTDOWN = ^SAVE_DATA_WHEN_SHUTDOWN;
 TSaveDataWhenShutdown = SAVE_DATA_WHEN_SHUTDOWN;
 PSaveDataWhenShutdown = ^TSaveDataWhenShutdown;



implementation

end.
