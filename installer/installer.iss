; HypeClip AI — Inno Setup installer (Windows x64)
; Drops the built plugin DLL + data folder into the user's OBS install.
; Build the plugin first (Release), then compile this script with Inno Setup 6+.
;
;   ISCC.exe installer.iss /DBuildDir="..\build_x64\rundir\Release"

#define MyAppName    "HypeClip AI"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "HypeClip AI"
#define MyAppURL     "https://github.com/hypeclip/hypeclip-ai"
#ifndef BuildDir
  #define BuildDir   "..\release"
#endif

[Setup]
AppId={{8F3C2A10-7E5B-4C9A-9B21-HYPECLIPAI001}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
DefaultDirName={code:GetOBSDir}
DefaultGroupName={#MyAppName}
DisableDirPage=no
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputBaseFilename=HypeClipAI-Setup-{#MyAppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName={#MyAppName}
SetupIconFile=hypeclip.ico

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; Plugin module
Source: "{#BuildDir}\obs-plugins\64bit\hypeclip-ai.dll"; \
  DestDir: "{app}\obs-plugins\64bit"; Flags: ignoreversion
; Locale + data (profiles, etc.)
Source: "{#BuildDir}\data\obs-plugins\hypeclip-ai\*"; \
  DestDir: "{app}\data\obs-plugins\hypeclip-ai"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"

[Run]
Filename: "{code:GetOBSExe}"; Description: "Launch OBS Studio"; \
  Flags: nowait postinstall skipifsilent unchecked

[Code]
{ Locate the OBS Studio install dir from the registry; fall back to Program Files. }
function GetOBSDir(Param: String): String;
var
  path: String;
begin
  if RegQueryStringValue(HKLM, 'SOFTWARE\OBS Studio', '', path) and (path <> '') then
    Result := path
  else
    Result := ExpandConstant('{commonpf64}\obs-studio');
end;

function GetOBSExe(Param: String): String;
begin
  Result := AddBackslash(GetOBSDir('')) + 'bin\64bit\obs64.exe';
end;

function InitializeSetup(): Boolean;
begin
  if not RegKeyExists(HKLM, 'SOFTWARE\OBS Studio') and
     not DirExists(ExpandConstant('{commonpf64}\obs-studio')) then
  begin
    if MsgBox('OBS Studio was not detected. Install it first, or continue and ' +
              'choose the OBS folder manually?', mbConfirmation, MB_YESNO) = IDNO then
    begin
      Result := False; Exit;
    end;
  end;
  Result := True;
end;
