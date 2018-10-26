@echo off
REM -- First make map file from Microsoft Visual C++ generated resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by LIBRARYEDITOR.HPJ. >"hlp\LibraryEditor.hm"
echo. >>"hlp\LibraryEditor.hm"
echo // Commands (ID_* and IDM_*) >>"hlp\LibraryEditor.hm"
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>"hlp\LibraryEditor.hm"
echo. >>"hlp\LibraryEditor.hm"
echo // Prompts (IDP_*) >>"hlp\LibraryEditor.hm"
makehm IDP_,HIDP_,0x30000 resource.h >>"hlp\LibraryEditor.hm"
echo. >>"hlp\LibraryEditor.hm"
echo // Resources (IDR_*) >>"hlp\LibraryEditor.hm"
makehm IDR_,HIDR_,0x20000 resource.h >>"hlp\LibraryEditor.hm"
echo. >>"hlp\LibraryEditor.hm"
echo // Dialogs (IDD_*) >>"hlp\LibraryEditor.hm"
makehm IDD_,HIDD_,0x20000 resource.h >>"hlp\LibraryEditor.hm"
echo. >>"hlp\LibraryEditor.hm"
echo // Frame Controls (IDW_*) >>"hlp\LibraryEditor.hm"
makehm IDW_,HIDW_,0x50000 resource.h >>"hlp\LibraryEditor.hm"
REM -- Make help for Project LIBRARYEDITOR


echo Building Win32 Help files
start /wait hcw /C /E /M "hlp\LibraryEditor.hpj"
if errorlevel 1 goto :Error
if not exist "hlp\LibraryEditor.hlp" goto :Error
if not exist "hlp\LibraryEditor.cnt" goto :Error
echo.
if exist Debug\nul copy "hlp\LibraryEditor.hlp" Debug
if exist Debug\nul copy "hlp\LibraryEditor.cnt" Debug
if exist Release\nul copy "hlp\LibraryEditor.hlp" Release
if exist Release\nul copy "hlp\LibraryEditor.cnt" Release
echo.
goto :done

:Error
echo hlp\LibraryEditor.hpj(1) : error: Problem encountered creating help file

:done
echo.
