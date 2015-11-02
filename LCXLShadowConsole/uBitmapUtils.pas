unit uBitmapUtils;

interface

uses
  Graphics, Windows;

type
  TColor2Grayscale = (
    c2gAverage,
    c2gLightness,
    c2gLuminosity
  );

  TBitmapAccess = (
    baScanLine,
    baPixels
  );

  TBitmapUtils = class
    class procedure ToGray(aBitmap: Graphics.TBitmap;
      cg: TColor2Grayscale = c2gLuminosity;
      ba: TBitmapAccess = baScanLine); static;
  end;

procedure ToGray(
  aBitmap: Graphics.TBitmap;
  cg: TColor2Grayscale = c2gLuminosity;
  ba: TBitmapAccess = baScanLine
);

implementation

uses
  SysUtils, Math;

function Min(const A, B, C: integer): integer;
begin
  Result := Math.Min(A, Math.Min(B,C));
end;

function Max(const A, B, C: integer): integer;
begin
  Result := Math.Max(A, Math.Max(B,C));
end;

function RGBToGray(R, G, B: byte; cg: TColor2Grayscale = c2gLuminosity): byte;
begin
  case cg of
    c2gAverage:
      Result := (R + G + B) div 3;

    c2gLightness:
      Result := (max(R, G, B) + min(R, G, B)) div 2;

    c2gLuminosity:
      Result := round(0.2989*R + 0.5870*G + 0.1141*B); // coeffs from Matlab

    else
      raise Exception.Create('Unknown Color2Grayscale value');
  end;
end;

function ColorToGray(c: TColor; cg: TColor2Grayscale = c2gLuminosity): TColor;
var R, G, B, X : byte;
begin
  R := c and $ff;
  G := (c and $ff00) shr 8;
  B := (c and $ff0000) shr 16;
  X := RGBToGray(R,G,B,cg);
  Result := TColor(RGB(X,X,X));
end;

procedure ToGray(aBitmap: Graphics.TBitmap;
  cg: TColor2Grayscale = c2gLuminosity;
  ba: TBitmapAccess = baScanLine);
var w, h: integer; CurrRow, OffSet: integer;
  x: byte; pRed, pGreen, pBlue: PByte;
begin
  if ba = baPixels then
  begin
    if aBitmap <> nil then
      for h := 0 to aBitmap.Height - 1 do
        for w := 0 to aBitmap.Width - 1 do
          aBitmap.Canvas.Pixels[w,h] :=
            ColorToGray(aBitmap.Canvas.Pixels[w,h], cg);
  end

  else // ba = baScanLine
  begin
    if aBitmap.PixelFormat <> pf24bit then
      raise Exception.Create(
        'Not implemented. PixelFormat has to be "pf24bit"');

    CurrRow := Integer(aBitmap.ScanLine[0]);
    OffSet := Integer(aBitmap.ScanLine[1]) - CurrRow;

    for h := 0 to aBitmap.Height - 1 do
    begin
      for w := 0 to aBitmap.Width - 1 do
      begin
        pBlue  := pByte(CurrRow + w*3);
        pGreen := pByte(CurrRow + w*3 + 1);
        pRed   := pByte(CurrRow + w*3 + 2);
        x := RGBToGray(pRed^, pGreen^, pBlue^, cg);
        pBlue^  := x;
        pGreen^ := x;
        pRed^   := x;
      end;
      inc(CurrRow, OffSet);
    end;
  end
end;

{ TBitmapUtils }

class procedure TBitmapUtils.ToGray(aBitmap: Graphics.TBitmap;
  cg: TColor2Grayscale; ba: TBitmapAccess);
begin
  ToGray(aBitmap, cg, ba);
end;

end.
