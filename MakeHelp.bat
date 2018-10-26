@echo off
REM -- First make map file from Microsoft Visual C++ generated resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by PGSUPER.HPJ. >"hlp\PGSuper.hm"
echo. >>"hlp\PGSuper.hm"
echo // Commands (ID_* and IDM_*) >>"hlp\PGSuper.hm"
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>"hlp\PGSuper.hm"
echo. >>"hlp\PGSuper.hm"
echo // Prompts (IDP_*) >>"hlp\PGSuper.hm"
makehm IDP_,HIDP_,0x30000 resource.h >>"hlp\PGSuper.hm"
echo. >>"hlp\PGSuper.hm"
echo // Resources (IDR_*) >>"hlp\PGSuper.hm"
makehm IDR_,HIDR_,0x20000 resource.h >>"hlp\PGSuper.hm"
echo. >>"hlp\PGSuper.hm"
echo // Dialogs (IDD_*) >>"hlp\PGSuper.hm"
makehm IDD_,HIDD_,0x20000 resource.h >>"hlp\PGSuper.hm"
echo. >>"hlp\PGSuper.hm"
echo // Frame Controls (IDW_*) >>"hlp\PGSuper.hm"
makehm IDW_,HIDW_,0x50000 resource.h >>"hlp\PGSuper.hm"
REM -- Make help for Project PGSUPER


echo Building Win32 Help files
start /wait hcw /C /E /M "hlp\PGSuper.hpj"
if errorlevel 1 goto :Error
if not exist "hlp\PGSuper.hlp" goto :Error
if not exist "hlp\PGSuper.cnt" goto :Error
echo.
if exist Debug\nul copy "hlp\PGSuper.hlp" Debug
if exist Debug\nul copy "hlp\PGSuper.cnt" Debug
if exist Release\nul copy "hlp\PGSuper.hlp" Release
if exist Release\nul copy "hlp\PGSuper.cnt" Release
echo.
goto :done

:Error
echo hlp\PGSuper.hpj(1) : error: Problem encountered creating help file

:done
echo.
