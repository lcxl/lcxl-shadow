unit untResString;

interface

resourcestring
  RS_MSG_INFORMATION_TITLE = '信息';
  RS_MSG_WARNING_TITLE = '警告';
  RS_MSG_STOP_TITLE = '错误';
  RS_YES = '是';
  RS_NO = '否';
  RS_VERSION_TYPE = 'Alpha个人版';
//LCXLShadowConsole.dpr
  RS_CONSOLE_IS_RUNNING = 'LCXLShadow控制台已经在前台运行。';
  RS_SERVICE_IS_CLOSED = 'LCXLShadow服务已关闭，本控制台即将退出。';
//untMain.pas
  RS_NOT_ADMIN_TEXT = '十分抱歉，您不是管理员用户，请以管理员身份运行本软件。';
  RS_NOT_ADMIN_TITLE = '权限不足';
  RS_APPLICATION_TITLE = 'LCXLShadow 影子系统控制端';
  RS_SHADOW_MODE = '影子模式';
  RS_NORMAL_MODE = '未保护';
  RS_SHADOW_SAVE_DATA = '关机保存';
  RS_FIRST_RUNNING_BALLOON_HINT = '控制台程序仍在运行，点击此处显示本程序，或者点击右键选择“退出”按钮退出。';
  RS_CANT_GET_DISK_LIST = '十分抱歉，程序无法获取磁盘列表。'#10'错误信息：%s';
  RS_CANT_GET_DRIVER_SETTING = '十分抱歉，程序无法获取启动信息。'#10'错误信息：%s';
  RS_CANT_OPEN_DEVICE = '十分抱歉，无法打开驱动设备。'#10'错误信息：%s';
  RS_CANT_SET_NEXT_REBOOT_SHADOW_MODE = '十分抱歉，无法设置下次启动的影子模式。'#10'错误信息：%s';
  RS_CANT_START_SHADOW_MODE = '十分抱歉，无法启动影子模式。'#10'错误信息：%s';
  RS_CANT_SET_AUTORUN = '十分抱歉，无法设置开机启动'#10'错误信息：%s';
  RS_CANT_CONNECT_SERVICE = '十分抱歉，无法连接到影子系统服务程序！'#10'错误信息：%s';
  RS_CANT_START_SERVICE = '十分抱歉，无法启动影子系统服务程序！'#10'错误信息：%s';
  RS_CANT_AUTH = '十分抱歉，影子系统服务程序无法验证此软件的安全性，请重新安装此软件。';
  RS_START_SHADOW_MODE_WARNNING = '请注意，在开启影子模式之前，您需要先保存您的工作，并关闭其他无关程序。'#13#10#13#10'点击“确定”按钮继续，点击“取消”按钮取消本操作。';
  RS_START_SHADOW_MODE_SUCESSFUL = '开启影子模式成功！';
  RS_START_SHADOW_MODE_NOCHANGE = '您已经处于此保护模式下了。';
  RS_STOP_SHADOW_RESTART_WARNNING = '要退出影子模式请重启您的电脑，现在要重启吗？';
  RS_SAVE_DATA_WARNING = '请注意，开启后会保存此磁盘影子模式下所有修改的数据，请谨慎操作。'#13#10'是否继续？';
  RS_DRIVER_NO_RUNNING = '十分抱歉，影子系统未正常启动，如果安装本软件后没有重启，请重启您的电脑。';
  RS_MAIN_LIST_SUB0 = '%s(%s)';
  RS_DEFAULT_VOLUME_NAME = '本地磁盘';
  RS_CLICK_TO_CHANGE = '点击更改';
  RS_SHADOW_MODE_NONE = '未保护';
  RS_SHADOW_MODE_SYSTEM = '系统盘保护';
  RS_SHADOW_MODE_ALL = '全盘保护';
  RS_SHADOW_MODE_CUSTOM = '自定义保护';
  RS_START_SHADOW = '开始保护(&S)';
  RS_STOP_SHADOW = '停止保护(&S)';
  RS_PASSWORD_CLEARED = '管理权限已释放，您需要重新输入密码才能执行对影子系统的管理操作';

//untSetShadowMode
  RS_DOS_NAME = '%s盘';
  RS_CANT_GET_CUSTOM_DISK_LIST = '十分抱歉，无法获取自定义保护磁盘列表。'#10'错误信息：%s';
  RS_CANT_SET_CUSTOM_DISK_LIST = '十分抱歉，无法设置自定义保护磁盘列表。'#10'错误信息：%s';
 //untSystemShutdown.pas
  RS_NEXT_REBOOT_SHADOW_MODE = '下次启动模式';
  //untSetPassword
  RS_PASSWORD_NOT_MATCH = '您输入的密码不一致，请重新输入。';
  RS_NO_PASSWORD_WARNING = '您输入了空密码，系统将不受密码保护，是否继续？';
  RS_SET_PASSWORD_FAILURE = '十分抱歉，设置密码失败！'#10'错误信息：%s';
  //untVerifyPassword
  RS_PASSWORD_VERIFY_FAILURE = '密码验证失败！';
implementation

end.
