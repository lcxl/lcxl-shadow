unit untSerInterface;

interface
uses
  Windows, untCommDataType;
const
  LCXLSHADOW_SER_NAME = 'LCXLShadowSer';

const
  //NONE
  LL_NONE = $0000;
  //认证
  LL_AUTH = $0001;
  //驱动是否在运行
  LL_IS_DRIVER_RUNNING = $0002;
  //获取驱动配置
  LL_GET_DRIVER_SETTING = $0003;
  //设置下次重启的影子模式
  LL_SET_NEXT_REBOOT_SHADOW_MODE = $0004;
  //是否处于影子模式下
  LL_IN_SHADOW_MODE = $0005;
  //设置密码
  LL_SET_PASSWORD = $0006;
  //验证密码
  LL_VERIFY_PASSWORD = $0007;
  //清空密码数据
  LL_CLEAR_PASSWORD = $0008;
  //获取磁盘列表
  LL_GET_DISK_LIST = $0009;
  //获取磁盘列表
  LL_GET_CUSTOM_DISK_LIST = $000A;
  //获取磁盘列表
  LL_SET_CUSTOM_DISK_LIST = $000B;
  //程序命令
  LL_APP_REQUEST= $000C;
  //启动影子模式
  LL_START_SHADOW_MODE = $000D;
  //设置当关机时保存数据
  LL_SET_SAVE_DATA_WHEN_SHUTDOWN = $000E;

  //成功 ，成功的命令
  LL_SUCC = $FF01;
  //失败，失败的命令，失败码
  LL_FAIL = $FF02;

  //下面的命令只和服务程序有关，不和驱动程序交互
  //获取系统的磁盘信息
  LL_SYS_VOLUME_INFO = $0101;
  //设置关机模式
  LL_SYS_SHUTDOWN = $0102;
type
  //LL_SYS_VOLUME_INFO
  //系统显示的磁盘信息
  TSysVolumeInfo = record
    ///	<summary>
    ///	  卷标
    ///	</summary>
    VolumeName: array [0 .. MAX_DEVNAME_LENGTH-1] of Char;
    ///	<summary>
    ///	  剩余磁盘空间
    ///	</summary>
    AvaliableSize: ULONGLONG;
    ///	<summary>
    ///	  总共磁盘空间
    ///	</summary>
    TotalSize: TLargeInteger;
  end;
  PSysVolumeInfo = ^TSysVolumeInfo;

  //命令
  TLLSysVolumeInfo = record
    DosName: array [0..MAX_DEVNAME_LENGTH-1] of Char;
    SysVolInfo: TSysVolumeInfo;
  end;
  PLLSysVolumeInfo = ^TLLSysVolumeInfo;
  //LL_SYS_SHUTDOWN
const
  //TSYS_SHUTDOWN_EMUN
  SS_NONE = $00;
  SS_SHUTDOWN = $01;
  SS_REBOOT = $02;

implementation

end.
