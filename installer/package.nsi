; usage:
;    1: copy an image(installer.ico) to to the same directory of this script file,
;       which is installer's icon.
;    2. make a folder named RotateDisp-installer;
;    3. copy RotateDisp.exe to this folder.
;    4: use windeployqt.exe RotateDisp.exe command to copy all of qt dependcies.
;    5: 

; Unicode True

RequestExecutionLevel user

; HM NIS Edit Wizard helper defines
!define PRODUCT_NAME "RotateDisp"
!define PRODUCT_VERSION "1.0"
!define PRODUCT_PUBLISHER "doufu"
!define PRODUCT_WEB_SITE "doufu.de"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\RotateDisp.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKCU"

; MUI 1.67 compatible ------
!include "MUI.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "installer.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Language Selection Dialog Settings
!define MUI_LANGDLL_REGISTRY_ROOT "${PRODUCT_UNINST_ROOT_KEY}"
!define MUI_LANGDLL_REGISTRY_KEY "${PRODUCT_UNINST_KEY}"
!define MUI_LANGDLL_REGISTRY_VALUENAME "NSIS:Language"

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!define MUI_FINISHPAGE_RUN "$INSTDIR\RotateDisp.exe"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "SimpChinese"

; MUI end ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "RotateDisp-installer.exe"
InstallDir "$LOCALAPPDATA\RotateDisp"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

#卸载程序翻译
LangString UNINSTALL_SUCC ${LANG_ENGLISH} "$(^Name) uninstall successfully?"
LangString UNINSTALL_SUCC ${LANG_SIMPCHINESE} "$(^Name) 已成功地从你的计算机移除。"

LangString UNINSTALL_CONFIRM ${LANG_ENGLISH} "Are you sure to uninstall $(^Name)?"
LangString UNINSTALL_CONFIRM ${LANG_SIMPCHINESE} "你确定要移除 $(^Name)？"


LangString VCREDIRT_INSTALL ${LANG_ENGLISH} "VisualC++ runtime library is not installed on this system, start the installation"
LangString VCREDIRT_INSTALL ${LANG_SIMPCHINESE} "此系统未安装VisualC++运行时库，开始安装"



Function .onInit
  #选择语言
  Push ""
  Push ${LANG_ENGLISH}
  Push "English"
  Push ${LANG_SIMPCHINESE}
  Push "简体中文"
  Push A
  
  #显示语言选择框
  LangDLL::LangDialog "Installer Language" "Please select the language of the installer"
  Pop $LANGUAGE
  StrCmp $LANGUAGE "cancel" 0 +2
  Abort
  
FunctionEnd

Section "MainSection" SEC01
  SetOutPath "$INSTDIR"
  SetOverwrite try
  File "RotateDisp-installer\d3dcompiler_47.dll"
  File "RotateDisp-installer\GY25T.exe"
  SetOutPath "$INSTDIR\iconengines"
  File "RotateDisp-installer\iconengines\qsvgicon.dll"
  SetOutPath "$INSTDIR\imageformats"
  File "RotateDisp-installer\imageformats\qgif.dll"
  File "RotateDisp-installer\imageformats\qicns.dll"
  File "RotateDisp-installer\imageformats\qico.dll"
  File "RotateDisp-installer\imageformats\qjpeg.dll"
  File "RotateDisp-installer\imageformats\qsvg.dll"
  File "RotateDisp-installer\imageformats\qtga.dll"
  File "RotateDisp-installer\imageformats\qtiff.dll"
  File "RotateDisp-installer\imageformats\qwbmp.dll"
  File "RotateDisp-installer\imageformats\qwebp.dll"
  SetOutPath "$INSTDIR"
  File "RotateDisp-installer\libEGL.dll"
  File "RotateDisp-installer\libGLESV2.dll"
  File "RotateDisp-installer\opengl32sw.dll"
  SetOutPath "$INSTDIR\platforms"
  File "RotateDisp-installer\platforms\qwindows.dll"
  SetOutPath "$INSTDIR"
  File "RotateDisp-installer\Qt5Core.dll"
  File "RotateDisp-installer\Qt5Gui.dll"
  File "RotateDisp-installer\Qt5SerialPort.dll"
  File "RotateDisp-installer\Qt5Svg.dll"
  File "RotateDisp-installer\Qt5Widgets.dll"
  File "RotateDisp-installer\RotateDisp.exe"
  CreateDirectory "$SMPROGRAMS\RotateDisp"
  CreateShortCut "$SMPROGRAMS\RotateDisp\RotateDisp.lnk" "$INSTDIR\RotateDisp.exe"
  CreateShortCut "$DESKTOP\RotateDisp.lnk" "$INSTDIR\RotateDisp.exe"
  SetOutPath "$INSTDIR\styles"
  File "RotateDisp-installer\styles\qwindowsvistastyle.dll"
  SetOutPath "$INSTDIR\translations"
  File "RotateDisp-installer\translations\*.qm"
  SetOutPath "$INSTDIR"
  File "RotateDisp-installer\vc_redist.x86.exe"
