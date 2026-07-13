; 悟空运费结算 — NSIS 安装包脚本
; 使用: makensis setup.nsi   (NSIS 3.x 需预装)
; 输出: 悟空结算_v1.1.8_Setup.exe

Unicode true
RequestExecutionLevel admin
!include "MUI2.nsh"

; ------------------- 基本信息 -------------------
Name "悟空运费结算"
OutFile "悟空结算_v1.1.8_Setup.exe"
InstallDir "$PROGRAMFILES64\悟空运费结算"
BrandingText "杭州喵喵至家网络有限公司"
SetCompressor /SOLID lzma

; ------------------- 版本信息 (安装包本身) -------------------
VIProductVersion "1.1.8.0"
VIAddVersionKey "ProductName"     "悟空运费结算"
VIAddVersionKey "CompanyName"     "杭州喵喵至家网络有限公司"
VIAddVersionKey "FileVersion"     "1.1.8"
VIAddVersionKey "FileDescription" "悟空运费结算安装程序"
VIAddVersionKey "LegalCopyright"  "Copyright (C) 2026 杭州喵喵至家"

; ------------------- 界面 -------------------
!define MUI_ICON "..\resources\app_icon.ico"
!define MUI_UNICON "..\resources\app_icon.ico"
!define MUI_ABORTWARNING

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "SimpChinese"

; ------------------- 安装 -------------------
Section "Install"
    SetOutPath "$INSTDIR"

    ; 主程序 + DLL
    File "..\build\悟空结算.exe"
    File "..\build\*.dll"

    ; Qt 插件目录
    SetOutPath "$INSTDIR\platforms"
    File "..\build\platforms\*.dll"

    SetOutPath "$INSTDIR\styles"
    File "..\build\styles\*.dll"

    SetOutPath "$INSTDIR\imageformats"
    File "..\build\imageformats\*.dll"

    SetOutPath "$INSTDIR\iconengines"
    File /nonfatal "..\build\iconengines\*.dll"

    SetOutPath "$INSTDIR\networkinformation"
    File /nonfatal "..\build\networkinformation\*.dll"

    SetOutPath "$INSTDIR\tls"
    File /nonfatal "..\build\tls\*.dll"

    SetOutPath "$INSTDIR\translations"
    File /nonfatal "..\build\translations\*.qm"

    SetOutPath "$INSTDIR\generic"
    File /nonfatal "..\build\generic\*.dll"

    ; 快捷方式
    CreateDirectory "$SMPROGRAMS\悟空运费结算"
    CreateShortCut "$SMPROGRAMS\悟空运费结算\悟空运费结算.lnk" "$INSTDIR\悟空结算.exe"
    CreateShortCut "$SMPROGRAMS\悟空运费结算\卸载.lnk" "$INSTDIR\uninst.exe"
    CreateShortCut "$DESKTOP\悟空运费结算.lnk" "$INSTDIR\悟空结算.exe"

    ; 卸载程序
    WriteUninstaller "$INSTDIR\uninst.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\WukongFreight" "DisplayName" "悟空运费结算"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\WukongFreight" "UninstallString" "$INSTDIR\uninst.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\WukongFreight" "Publisher" "杭州喵喵至家网络有限公司"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\WukongFreight" "DisplayVersion" "1.1.8"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\WukongFreight" "DisplayIcon" "$INSTDIR\悟空结算.exe"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\WukongFreight" "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\WukongFreight" "NoRepair" 1
SectionEnd

; ------------------- 卸载 -------------------
Section "Uninstall"
    RMDir /r "$INSTDIR"
    Delete "$DESKTOP\悟空运费结算.lnk"
    RMDir /r "$SMPROGRAMS\悟空运费结算"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\WukongFreight"
SectionEnd
