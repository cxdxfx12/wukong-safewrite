; 悟空运费结算 — Inno Setup 安装包脚本
; 使用: "C:\Program Files\Inno Setup 6\ISCC.exe" setup.iss
; 输出: 悟空结算_v1.2.3_Setup.exe

[Setup]
AppId={{8F3A5C2D-1E6B-4A9D-8F3E-7B2C1D5A8E9F}
AppName=悟空运费结算
AppVersion=1.2.3
AppPublisher=杭州喵喵至家网络有限公司
DefaultDirName={autopf}\悟空运费结算
DefaultGroupName=悟空运费结算
UninstallDisplayIcon={app}\悟空结算.exe
Compression=lzma2/max
SolidCompression=yes
OutputDir=.
OutputBaseFilename=悟空结算_v1.2.3_Setup
DisableProgramGroupPage=yes
PrivilegesRequired=admin
WizardStyle=modern
SetupIconFile=..\resources\app_icon.ico


[Files]
Source: "..\build\悟空结算.exe"; DestDir: "{app}"
Source: "..\build\*.dll"; DestDir: "{app}"
Source: "..\build\platforms\*.dll"; DestDir: "{app}\platforms"
Source: "..\build\styles\*.dll"; DestDir: "{app}\styles"
Source: "..\build\imageformats\*.dll"; DestDir: "{app}\imageformats"
Source: "..\build\iconengines\*.dll"; DestDir: "{app}\iconengines"
Source: "..\build\networkinformation\*.dll"; DestDir: "{app}\networkinformation"
Source: "..\build\tls\*.dll"; DestDir: "{app}\tls"
Source: "..\build\translations\*.qm"; DestDir: "{app}\translations"

[Icons]
Name: "{autoprograms}\悟空运费结算\悟空运费结算"; Filename: "{app}\悟空结算.exe"
Name: "{autoprograms}\悟空运费结算\卸载"; Filename: "{uninstallexe}"
Name: "{autodesktop}\悟空运费结算"; Filename: "{app}\悟空结算.exe"

[Run]
Filename: "{app}\悟空结算.exe"; Description: "启动悟空运费结算"; Flags: nowait postinstall skipifsilent
