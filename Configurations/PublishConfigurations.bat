@echo off
REM ---------------------------------------------------------------------------------------
REM Publishes the WSDOT configurations for the current program version.
REM Update VERSION below for each release.
REM
REM The packages are published on the WSDOT web site and listed in ConfigurationPackages.json:
REM    https://wsdot.wa.gov/eesc/bridge/software/configurations/
REM
REM Until version 10.0, the same packages are also copied to the WSDOT FTP site as .pgz files
REM for program versions that read PGSuperPackages.ini and PGSplicePackages.ini.
REM Update those INI files on the FTP site by hand, as before.
REM
REM Every package is built, but only configurations marked LIST are added to
REM ConfigurationPackages.json for this version. The others keep using the version they
REM are listed for (for example, PCI UHPC and FHWA UHPC stay at 7.1.0). This matches the
REM hand-edited INI files. To list one of them for this version, change BUILD to LIST, or run:
REM    ConfigPackager /AddPackage <json> PGSuper "PCI UHPC" <version> <package>
REM ---------------------------------------------------------------------------------------

SET VERSION=9.1.0
SET CONFIGPACKAGER=%ARPDIR%\BridgeLink\RegFreeCOM\x64\Release\ConfigPackager.exe
SET SOURCE=%ARPDIR%\PGSuper\Configurations
SET WEB=\\wsdot\Resources\Topics\Publish\Web\ProdCF\EESC\Bridge\software\configurations
SET MANIFEST=%WEB%\ConfigurationPackages.json
SET WEBLINK=https://wsdot.wa.gov/eesc/bridge/software/configurations/LibraryInfo.html
SET FTP=\\wsdot\resources\Topics\Publish\FTP\Data\public\Bridge\Software

REM -------------------------------------------
REM - PGSuper Configurations
REM -------------------------------------------
call :Publish PGSuper "WSDOT" WSDOT PGT LIST || exit /b 1
call :Publish PGSuper "AASHTO Standard Girders from PCI BDM" AASHTO PGT BUILD || exit /b 1
call :Publish PGSuper "PCI UHPC" PCI_UHPC PGT BUILD || exit /b 1
call :Publish PGSuper "FHWA UHPC" FHWA_UHPC PGT BUILD || exit /b 1

REM -------------------------------------------
REM - PGSplice Configurations
REM -------------------------------------------
call :Publish PGSplice "WSDOT" WSDOT SPT LIST || exit /b 1

REM -------------------------------------------
REM - Documentation for the configurations
REM -------------------------------------------
copy /y "%SOURCE%\LibraryInfo.html" "%WEB%\"
copy /y "%SOURCE%\WSDOTLogoGreen.png" "%WEB%\"

REM Check the published list of configurations
"%CONFIGPACKAGER%" /Check "%MANIFEST%"
exit /b %ERRORLEVEL%


REM ---------------------------------------------------------------------------------------
REM :Publish Program "Configuration Name" FileName TemplateExtension LIST|BUILD
REM ---------------------------------------------------------------------------------------
:Publish
SET TARGET=%WEB%\%~1\Version_%VERSION%
if not exist "%TARGET%" mkdir "%TARGET%"
del /q "%TARGET%\%~3.*" 2>nul

"%CONFIGPACKAGER%" "%TARGET%\%~3.zip" "%SOURCE%\%~3.lbr" "%SOURCE%\%~1\%~3" %~4 || exit /b 1
if /i "%~5"=="LIST" (
   "%CONFIGPACKAGER%" /AddPackage "%MANIFEST%" %~1 "%~2" %VERSION% "%TARGET%\%~3.zip" /WebLink=%WEBLINK% || exit /b 1
) else (
   echo %~2 built but not listed for version %VERSION%
)

REM The checksum is in ConfigurationPackages.json. The web site won't serve .md5 files.
SET FTPTARGET=%FTP%\%~1\Version_%VERSION%
if not exist "%FTPTARGET%" mkdir "%FTPTARGET%"
copy /y "%TARGET%\%~3.zip" "%FTPTARGET%\%~3.pgz" || exit /b 1
move /y "%TARGET%\%~3.zip.md5" "%FTPTARGET%\%~3.pgz.md5" || exit /b 1
exit /b 0
