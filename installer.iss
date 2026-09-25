#define MyAppName "MemoraDB"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Vihaan Jain"
#define MyAppExeName "memora.exe"

[Setup]

AppId={{B8D0F2A4-5F7E-4A91-9B13-MEMORADB100}}

AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}

DefaultDirName={localappdata}\MemoraDB

DefaultGroupName=MemoraDB

OutputDir=.

OutputBaseFilename=MemoraDB-Setup-v{#MyAppVersion}

SetupIconFile=MemoraDB.ico

Compression=lzma
SolidCompression=yes

PrivilegesRequired=lowest

DisableProgramGroupPage=yes

UninstallDisplayName=MemoraDB


[Files]

; ============================================================
; MAIN EXECUTABLE
; ============================================================

Source: "build\release\memora.exe"; \
    DestDir: "{app}"; \
    Flags: ignoreversion


; ============================================================
; MEMORADB ICON
; ============================================================

Source: "MemoraDB.ico"; \
    DestDir: "{app}"; \
    Flags: ignoreversion


; ============================================================
; ONNX RUNTIME
; ============================================================

Source: "build\release\onnxruntime.dll"; \
    DestDir: "{app}"; \
    Flags: ignoreversion

Source: "build\release\onnxruntime_providers_shared.dll"; \
    DestDir: "{app}"; \
    Flags: ignoreversion


; ============================================================
; MINGW RUNTIME
; ============================================================

Source: "build\release\libstdc++-6.dll"; \
    DestDir: "{app}"; \
    Flags: ignoreversion

Source: "build\release\libgcc_s_seh-1.dll"; \
    DestDir: "{app}"; \
    Flags: ignoreversion

Source: "build\release\libwinpthread-1.dll"; \
    DestDir: "{app}"; \
    Flags: ignoreversion


; ============================================================
; MINI-LM MODEL
; ============================================================

Source: "build\release\models\all-MiniLM-L6-v2\*"; \
    DestDir: "{app}\models\all-MiniLM-L6-v2"; \
    Flags: ignoreversion recursesubdirs createallsubdirs


[Icons]

; ============================================================
; START MENU
; ============================================================

Name: "{group}\MemoraDB"; \
    Filename: "{app}\memora.exe"; \
    IconFilename: "{app}\MemoraDB.ico"


; ============================================================
; DESKTOP
; ============================================================

Name: "{autodesktop}\MemoraDB"; \
    Filename: "{app}\memora.exe"; \
    IconFilename: "{app}\MemoraDB.ico"


[Run]

; ============================================================
; LAUNCH AFTER INSTALLATION
; ============================================================

Filename: "{app}\memora.exe"; \
    Description: "Launch MemoraDB"; \
    Flags: nowait postinstall skipifsilent