echo off
IF "%1"=="/n" goto nopt1
IF "%1"=="/N" goto nopt1

regsvr32 /s \ARP\PGSuper\RegFreeCOM\Win32\Release\PGSuperAnalysisAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Win32\Release\PGSuperBeams.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Win32\Release\PGSuperBridgeAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Win32\Release\PGSuperEngAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Win32\Release\PGSuperProjectAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Win32\Release\PGSuperReportAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Win32\Release\PGSuperSpecAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Win32\Release\PGSuperTestAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Win32\Release\WSDOTAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Win32\Release\TxDOTAgent.dll

:nopt1

cscript.exe \ARP\PGSuper\RegressionTest\runregression.js /VRELEASE /Px64 %1 %2

if %errorlevel% NEQ 0 goto nopt2

IF "%1"=="/n" goto nopt2
IF "%1"=="/N" goto nopt2

call UpdateTestResults.bat

:nopt2
