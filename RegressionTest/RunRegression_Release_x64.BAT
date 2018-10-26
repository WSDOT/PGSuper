echo off
IF "%1"=="/n" goto nopt1
IF "%1"=="/N" goto nopt1

regsvr32 /s \ARP\PGSuper\RegFreeCOM\x64\Release\PGSuperAnalysisAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\x64\Release\PGSuperBeams.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\x64\Release\PGSuperBridgeAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\x64\Release\PGSuperEngAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\x64\Release\PGSuperProjectAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\x64\Release\PGSuperReportAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\x64\Release\PGSuperSpecAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\x64\Release\PGSuperTestAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\x64\Release\WSDOTAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\x64\Release\TxDOTAgent.dll

:nopt1

cscript.exe \ARP\PGSuper\RegressionTest\runregression.js /VRELEASE /Px64 %1 %2

if %errorlevel% NEQ 0 goto nopt2

IF "%1"=="/n" goto nopt2
IF "%1"=="/N" goto nopt2

call UpdateTestResults.bat

:nopt2
