unit untFunc;

interface
uses
  Windows, WinSvc;

function SetAsDiskFilter(SerName: string): Boolean;
function SetAsDiskVolFilter(SerName: string): Boolean;
function SetAsDriverFilter(UUID: string; SerName: string): Boolean;

implementation

function SetAsDiskFilter(SerName: string): Boolean;
begin
  result := SetAsDriverFilter('{4D36E967-E325-11CE-BFC1-08002BE10318}', SerName);
end;

function SetAsDiskVolFilter(SerName: string): Boolean;
begin
  result := SetAsDriverFilter('{71A27CDD-812A-11D0-BEC7-08002BE2092F}', SerName);
end;

function SetAsDriverFilter(UUID: string; SerName: string): Boolean;
var
  hReg: HKEY;
  keyresu: Integer;
  keytype: DWORD;
  keybuf: array [0..2048] of Char;
  bufpos: PChar;
  isserisexist: Boolean;
  keybufsize: DWORD;
begin
  Result := True;
  keyresu := RegOpenKeyEx(HKEY_LOCAL_MACHINE,
    PChar('SYSTEM\CurrentControlSet\Control\Class\'+UUID), //{4D36E967-E325-11CE-BFC1-08002BE10318}
    0,
    KEY_READ or KEY_WRITE, hReg);
  if keyresu = ERROR_SUCCESS then
  begin
    keybufsize := SizeOf(keybuf);
    keytype := REG_MULTI_SZ;
    keyresu := RegQueryValueEx(hReg, 'UpperFilters', nil, @keytype, Pbyte(@(keybuf[0])), @keybufsize);
    if keyresu <> ERROR_SUCCESS then
    begin
      keybufsize := 0;
      keytype := REG_MULTI_SZ;
    end;
    bufpos := @(keybuf[0]);
    isserisexist := false;
    while (DWORD_PTR(bufpos)-DWORD_PTR(@keybuf[0]) < keybufsize) and (bufpos^ <> #0) do
    begin
      if lstrcmpi(bufpos, PChar(SerName)) = 0 then
      begin
        isserisexist := true;
        Break;
      end;
      bufpos := bufpos+lstrlen(bufpos)+1;
    end;
    if not isserisexist then
    begin
      CopyMemory(bufpos, PChar(SerName), Length(SerName)*sizeof(Char));
      bufpos := bufpos+ Length(SerName);
      bufpos^ := #0;
      Inc(bufpos);
      bufpos^ := #0;
      keyresu := RegSetValueEx(hReg, 'UpperFilters', 0, REG_MULTI_SZ, Pbyte(@(keybuf[0])), keybufsize + ((Length(SerName) + 1) * sizeof(Char)));
      RegFlushKey(hReg);
      result := keyresu = ERROR_SUCCESS;
    end
    else
    begin
      result := True;
    end;
    RegCloseKey(hReg);
  end;
end;

end.
