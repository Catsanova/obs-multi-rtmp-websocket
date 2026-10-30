OutFile "obs-multi-rtmp-websocket-setup.exe"

!define PLUGIN_NAME "obs-multi-rtmp-websocket"
; The original plugin reads the same obs-multi-rtmp.json settings, so only one may be installed.
!define ORIGINAL_PLUGIN_NAME "obs-multi-rtmp"

!ifndef PLUGIN_SOURCE_DIR
    !define PLUGIN_SOURCE_DIR "release\Release\${PLUGIN_NAME}"
!endif

Unicode true
RequestExecutionLevel user

SetDatablockOptimize on
SetCompress auto
SetCompressor /SOLID lzma

Name "${PLUGIN_NAME}"
Caption "Multiple RTMP Output Plugin for OBS Studio (WebSocket fork)"
Icon "${NSISDIR}\Contrib\Graphics\Icons\win-install.ico"

Var /Global DefInstDir
Function .onInit
    ReadEnvStr $0 "ALLUSERSPROFILE"
    StrCpy $DefInstDir "$0\obs-studio\plugins"
    StrCpy $INSTDIR "$DefInstDir"

    IfFileExists "$DefInstDir\${PLUGIN_NAME}\*.*" AskUninst DontAskUninst
    AskUninst:
        MessageBox MB_YESNO|MB_ICONQUESTION "Install or remove ${PLUGIN_NAME}? Yes = Install, No = Remove" IDYES NotDoUninst IDNO DoUninst
    DoUninst:
        RMDir /r "$DefInstDir\${PLUGIN_NAME}"
        MessageBox MB_OK|MB_ICONINFORMATION "Done"
        Quit
    NotDoUninst:
    DontAskUninst:
FunctionEnd

Function onDirPageLeave
StrCmp "$INSTDIR" "$DefInstDir" DirNotModified DirModified
DirModified:
MessageBox MB_OK|MB_ICONSTOP "Please don't change the install directory."
Abort
DirNotModified:
FunctionEnd

Page directory "" "" onDirPageLeave
Page instfiles

Section
IfFileExists "$INSTDIR\${ORIGINAL_PLUGIN_NAME}\*.*" 0 NoOriginal
    DetailPrint "Removing original ${ORIGINAL_PLUGIN_NAME} plugin (settings are kept)"
    RMDir /r "$INSTDIR\${ORIGINAL_PLUGIN_NAME}"
NoOriginal:
SetOutPath "$INSTDIR"
File /r "${PLUGIN_SOURCE_DIR}"
SectionEnd
