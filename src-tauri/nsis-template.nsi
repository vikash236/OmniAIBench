; Custom NSIS template for OmniAIBench
; Sets shortcuts to run as administrator automatically

!include "MUI2.nsh"
!include "FileFunc.nsh"

; Request admin for installer
RequestExecutionLevel admin

; Plugin for setting shortcut admin flag
!include "StdUtils.nsh"

; Variables
Var StartMenuFolder

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Welcome page with admin notice
!define MUI_WELCOMEPAGE_TEXT "This application requires administrator privileges to access hardware sensors.$\r$\n$\r$\nShortcuts will be configured to run as administrator automatically."
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; Installer sections
Section "Install"
    SetOutPath "$INSTDIR"
    
    ; Install files (populated by Tauri build)
    {{tauri_bundle_install_files}}
    
    ; Create shortcuts
    CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
    CreateShortcut "$SMPROGRAMS\$StartMenuFolder\OmniAIBench.lnk" "$INSTDIR\OmniAIBench.exe"
    CreateShortcut "$DESKTOP\OmniAIBench.lnk" "$INSTDIR\OmniAIBench.exe"
    
    ; Set shortcuts to run as administrator
    ${StdUtils.ExecShellAsUser} $0 "$SMPROGRAMS\$StartMenuFolder\OmniAIBench.lnk" "runas" ""
    ${StdUtils.ExecShellAsUser} $0 "$DESKTOP\OmniAIBench.lnk" "runas" ""
    
    ; Alternative method using registry
    WriteRegStr HKLM "Software\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers" "$INSTDIR\OmniAIBench.exe" "RUNASADMIN"
    
    ; Create uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"
    
    ; Registry entries
    WriteRegStr HKLM "Software\OmniAIBench" "InstallDir" "$INSTDIR"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OmniAIBench" "DisplayName" "OmniAIBench"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OmniAIBench" "UninstallString" "$INSTDIR\Uninstall.exe"
SectionEnd

; Uninstaller section
Section "Uninstall"
    Delete "$DESKTOP\OmniAIBench.lnk"
    Delete "$SMPROGRAMS\$StartMenuFolder\OmniAIBench.lnk"
    RMDir "$SMPROGRAMS\$StartMenuFolder"
    
    Delete "$INSTDIR\Uninstall.exe"
    {{tauri_bundle_uninstall_files}}
    RMDir "$INSTDIR"
    
    DeleteRegKey HKLM "Software\OmniAIBench"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OmniAIBench"
    DeleteRegValue HKLM "Software\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers" "$INSTDIR\OmniAIBench.exe"
SectionEnd
