unit ProfileAnalyzerWrapper;

(*
  Delphi adapter for profile_analyzer.dll (C++).

  * The DLL is loaded lazily on the first call (LoadLibrary) from the directory
    of the module that contains this unit (EXE, DLL or BPL) and stays loaded
    until the unit is finalized (FreeLibrary).
  * The entry point is resolved with GetProcAddress and validated; a missing DLL
    or export makes AnalyzeProfile return False (no exceptions leak to callers).
  * Loading is guarded by a critical section, so the unit is thread-safe.
  * Floating-point exceptions are masked for the duration of the DLL call:
    Delphi unmasks them by default, while MSVC-compiled code expects them masked.

  Requirements: Delphi 2009+ (string = UnicodeString, Char = WideChar).
  Also compiles with Free Pascal in DELPHIUNICODE mode.
  The bitness of the DLL must match the bitness of the application (Win32/Win64).

  NB: Delphi comments do not nest - never put curly braces inside this comment.
*)

{$IFDEF FPC}
  {$MODE DELPHIUNICODE}
{$ENDIF}

interface

type
  TProfileInfo = record
    ProfileType: string;
    NominalHeight: Double;
    WeightPerMeter: Double;
  end;

/// Analyzes a profile designation such as 'IPE200', 'HEA160', 'L150*5'.
/// Returns True on success (AInfo filled). On failure AInfo is reset/partially
/// filled and the reason is available via ProfileAnalyzerLastError*.
function AnalyzeProfile(const AProfileName: string; out AInfo: TProfileInfo): Boolean;

/// Result code of the last AnalyzeProfile call in the current thread
/// (PA_* codes of profile_analyzer.h; PA_E_LIBRARY_NOT_LOADED if the DLL is unavailable).
function ProfileAnalyzerLastErrorCode: Integer;
/// Human readable description of the last result in the current thread.
function ProfileAnalyzerLastErrorText: string;

const
  ProfileAnalyzerDllName = 'profile_analyzer.dll';

  // Result codes, mirror of profile_analyzer.h
  PA_OK                 = 0;
  PA_E_INVALID_ARGUMENT = -1;
  PA_E_EMPTY_NAME       = -2;
  PA_E_INVALID_FORMAT   = -3;
  PA_E_UNKNOWN_TYPE     = -4;
  PA_E_OUT_OF_RANGE     = -5;
  PA_E_INTERNAL         = -100;
  /// Wrapper-only code: the DLL or its export could not be loaded.
  PA_E_LIBRARY_NOT_LOADED = -1000;

implementation

uses
  Windows, SysUtils, Math;

const
  // Must match PA_PROFILE_TYPE_BUFFER_SIZE in profile_analyzer.h
  PA_PROFILE_TYPE_BUFFER_SIZE = 32;

type
  TAnalyzeProfileFunc = function(AProfileName: PWideChar; ANominalHeight: PDouble;
    AWeightPerMeter: PDouble; AProfileType: PWideChar): Integer; stdcall;
  TGetErrorTextFunc = function(ACode: Integer): PWideChar; stdcall;

var
  GLock: TRTLCriticalSection;
  GLibHandle: HMODULE = 0;
  GAnalyzeProfile: TAnalyzeProfileFunc = nil;
  GGetErrorText: TGetErrorTextFunc = nil;
  GLoadAttempted: Boolean = False;
  GLoadError: string = '';

threadvar
  TLastCode: Integer;
  TLastText: string;

function DllPath: string;
begin
  // Absolute path next to the module that contains this unit (not ParamStr(0): the unit
  // may live in a DLL/BPL loaded by a host EXE elsewhere). Protects against DLL
  // search-order hijacking.
  Result := ExtractFilePath(GetModuleName(HInstance)) + ProfileAnalyzerDllName;
end;

function EnsureLoaded: Boolean;
var
  Handle: HMODULE;
  Proc: Pointer;
begin
  Result := False;
  EnterCriticalSection(GLock);
  try
    if GLoadAttempted then
    begin
      Result := Assigned(GAnalyzeProfile);
      Exit;
    end;

    GLoadAttempted := True;

    Handle := LoadLibraryW(PWideChar(DllPath));
    if Handle = 0 then
    begin
      GLoadError := Format(
        'Cannot load %s: %s',
        [DllPath, SysErrorMessage(GetLastError)]
      );
      Exit;
    end;

    Proc := GetProcAddress(Handle, 'AnalyzeProfile');
    if Proc = nil then
    begin
      GLoadError := Format(
        'Function AnalyzeProfile not found in %s',
        [DllPath]
      );
      FreeLibrary(Handle);
      Exit;
    end;

    GLibHandle := Handle;
    GAnalyzeProfile := TAnalyzeProfileFunc(Proc);
    GGetErrorText := TGetErrorTextFunc(GetProcAddress(Handle, 'GetAnalyzeProfileErrorText'));

    Result := True;
  finally
    LeaveCriticalSection(GLock);
  end;
end;

procedure SetLastResult(ACode: Integer; const AText: string);
begin
  TLastCode := ACode;
  TLastText := AText;
end;

function DescribeCode(ACode: Integer): string;
begin
  if Assigned(GGetErrorText) then
    Result := string(GGetErrorText(ACode))
  else
    Result := Format('Result code %d', [ACode]);
end;

function AnalyzeProfile(const AProfileName: string; out AInfo: TProfileInfo): Boolean;
var
  Height, Weight: Double;
  TypeBuffer: array[0..PA_PROFILE_TYPE_BUFFER_SIZE - 1] of WideChar;
  Code: Integer;
  OldMask: TFPUExceptionMask;
begin
  AInfo.ProfileType := '';
  AInfo.NominalHeight := 0;
  AInfo.WeightPerMeter := 0;
  Result := False;

  if not EnsureLoaded then
  begin
    SetLastResult(PA_E_LIBRARY_NOT_LOADED, GLoadError);
    Exit;
  end;

  Height := 0;
  Weight := 0;
  FillChar(TypeBuffer, SizeOf(TypeBuffer), 0);
  // Delphi unmasks FPU exceptions (invalid op, zero divide, overflow), C++ code
  // compiled by MSVC assumes they are masked. Mask them around the foreign call.
  OldMask := SetExceptionMask([exInvalidOp, exDenormalized, exZeroDivide,
    exOverflow, exUnderflow, exPrecision]);
  try
    try
      Code := GAnalyzeProfile(PWideChar(AProfileName), @Height, @Weight, @TypeBuffer[0]);
    except
      // Defensive: a crash inside foreign code must not take the caller down silently.
      on E: Exception do
      begin
        SetLastResult(PA_E_INTERNAL, E.Message);
        Exit;
      end;
    end;
  finally
    SetExceptionMask(OldMask);
  end;
  TypeBuffer[High(TypeBuffer)] := #0; // never trust a foreign buffer blindly

  AInfo.ProfileType := string(PWideChar(@TypeBuffer[0]));
  AInfo.NominalHeight := Height;
  AInfo.WeightPerMeter := Weight;
  SetLastResult(Code, DescribeCode(Code));
  Result := Code >= 0;
end;

function ProfileAnalyzerLastErrorCode: Integer;
begin
  Result := TLastCode;
end;

function ProfileAnalyzerLastErrorText: string;
begin
  Result := TLastText;
end;

initialization
  InitializeCriticalSection(GLock);

finalization
  if GLibHandle <> 0 then
    FreeLibrary(GLibHandle);
  GLibHandle := 0;
  GAnalyzeProfile := nil;
  GGetErrorText := nil;
  DeleteCriticalSection(GLock);

end.