SectionEnd

Section -AdditionalIcons
  CreateShortCut "$SMPROGRAMS\RotateDisp\Uninstall.lnk" "$INSTDIR\uninst.exe"
SectionEnd

Section -Post
  ReadRegStr $0 HKLM "SOFTWARE\WOW6432Node\Microsoft\VisualStudio\14.0\VC\Runtimes\x86" "Installed"
  StrCmp $0 1 +3
  MessageBox MB_ICONINFORMATION|MB_OK  "$(VCREDIRT_INSTALL)"
  ExecWait "$INSTDIR\vc_redist.x86.exe"

  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\GY25T.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\GY25T.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
SectionEnd

Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(UNINSTALL_SUCC)"
FunctionEnd

Function un.onInit
!insertmacro MUI_UNGETLANGUAGE
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "$(UNINSTALL_CONFIRM)" IDYES +2
  Abort
FunctionEnd

Section Uninstall
  Delete "$INSTDIR\uninst.exe"
  Delete "$INSTDIR\vc_redist.x86.exe"
  Delete "$INSTDIR\translations\RotateDisp_zh_CN.qm"
  Delete "$INSTDIR\translations\qt_zh_CN.qm"
  Delete "$INSTDIR\translations\qtbase_zh_CN.qm"
  Delete "$INSTDIR\styles\qwindowsvistastyle.dll"
  Delete "$INSTDIR\RotateDisp.exe"
  Delete "$INSTDIR\Qt5Widgets.dll"
  Delete "$INSTDIR\Qt5Svg.dll"
  Delete "$INSTDIR\Qt5SerialPort.dll"
  Delete "$INSTDIR\Qt5Gui.dll"
  Delete "$INSTDIR\Qt5Core.dll"
  Delete "$INSTDIR\platforms\qwindows.dll"
  Delete "$INSTDIR\opengl32sw.dll"
  Delete "$INSTDIR\libGLESV2.dll"
  Delete "$INSTDIR\libEGL.dll"
  Delete "$INSTDIR\imageformats\qwebp.dll"
  Delete "$INSTDIR\imageformats\qwbmp.dll"
  Delete "$INSTDIR\imageformats\qtiff.dll"
  Delete "$INSTDIR\imageformats\qtga.dll"
  Delete "$INSTDIR\imageformats\qsvg.dll"
  Delete "$INSTDIR\imageformats\qjpeg.dll"
  Delete "$INSTDIR\imageformats\qico.dll"
  Delete "$INSTDIR\imageformats\qicns.dll"
  Delete "$INSTDIR\imageformats\qgif.dll"
  Delete "$INSTDIR\iconengines\qsvgicon.dll"
  Delete "$INSTDIR\GY25T.exe"
  Delete "$INSTDIR\d3dcompiler_47.dll"

  Delete "$SMPROGRAMS\RotateDisp\Uninstall.lnk"
  Delete "$DESKTOP\RotateDisp.lnk"
  Delete "$SMPROGRAMS\RotateDisp\RotateDisp.lnk"

  RMDir "$SMPROGRAMS\RotateDisp"
  RMDir "$INSTDIR\translations"
  RMDir "$INSTDIR\styles"
  RMDir "$INSTDIR\platforms"
  RMDir "$INSTDIR\imageformats"
  RMDir "$INSTDIR\iconengines"
  RMDir /r "$INSTDIR"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  SetAutoClose true
SectionEnd