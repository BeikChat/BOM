program ProfileAnalyzerDemo;

{
  Console demo for ProfileAnalyzerWrapper.pas.

  Usage:
    ProfileAnalyzerDemo.exe                 - analyzes a built-in list of samples
    ProfileAnalyzerDemo.exe IPE200 L150*5   - analyzes the given names

  profile_analyzer.dll of the same bitness must be next to the executable.
}

{$IFDEF FPC}
  {$MODE DELPHIUNICODE}
{$ENDIF}
{$APPTYPE CONSOLE}

uses
  SysUtils,
  ProfileAnalyzerWrapper in 'ProfileAnalyzerWrapper.pas';

const
  Samples: array[0..8] of string = (
    'IPE200', 'HEA160', 'HEB300', 'L150*5', 'RHS100x50x4', 'CHS114.3x4', 'IPE210', 'XYZ100', 'IPE');

var
  FailureCount: Integer = 0;

procedure Analyze(const AName: string);
var
  Info: TProfileInfo;
begin
  if ProfileAnalyzerWrapper.AnalyzeProfile(AName, Info) then
  begin
    Writeln(Format('  %-14s type=%-4s height=%7.1f mm  weight=%8.3f kg/m',
      [AName, Info.ProfileType, Info.NominalHeight, Info.WeightPerMeter]));
  end
  else
  begin
    Inc(FailureCount);
    Writeln(Format('  %-14s FAILED, code %d: %s',
      [AName, ProfileAnalyzerLastErrorCode, ProfileAnalyzerLastErrorText]));
  end;
end;

var
  I: Integer;
begin
  Writeln('Profile analyzer demo (', SizeOf(Pointer) * 8, '-bit), DLL: ', ProfileAnalyzerDllName);
  if ParamCount > 0 then
    for I := 1 to ParamCount do
      Analyze(ParamStr(I))
  else
    for I := Low(Samples) to High(Samples) do
      Analyze(Samples[I]);

  // For explicit names: exit code = number of failed analyses (handy for scripted smoke tests).
  // The built-in samples intentionally include invalid names, so they always exit with 0.
  if ParamCount > 0 then
    ExitCode := FailureCount;
end.
